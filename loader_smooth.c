/* 
 *     loader_smooth.c
 *
 * 2006 - 2007 Copyright (c)
 * Robert Iakobashvili, <coroberti@gmail.com>
 * Michael Moser, <moser.michael@gmail.com>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Cooked from the CURL-project examples with thanks to the 
 * great CURL-project authors and contributors.
 */

// must be first include
#include "fdsetsize.h"

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>

#include "batch.h"
#include "client.h"
#include "loader.h"
#include "conf.h"
#include "screen.h"


static int mget_url_smooth (batch_context* bctx);
static int mperform_smooth (batch_context* bctx,
                            unsigned long* now_time,
                            int* still_running);

/******************************************************************************
 * Function name - user_activity_smooth
 *
 * Description - Simulates user-activities using SMOOTH-MODE
 * Input -       *cctx_array - array of client contexts (related to a certain 
 *                             batch of clients)
 * Return Code/Output - On Success - 0, on Error -1
 *******************************************************************************/
int user_activity_smooth (client_context* cctx_array)
{
  batch_context* bctx = cctx_array->bctx;

  if (!bctx)
    {
      fprintf (stderr, "%s - error: bctx is a NULL pointer.\n", __func__);
      return -1;
    }

  if (alloc_init_timer_waiting_queue (bctx->client_num_max + PERIODIC_TIMERS_NUMBER + 1,
                                      &bctx->waiting_queue) == -1)
    {
      fprintf (stderr, 
               "%s - error: failed to alloc or init timer waiting queue.\n", 
               __func__);
      return -1;
    }

  const unsigned long now_time = get_tick_count ();
  
  if (init_timers_and_add_initial_clients_to_load (bctx, now_time) == -1)
    {
      fprintf (stderr, 
               "%s - error: init_timers_and_add_initial_clients_to_load () failed.\n", 
               __func__);
      return -1;
    }

  if (is_batch_group_leader (bctx))
    {
      dump_snapshot_interval (bctx, now_time);
    }

  /* 
     ========= Run the loading machinery ================
  */
  while ((pending_active_and_waiting_clients_num (bctx)) ||
         bctx->do_client_num_gradual_increase)
    {
      if (mget_url_smooth (bctx) == -1)
        {
          fprintf (stderr, "%s error: mget_url () failed.\n", __func__) ;
          return -1;
        }
    }

  dump_final_statistics (cctx_array);
  screen_release ();

  /* 
     ======= Release resources =========================
  */
  if (bctx->waiting_queue)
    {
      /* Cancel periodic timers */
      cancel_periodic_timers (bctx);

      tq_release (bctx->waiting_queue);
      free (bctx->waiting_queue);
      bctx->waiting_queue = 0;
    }

  return 0;
}



/*******************************************************************************
 * Function name - mget_url_smooth
 *
 * Description - Performs actual fetching of urls for a whole batch. Starts 
 *               with initial fetch by mperform_smooth () and further acts 
 *               using mperform_smooth () on select events
 *
 * Input -       *bctx - pointer to the batch of contexts
 *
 * Return Code/Output - On Success - 0, on Error -1
 ********************************************************************************/
static int mget_url_smooth (batch_context* bctx)  		       
{
  int max_timeout_msec = 1000;
  unsigned long now_time = get_tick_count ();
  int cycle_counter = 0;
    
  int still_running = 0;
  struct timeval timeout;

  mperform_smooth (bctx, &now_time, &still_running);

  while (max_timeout_msec > 0) 
    {
      int rc, maxfd;
      fd_set fdread, fdwrite, fdexcep;

      FD_ZERO(&fdread); FD_ZERO(&fdwrite); FD_ZERO(&fdexcep);
      timeout.tv_sec = 0;
      timeout.tv_usec = 250000;

      max_timeout_msec -= timeout.tv_sec*1000 + timeout.tv_usec * 0.001;

      curl_multi_fdset(bctx->multiple_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

      //fprintf (stderr, "%s - Waiting for %d clients with seconds %f.\n", 
      //name, still_running, max_timeout);

      rc = select (maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);

      switch(rc)
        {
        case -1: /* select error */
          break;

        case 0: /* timeout */
          now_time = get_tick_count ();
        default: /* timeout or readable/writable sockets */
          mperform_smooth (bctx, &now_time, &still_running);            
          break;
        }
          
      if (! (++cycle_counter % TIME_RECALCULATION_CYCLES_NUM))
        {
          now_time = get_tick_count ();
        }

      dispatch_expired_timers (bctx, now_time);
    } 
  return 0;
}

/****************************************************************************************
 * Function name - mperform_smooth
 *
 * Description - Uses curl_multi_perform () for initial url-fetch and to react 
 *               on socket events. It calls curl_multi_info_read () to test url-fetch 
 *               completion events and to proceed with the next step for a client, 
 *               using load_next_step (). It cares about statistics at certain timeouts.
 *
 * Input -       *bctx          - pointer to the batch of contexts;
 *               *still_running - pointer to counter of still running clients (CURL handles)
 *               
 * Return Code/Output - On Success - 0, on Error -1
 ****************************************************************************************/
static int mperform_smooth (batch_context* bctx, 
                            unsigned long* now_time, 
                            int* still_running)
{
  CURLM *mhandle =  bctx->multiple_handle;
  int cycle_counter = 0;	
  int msg_num = 0;
  const int snapshot_timeout = snapshot_statistics_timeout*1000;
  CURLMsg *msg;
  int sched_now = 0; 
    
  while (CURLM_CALL_MULTI_PERFORM == 
        curl_multi_perform(mhandle, still_running))
    ;

  if ((long)(*now_time - bctx->last_measure) > snapshot_timeout) 
    {
      if (is_batch_group_leader (bctx))
        {
          dump_snapshot_interval (bctx, *now_time);
        }
    }

  while( (msg = curl_multi_info_read (mhandle, &msg_num)) != 0)
    {
      if (msg->msg == CURLMSG_DONE)
        {
          /* 
	   * TODO: CURLMsg returns 'result' field as curl return code. 
	   * We may wish to use it. 
	   */

          CURL *handle = msg->easy_handle;
          client_context *cctx = NULL;

          curl_easy_getinfo (handle, CURLINFO_PRIVATE, &cctx);

          if (!cctx)
            {
              fprintf (stderr, "%s - error: cctx is a NULL pointer.\n", __func__);
              return -1;
            }

          if (msg->data.result)
            {
              // fprintf (stderr, "res is %d ", msg->data.result);
              cctx->client_state = CSTATE_ERROR;
                
              // fprintf(cctx->file_output, "%ld %s !! ERROR: %d - %s\n", cctx->cycle_num, 
              // cctx->client_name, msg->data.result, curl_easy_strerror(msg->data.result ));
            }

          if (! (++cycle_counter % TIME_RECALCULATION_MSG_NUM))
            {
              *now_time = get_tick_count ();
            }

            /*
              Load next step only if request rate is not specified.
              Otherwise requests are made on a timer.
            */
          if (bctx->req_rate)
            {
              if (put_free_client(cctx) < 0)
                {
                  fprintf (stderr, "%s error: cannot free a client.\n",
                   __func__);
                  return -1;
                }
            }
          else
            {
              /*cstate client_state =  */
              load_next_step (cctx, *now_time, &sched_now);

              //fprintf (stderr, "%s - after load_next_step client state %d.\n", __func__, client_state);
            }

          if (msg_num <= 0)
            {
              break;  /* If no messages left in the queue - go out */
            }

          cycle_counter++;
        }
    }

  return 0;
}

