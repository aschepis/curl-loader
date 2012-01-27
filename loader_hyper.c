/* 
 *     loader_hyper.c
 *
 * 2006-2007 Copyright (c)
 * Michael Moser, <moser.michael@gmail.com>
 * Robert Iakobashvili, <coroberti@gmail.com>
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
 * Cooked using CURL-project example hypev.c with thanks to the 
 * great CURL-project authors and contributors.
 */

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <event.h>
#include <string.h>

#include "batch.h"
#include "client.h"
#include "loader.h"
#include "conf.h"
#include "cl_alloc.h"
#include "screen.h"


#define TIMER_NEXT_LOAD 20000

static int mget_url_hyper (batch_context* bctx);
static int mperform_hyper (batch_context* bctx, int* still_running);


#if 0
#define PRINTF(args...) fprintf(stdout, ## args);
#else
#define PRINTF(args...)
#endif



typedef struct sock_info
{
  curl_socket_t sockfd;

  int action;  /*CURL_POLL_IN CURL_POLL_OUT */

  long timeout;

  struct event ev;

  int evset;

} sock_info;


int still_running;

static void event_cb_hyper (int fd, short kind, void *userp);
static void update_timeout_hyper (batch_context *bctx);

static int on_exit_hyper (batch_context* bctx);


//static struct event timer_event;
//static struct event timer_next_load_event;


/************************************************************************
 * Function name - event_cb_hyper
 *
 * Description - A libevent callback. Called by libevent when we get action on a socket.
 * Input -       fd - descriptor (socket)
 *                   kind -  a bitmask of events from libevent
 *                   *userp - user pointer, we pass pointer to batch-context structure
 * Return Code/Output - None
 *************************************************************************/
static void event_cb_hyper (int fd, short kind, void *userp)
{
  batch_context *bctx = (batch_context *) userp;
  int st;
  CURLMcode rc;

  int bitset = 0;
  
  if (kind & EV_READ) 
    {
      bitset |= CURL_CSELECT_IN;
    }
  if (kind & EV_WRITE) 
    {
      bitset |= CURL_CSELECT_OUT;
    }
  
  PRINTF("event_cb_hyper enter\n");
  
  /* 
     Tell libcurl to deal with the transfer associated with this socket 
  */
  do 
    {
      rc = curl_multi_socket_action (bctx->multiple_handle, fd, bitset, &st);
      //rc = curl_multi_socket (bctx->multiple_handle, fd, &st);
    } 
  while (rc == CURLM_CALL_MULTI_PERFORM);

  if(st) 
    {
      update_timeout_hyper(bctx);
    } 
  else 
    {
#if 0
      PRINTF("last transfer done, kill timeout\n");
      if (evtimer_pending(&timer_event, NULL)) {
        evtimer_del(&timer_event);
      }
#endif
    }
  PRINTF("event_cb_hyper exit\n");
}

/************************************************************************
 * Function name - timer_cb_hyper
 *
 * Description - A libevent callback. Called by libevent when timeout expires
 * Input -       fd - descriptor (socket)
 *                   kind -  a bitmask of events from libevent
 *                   *userp - user pointer, we pass pointer to batch-context structure
 * Return Code/Output - None
 *************************************************************************/
static void timer_cb_hyper (int fd, short kind, void *userp)
{
  (void)fd;
  (void)kind;
  batch_context *bctx = (batch_context *)userp;
  CURLMcode rc;
  int st;

  //PRINTF("timer_cb_hyper enter\n");

  do 
    {
      rc = curl_multi_socket(bctx->multiple_handle, CURL_SOCKET_TIMEOUT, &st);
    } 
  while (rc == CURLM_CALL_MULTI_PERFORM);
    
  if (still_running ) 
    { 
      update_timeout_hyper(bctx); 
    }

  //PRINTF("timer_cb_hyper exit\n");
}

/************************************************************************
 * Function name - remsock
 *
 * Description - Clean up the sock_info structure
 * Input -      *sinfo - pointer to sinfo structure 
 * Return Code/Output - None
 *************************************************************************/
static void remsock(sock_info *sinfo)
{
  PRINTF("remsock- enter\n");

  if (!sinfo) 
    { 
      return; 
    }

  if (sinfo->evset) 
    { 
      event_del(&sinfo->ev); 
    }
  sinfo->evset = 0;
}

/************************************************************************
 * Function name - setsock_hyper
 *
 * Description - Assign information to a sock_info structure
 * Input -      *sinfo - pointer to sinfo structure to fill
 *               socket - socket descriptor
 *               *handle -  pointer to CURL library handle
 *               action - bitmask of events from curl library
 *               *bctx - pointer to batch context
 * Return Code/Output - None
 *************************************************************************/
static void setsock_hyper(sock_info*sinfo, 
                    curl_socket_t socket, 
                    CURL* handle, 
                    int action,
                    batch_context* bctx)
{
  int kind = 
    (action & CURL_POLL_IN ? EV_READ : 0)|
    (action & CURL_POLL_OUT ? EV_WRITE : 0)|
    EV_PERSIST;

  (void) handle;

  PRINTF("setsock_hyper- enter\n");

  sinfo->sockfd = socket;
  sinfo->action = action;

  if (sinfo->evset) 
    { 
      event_del(&sinfo->ev);
    }

  event_set( &sinfo->ev, sinfo->sockfd, kind, event_cb_hyper, bctx);
  event_base_set(bctx->eb, &sinfo->ev);
  
  sinfo->evset=1;

  event_add(&sinfo->ev, NULL);
}

/************************************************************************
 * Function name - addsock_hyper
 *
 * Description - Initialize a new sock_info structure
 *
 * Input -   socket - socket descriptor   
 *               *handle -  pointer to CURL library handle
 *               action - bitmask of events from curl library
 *               *cctx - pointer to client context
 *               *sinfo - pointer to sinfo structure to fill
 *               *bctx - pointer to batch context
 * Return Code/Output - None
 *************************************************************************/
static void addsock_hyper(curl_socket_t socket, 
                    CURL *handle, 
                    int action,
                    client_context *cctx, 
                    sock_info *sinfo, 
                    batch_context *bctx) 
{
  PRINTF("addsock_hyper - enter\n");

  setsock_hyper (sinfo, socket, handle, action, bctx);

  curl_multi_assign (bctx->multiple_handle, socket, cctx);
}

#if 0
static char *whatstr[]={ "none", "IN", "OUT", "INOUT", "REMOVE" };
#endif

/************************************************************************
 * Function name - socket_callback
 *
 * Description - A libcurl socket callback. Called by libcurl, when there is an
 *                    event on socket. Unfortunately, it is called only for the trasfer stage.
 *
 * Input -       *handle - pointer to CURL handle
 *                   socket - socket descriptor
 *                  what - libcurl event bitmask
 *                  *cbp - libcurl callback pointer; we pass batch context here
 *                   *sockp - pointer to the handle user-assigned private data, here client-context 
 * Return Code/Output - On Success - 0, on Error -1
 *************************************************************************/
static int socket_callback (CURL *handle, 
                    curl_socket_t socket, 
                    int what, 
                    void *cbp, 
                    void *sockp)
{
  batch_context* bctx = (batch_context *) cbp;
  client_context* cctx = (client_context *) sockp;
  sock_info *sinfo = (sock_info*) cctx ? cctx->ext_data : 0;

  PRINTF("socket_callback - enter\n");

  if (!cctx)
    {
      curl_easy_getinfo (handle, CURLINFO_PRIVATE, &cctx);

      if (cctx) 
        {
          sinfo = (sock_info*)cctx->ext_data;
        }
    }

  if (!cctx) 
    {
      PRINTF("no client context\n");
      return 0;
    }

  if (!sinfo) 
    {
      PRINTF("no extension data\n");
      return 0;
    }
  
  PRINTF("socket callback: ctx=%p sock=%d what=%s\n", cctx, socket , whatstr[what]);

   
  if (what == CURL_POLL_REMOVE) 
    {
      PRINTF("\n");
      remsock(sinfo);
    } 
  else 
    {
      if (!sinfo->evset) 
        {
          PRINTF("Adding data: %s%s\n",
                 what & CURL_POLL_IN ? "READ":"",
                 what & CURL_POLL_OUT ? "WRITE":"" );

          addsock_hyper (socket, handle, what, cctx, sinfo, bctx);
        }
      else 
        {
          PRINTF("Changing action from %d to %d\n", sinfo->action, what);
          setsock_hyper(sinfo, socket, handle, what, bctx);
        }
    }
  return 0;
}

/****************************************************************************************
 * Function name - update_timeout_hyper
 *
 * Description - Unclear, whether we need it? Update the event timer 
 *                     after curl_multi library calls?
 *
 * Input -       *bctx - pointer to the batch of contexts
 * Return Code/Output - None
 ****************************************************************************************/
static void update_timeout_hyper (batch_context *bctx)
{
    (void) bctx;
#if 1
  long timeout_ms;
  struct timeval timeout;

  //PRINTF("update_timeout_hyper - enter\n");

  curl_multi_timeout (bctx->multiple_handle, &timeout_ms);

  if (timeout_ms < 0) 
    {
      return;
    }

  timeout.tv_sec = timeout_ms/1000;
  timeout.tv_usec = (timeout_ms%1000)*1000;
  evtimer_add (bctx->timer_event, &timeout);
#endif
}


/************************************************************************
 * Function name - next_load_cb_hyper
 *
 * Description - Called on timer. Makes next load actions and dispatches
 *                  expired timers on the waiting queue.
 *
 * Input -   fd - socket descriptor   
 *               kind - bitmask of events from libevent
 *               *userp - libevent passed pointer to user data; here is batch context
 * Return Code/Output - None
 *************************************************************************/
static void next_load_cb_hyper (int fd, short kind, void *userp)
{
  (void)fd;
  (void)kind;
  batch_context *bctx = (batch_context *)userp;
  int st;

  PRINTF("next_load_cb_hyper\n");
  
  /* 
     1. Checks completion of operations and goes to the next step;
     2. Dispatches expired timers, adds clients from waiting queue to multihandle;
     3. Runs multi_socket_all () to open sockets, call event_cb_hyper  and add 
         their the sockets to the epoll.
  */
  mperform_hyper (bctx, &st);

  struct timeval tv;
  timerclear(&tv);
  tv.tv_usec = TIMER_NEXT_LOAD;
  
  event_add (bctx->timer_next_load_event, &tv);  
}

/****************************************************************************************
 * Function name - user_activity_hyper
 *
 * Description - Simulates user-activities using HYPER mode
 * Input -       *cctx_array - array of client contexts (related to a certain batch of clients)
 *
 * Return Code/Output - On Success - 0, on Error -1
 ****************************************************************************************/
int user_activity_hyper (client_context* cctx_array)
{
  batch_context* bctx = cctx_array->bctx;
  sock_info *sinfo;
  int k, st;

  if (!bctx)
    {
      fprintf (stderr, "%s - error: bctx is a NULL pointer.\n", __func__);
      return -1;
    }

  /* Init libevent library */
  bctx->eb = event_init ();

  if (! (bctx->timer_event = cl_calloc (sizeof (struct event), 1)))
  {
      fprintf (stderr, "%s - error: timer_event allocation faled.\n", __func__);
      return -1;
  }

  if (! (bctx->timer_next_load_event = cl_calloc (sizeof (struct event), 1)))
  {
      fprintf (stderr, "%s - error: timer_next_load_event allocation faled.\n", __func__);
      return -1;
  }

  /* Set the socket callback on multi-handle */ 
  curl_multi_setopt (bctx->multiple_handle, 
                    CURLMOPT_SOCKETFUNCTION, 
                    socket_callback);

  curl_multi_setopt (bctx->multiple_handle, CURLMOPT_SOCKETDATA, bctx);


  still_running = 1; 
 
  for (k = 0 ; k < bctx->client_num_max ; k++)
    {
      sinfo = cl_calloc (1, sizeof (sock_info));
      if (!sinfo)
        {
           fprintf (stderr, "%s - error: allocation of sock_info failed.\n", __func__);
          return -1;
        }

      bctx->cctx_array[k].ext_data = sinfo;
    }

  if (alloc_init_timer_waiting_queue (
                                      bctx->client_num_max + PERIODIC_TIMERS_NUMBER + 1,
                                      &bctx->waiting_queue) == -1)
    {
      fprintf (stderr, "%s - error: failed to alloc or init timer waiting queue.\n", __func__);
      return -1;
    }

  const unsigned long now_time = get_tick_count ();
  
  if (init_timers_and_add_initial_clients_to_load (bctx,
                                                   now_time) == -1)
    {
      fprintf (stderr, 
               "%s - error: init_timers_and_add_initial_clients_to_load () failed.\n", 
               __func__);
      return -1;
    }

  evtimer_set (bctx->timer_event, timer_cb_hyper, bctx);
  event_base_set(bctx->eb, bctx->timer_event);

  while (CURLM_CALL_MULTI_PERFORM == 
         curl_multi_socket_all(bctx->multiple_handle, &st))
         ;

  evtimer_set (bctx->timer_next_load_event, next_load_cb_hyper, bctx);
  event_base_set(bctx->eb, bctx->timer_next_load_event);
  
  struct timeval tv;
  timerclear(&tv);
  tv.tv_usec = TIMER_NEXT_LOAD;	
  event_add (bctx->timer_next_load_event, &tv);

  if (is_batch_group_leader (bctx))
    {
      dump_snapshot_interval (bctx, now_time);
    }

  /* 
     ========= Run the loading machinery ================
  */
  if (mget_url_hyper (bctx) == -1)
  {
      fprintf (stderr, "%s error: mget_url () failed.\n", __func__) ;
      return -1;
  }


  on_exit_hyper (bctx);

  return 0;
}

static int on_exit_hyper (batch_context* bctx)
{
    //fprintf (stderr, "%s - entered.\n", __func__);

    still_running =0;

  dump_final_statistics (bctx->cctx_array);
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

  exit (0);
}


/****************************************************************************************
 * Function name - mget_url_hyper
 *
 * Description - Performs actual fetching of urls for a whole batch. Starts with initial fetch
 *               by mperform_hyper () and further acts using mperform_hyper () on select events
 *
 * Input -       *bctx - pointer to the batch of contexts
 *
 * Return Code/Output - On Success - 0, on Error -1
 ****************************************************************************************/
static int mget_url_hyper (batch_context* bctx)		       
{
  // float max_timeout = DEFAULT_SMOOTH_URL_COMPLETION_TIME;
  // if (bctx->url_ctx_array)
  //  {
  //    max_timeout = bctx->url_ctx_array[0].timer_url_completion;
  //  }
 
  /* update timeout */
  update_timeout_hyper (bctx);

  /* Run the event loop */
  //event_dispatch();
  event_base_dispatch((struct event_base *) bctx->eb);

  fprintf (stderr, "%s - out of event_dispatch () loop.\n", __func__);

  return 0;
}

/****************************************************************************************
 * Function name - mperform_hyper
 *
 * Description - Uses curl_multi_perform () for initial url-fetch and to react on socket events.
 *               Uses curl_multi_info_read () to test url-fetch completion events and to proceed
 *               with the next step for the client, using load_next_step (). Cares about statistics
 *               at certain timeouts.
 *
 * Input -       *bctx - pointer to the batch of contexts;
 *               *still_running - pointer to counter of still running clients (CURL handles)
 *               
 * Return Code/Output - On Success - 0, on Error -1
 ****************************************************************************************/
static int mperform_hyper (batch_context* bctx, int* still_running)
{
  CURLM *mhandle =  bctx->multiple_handle;
  int cycle_counter = 0;	
  int msg_num = 0, st;
  const int snapshot_timeout = snapshot_statistics_timeout*1000;
  unsigned long now_time;
  CURLMsg *msg;
  int scheduled_now_count = 0, scheduled_now = 0;

  (void)still_running;

  if (pending_active_and_waiting_clients_num (bctx) == 0 &&
      bctx->do_client_num_gradual_increase == 0)
  {
      return on_exit_hyper (bctx);
  }
    
  now_time = get_tick_count ();

  if ((long)(now_time - bctx->last_measure) > snapshot_timeout) 
    {
      if (is_batch_group_leader (bctx))
        {
          dump_snapshot_interval (bctx, now_time);
        }
    }

  while( (msg = curl_multi_info_read (mhandle, &msg_num)) != 0)
    {
      if (msg->msg == CURLMSG_DONE)
        {
          /* TODO: CURLMsg returns 'result' field as curl return code. We may wish to use it. */

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
              now_time = get_tick_count ();
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
              load_next_step (cctx, now_time, &scheduled_now);

             if (scheduled_now)
               {
                 scheduled_now_count++;
               }

             //fprintf (stderr, "%s - after load_next_step client state %d.\n", __func__, client_state);
            }

          if (msg_num <= 0)
            {
              break;  /* If no messages left in the queue - go out */
            }

          cycle_counter++;
        }
    }

  if (dispatch_expired_timers (bctx, now_time) > 0 || scheduled_now_count)
    {
      while (CURLM_CALL_MULTI_PERFORM == 
             curl_multi_socket_all (bctx->multiple_handle, &st))
          ;
    }

  return 0;
}

