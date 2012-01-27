/* 
 *     loader.c
 *
 * 2006-2012 Copyright (c) 
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

// must be the first include
#include "fdsetsize.h"

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>

#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <curl/curl.h>
#include <curl/multi.h>

// getrlimit
#include <sys/resource.h>

#include "batch.h"
#include "client.h"
#include "loader.h"
#include "conf.h"
#include "ssl_thr_lock.h"
#include "screen.h"
#include "cl_alloc.h"


static int client_tracing_function (CURL *handle, 
                             curl_infotype type, 
                             unsigned char *data, 
                             size_t size, 
                             void *userp);
static size_t do_nothing_write_func (void *ptr, 
                              size_t size, 
                              size_t nmemb, 
                              void *stream);

static int create_ip_addrs (batch_context* bctx, int bctx_num);

static void* batch_function (void *batch_data);
static int initial_handles_init (struct client_context*const cdata);
static int setup_curl_handle_appl (struct client_context*const cctx,  
                                   url_context* url_ctx);
static int init_client_formed_buffer (client_context* cctx, 
                                      url_context* url,
                                      char* buffer,
                                      size_t buffer_len);
static int init_client_contexts (batch_context* bctx, FILE* output_file);
static void free_batch_data_allocations (struct batch_context* bctx);
static void free_url (url_context* url, int clients_max);
static int ipv6_increment(const struct in6_addr *const src, 
                          struct in6_addr *const dest);
static int create_thr_subbatches (batch_context *bc_arr, int subbatches_num);
static int ip_addr_str_allocate_init (batch_context* bctx, 
                                      int client_index, 
                                      char** addr_str);

int stop_loading = 0;


static void sigint_handler (int signum)
{
  (void) signum;

  stop_loading = 1;

  screen_release ();

  close (STDIN_FILENO);

  fprintf (stderr, "\n\n======= SIGINT Received ============.\n");
}

typedef int (*pf_user_activity) (struct client_context*const);

/*
 * Batch functions for the 2 loading modes: 
 * hyper (epoll-based) and smooth (poll-based).
*/
static pf_user_activity ua_array[2] = 
{ 
  user_activity_hyper,
  user_activity_smooth
};

static FILE *create_file (batch_context* bctx, char* fname)
{
  FILE *fp = fopen(fname,"w");
  if (!fp)
    (void)fprintf(stderr,"%s, cannot create file \"%s\", %s\n", 
      bctx->batch_name,fname,strerror(errno));
  return fp;
}

int 
main (int argc, char *argv [])
{
  batch_context bc_arr[BATCHES_MAX_NUM];
  pthread_t tid[BATCHES_MAX_NUM];
  int batches_num = 0; 
  int i = 0, error = 0;


  signal (SIGPIPE, SIG_IGN);

  if (parse_command_line (argc, argv) == -1)
    {
      fprintf (stderr, 
               "%s - error: failed parsing of the command line.\n", __func__);
      return -1;
    }

  if (geteuid())
    {
      fprintf (stderr, 
               "%s - error: lacking root priviledges to run this program.\n", __func__);
      return -1;
    }
  
   memset(bc_arr, 0, sizeof(bc_arr));

  /* 
     Parse the configuration file. 
  */
  if ((batches_num = parse_config_file (config_file, bc_arr, 
                                        sizeof(bc_arr)/sizeof(*bc_arr))) <= 0)
    {
      fprintf (stderr, "%s - error: parse_config_file () failed.\n", __func__);
      return -1;
    }

   /*
    * De-facto the support is only for a single batch. However, we are using 
    * internal support for multiple batches for loading from several threads, 
    * using sub-batches (a subset of virtual clients).
    * TODO: test env for all batches.
    */
   if (test_environment (&bc_arr[0]) == -1)
   {
       fprintf (stderr, "%s - error: test_environment () - error.\n", __func__);
      return -1;
   }
   
  /* 
     Add ip-addresses to the loading network interfaces
     and keep them in batch-contexts. 
  */
  if (create_ip_addrs (bc_arr, batches_num) == -1)
    {
      fprintf (stderr, "%s - error: create_ip_addrs () failed. \n", __func__);
      return -1;
    }
  else
    {
      fprintf (stderr, 
               "%s - added IP-addresses to the loading network interface.\n", 
               __func__);
    }

  signal (SIGINT, sigint_handler);

  screen_init ();
  
  if (! threads_subbatches_num)
    {
      fprintf (stderr, "\nRUNNING LOAD\n\n");
      sleep (1);
      batch_function (&bc_arr[0]);
      fprintf (stderr, "Exited batch_function\n");
      screen_release ();
    }
  else
    {
      fprintf (stderr, "\n%s - RUNNING LOAD, STARTING THREADS\n\n", __func__);
      sleep (1);
      
      /* Init openssl mutexes and pass two callbacks to openssl. */
      if (thread_openssl_setup () == -1)
        {
          fprintf (stderr, "%s - error: thread_setup () - failed.\n", __func__);
          return -1;
        }

      create_thr_subbatches (bc_arr, threads_subbatches_num); 
      
      /* 
         Opening threads for the batches of clients 
      */
      for (i = 0 ; i < threads_subbatches_num ; i++) 
        {
          bc_arr[i].batch_id = i;
          error = pthread_create (&tid[i], NULL, batch_function, &bc_arr[i]);
          

          if (0 != error)
            {
            fprintf(stderr, "%s - error: Couldn't run thread number %d, errno %d\n", 
                    __func__, i, errno);
            }
          else 
            {
              bc_arr[i].thread_id = tid[i]; /* Set the thread-id */

              fprintf(stderr, "%s - note: Thread %d, started normally\n", __func__, i);
            }
        }

      /* Waiting for all running threads to terminate */
      for (i = 0 ; i < threads_subbatches_num ; i++) 
        {
          error = pthread_join (tid[i], NULL) ;
          fprintf(stderr, "%s - note: Thread %d terminated normally\n", __func__, i) ;
        }

      thread_openssl_cleanup ();
    }
   
  return 0;
}

/****************************************************************************************
* Function name - batch_function
* Description -   Runs the batch test either within the main-thread or in a separate thread.
*
* Input -         *batch_data - contains loading configuration and active entities for a 
*                               particular batch of clients.
* Return Code/Output - NULL in all cases
****************************************************************************************/
static void* batch_function (void * batch_data)
{
  batch_context* bctx = (batch_context *) batch_data;
  FILE* log_file = 0;
  FILE* statistics_file = 0;
  FILE* opstats_file = 0;
  
  int  rval = -1;

  if (!bctx)
    {
      fprintf (stderr, 
               "%s - error: batch_data input is zero.\n", __func__);
      return NULL;
    }

  if (! stderr_print_client_msg)
    {
      /*
        Init batch logfile for the batch client output 
      */
      (void)sprintf (bctx-> batch_logfile, "./%s.log", bctx->batch_name);
      if (!(log_file = create_file(bctx,bctx->batch_logfile)))
          return NULL;
      else
        {
          char tbuf[256];
          (void)fprintf(log_file,"# %ld %s",get_tick_count(),ascii_time(tbuf));
	  (void)fprintf(log_file,
            "# msec_offset cycle_no url_no client_no (ip) indic info\n");
        }
    }

  /*
    Init batch statistics file
  */
  (void)sprintf (bctx->batch_statistics, "./%s.txt", bctx->batch_name);
  if (!(bctx->statistics_file = statistics_file = create_file(bctx,
    bctx->batch_statistics)))
      return NULL;
  else
      print_statistics_header (statistics_file);
  
  /*
    Init batch operational statistics file
  */
  if (bctx->dump_opstats) 
    {
      (void)sprintf (bctx->batch_opstats, "./%s.ops", bctx->batch_name);
      if (!(bctx->opstats_file = opstats_file = create_file(bctx,
       bctx->batch_opstats)))
          return NULL;
    }
  
  /* 
     Init the objects, containing client-context information.
  */
  if (init_client_contexts (bctx, log_file) == -1)
    {
      fprintf (stderr, "%s - \"%s\" - failed to allocate or init client_contexts.\n", 
               __func__, bctx->batch_name);
      goto cleanup;
    }
 
  /* 
     Init libcurl MCURL and CURL handles. Setup of the handles is delayed to
     the later step, depending on urls required.
  */
  if (initial_handles_init (bctx->cctx_array) == -1)
    {
      fprintf (stderr, "%s - \"%s\" initial_handles_init () failed.\n", 
               __func__, bctx->batch_name);
      goto cleanup;
    }

  /* 
     Now run configuration-defined actions, like login, fetching various urls and and 
     sleeping in between and loggoff.

     It calls user activity loading function corresponding to the used loading mode
     (user_activity_smooth () or user_activity_hyper ()).
  */ 
  rval = ua_array[loading_mode] (bctx->cctx_array);

  if (rval == -1)
    {
      fprintf (stderr, "%s - \"%s\" -user activity failed.\n", 
               __func__, bctx->batch_name);
      goto cleanup;
    }

 cleanup:
  if (bctx->multiple_handle)
    curl_multi_cleanup(bctx->multiple_handle);

  if (log_file)
      fclose (log_file);

  if (statistics_file)
      fclose (statistics_file);

  if (opstats_file)
      fclose (opstats_file);

  free_batch_data_allocations (bctx);

  return NULL;
}

/****************************************************************************************
* Function name - initial_handles_init
*
* Description - Libcurl initialization of curl multi-handle and the curl handles (clients), 
*               used in the batch
*
* Input -       *ctx_array - array of clients for a particular batch/sub-batch of clients
* Return Code/Output - On Success - 0, on Error -1
****************************************************************************************/
static int initial_handles_init (client_context*const ctx_array)
{
  batch_context* bctx = ctx_array->bctx;
  int k = 0;

  /* Init CURL multi-handle. */
  if (! (bctx->multiple_handle = curl_multi_init()) )
    {
      fprintf (stderr, 
               "%s - error: curl_multi_init() failed for batch \"%s\" .\n", 
               __func__, bctx->batch_name) ;
      return -1;
    }

  /* Initialize all CURL handles */
  for (k = 0 ; k < bctx->client_num_max ; k++)
    {
      if (!(bctx->cctx_array[k].handle = curl_easy_init ()))
        {
          fprintf (stderr,"%s - error: curl_easy_init () failed for k=%d.\n",
                   __func__, k);
          return -1;
        }
    }
        
  return 0;
}

/*
  The callback to libcurl to write all bytes to ptr.
*/
size_t writefunction( void *ptr, size_t size, size_t nmemb, void *stream)
{
  fwrite (ptr, size, nmemb, stream);
  return(nmemb * size);
}

/*
  The callback to libcurl to skip all body bytes of the fetched urls.
*/
size_t 
do_nothing_write_func (void *ptr, size_t size, size_t nmemb, void *stream)
{
  (void)ptr;
  (void)stream;

  /* 
     Overwriting the default behavior to write body bytes to stdout and 
     just skipping the body bytes without any output. 
  */
  return (size*nmemb);
}

/****************************************************************************************
* Function name - setup_curl_handle
*
* Description - Inits a CURL handle, using setup_curl_handle_init () function.
*
* Input -       *cctx - pointer to client context, containing CURL handle pointer;
*               *url  - pointer to url-context, containing all url-related information;
* Return Code/Output - On Success - 0, on Error -1
****************************************************************************************/
int setup_curl_handle (client_context*const cctx, url_context* url)
{
  if (setup_curl_handle_init (cctx, url) == -1)
  {
      fprintf (stderr,"%s - error: failed.\n",__func__);
      return -1;
  }

  return 0;
}

/****************************************************************************
* Function name - setup_curl_handle_init
*
* Description - Resets client context kept CURL handle and inits it locally, using 
*               setup_curl_handle_appl () function for the application-specific 
*               (HTTP/FTP) initialization.
*
* Input -       *cctx- pointer to client context, containing CURL handle pointer;
*               *url - pointer to url-context, containing all url-related information;
* Return Code/Output - On Success - 0, on Error -1
******************************************************************************/
int setup_curl_handle_init (client_context*const cctx, url_context* url)
{

  if (!cctx || !url)
    {
      return -1;
    }

  batch_context* bctx = cctx->bctx;
  CURL* handle = cctx->handle;

  curl_easy_reset (handle);

  /*
   Choose the next URL from an url set, or complete the url template from 
   prior responses, or prepare to scan for new response values.
   This updates the url_str with the appropriate token values, and hands the url to curl
   before any other clients (possibly in other threads) can intervene.
   */
  if (update_url_from_set_or_template (handle, cctx, url) < 0)
  {
      fprintf (stderr,"%s - error: update_url_from_set_or_template failed\n", __func__);
	  return -1;
  }
  
  if (bctx->ipv6)
    curl_easy_setopt (handle, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V6);
      
  /* Bind the handle to a certain IP-address */
  curl_easy_setopt (handle, CURLOPT_INTERFACE, 
                    bctx->ip_addr_array [cctx->client_index]);

  curl_easy_setopt (handle, CURLOPT_NOSIGNAL, 1);

  /* set|unset the curl proxy */
  curl_easy_setopt (handle, CURLOPT_PROXY, config_proxy);
    
  /* Set the url */
  if (url->url_str && url->url_str_len)
    {
      /* 
         Note, target URL for PUT should include directory with a file
         name, not just a directory.
      */

      if ((url->req_type == HTTP_REQ_TYPE_GET  && url->form_str) || 
		  (url->req_type == HTTP_REQ_TYPE_HEAD  && url->form_str) || 
		  (url->req_type == HTTP_REQ_TYPE_DELETE  && url->form_str))
        {
          /* 
             GET url with form fields. If not making searches with a search
             engine, better to do it encrypted by HTTPS.
          */
          if (!cctx->get_url_form_data || !cctx->get_url_form_data_len)
            {
              fprintf (stderr,"%s - error: get_url_form_data not allocated/initialized.\n",
                       __func__);
              return -1;
            }
 
          strcpy (cctx->get_url_form_data, url->url_str);
          
          if (init_client_formed_buffer (cctx, 
                                         url,
                                         cctx->get_url_form_data + url->url_str_len -1,
                                         cctx->get_url_form_data_len - url->url_str_len) == -1)
            {
              fprintf (stderr,
                       "%s - error: init_client_formed_buffer() failed for GET form fields.\n",
                       __func__);
              return -1;
            }
          
          curl_easy_setopt (handle, CURLOPT_URL, cctx->get_url_form_data);
        }
      else
        {
          if (! is_template(url)) /* Handled in update_url_from_set_or_template () above. GF */
          {
#if DEBUG
              // curl_easy_setopt (handle, CURLOPT_URL, url->url_str);
              char buf[1000];
              sprintf(buf,"%s.%ld.%ld.%s",url->url_str,
                  cctx->cycle_num,cctx->url_curr_index,
                  cctx->client_name);
              buf[strlen(buf)-1] = '\0'; // suppress space
              curl_easy_setopt (handle, CURLOPT_URL, buf);
#else
              curl_easy_setopt (handle, CURLOPT_URL, url->url_str);
#endif // DEBUG

          }
        }
    }
  else
    {
      fprintf (stderr,"%s - error: empty url provided.\n", __func__);
      return -1;
    }
  
  // reset the url if URL_RANDOM_RANGE is used, to create a random url for caching
  if ( (url->random_hrange > 0) && (url->random_hrange > url->random_lrange) ) {
      randomize_url(handle, url);
  }

  /* Set the index to client */
  if (url->url_ind >= 0)
  {
    cctx->url_curr_index = url->url_ind;
  }
  
  bctx->url_index = url->url_ind;

  curl_easy_setopt (handle, CURLOPT_DNS_CACHE_TIMEOUT, -1);

  /* Set the connection timeout */
  curl_easy_setopt (handle, 
                    CURLOPT_CONNECTTIMEOUT, 
                    url->connect_timeout ? url->connect_timeout : connect_timeout);

  /* Define the connection re-use policy. When passed 1, re-establish */
  curl_easy_setopt (handle, CURLOPT_FRESH_CONNECT, url->fresh_connect);

  if (url->fresh_connect)
    {
      curl_easy_setopt (handle, CURLOPT_FORBID_REUSE, 1);
    }

  /* 
     If DNS resolving is necesary, global DNS cache is enough,
     otherwise compile libcurl with ares (cares) library support.
     Attention: DNS global cache is not thread-safe, therefore use
     cares for asynchronous DNS lookups.

     curl_easy_setopt (handle, CURLOPT_DNS_USE_GLOBAL_CACHE, 1); 
  */
  
  curl_easy_setopt (handle, CURLOPT_VERBOSE, 1);
  curl_easy_setopt (handle, CURLOPT_DEBUGFUNCTION, 
                    client_tracing_function);

  /* 
     This is to return cctx pointer as the void* userp to the 
     tracing function. 
  */
  curl_easy_setopt (handle, CURLOPT_DEBUGDATA, cctx);

#if 0
  curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, prog_cb);
  curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, cctx);
  if (loading_mode == LOAD_MODE_HYPER)
    {
      curl_easy_setopt (handle, CURLOPT_WRITEDATA, cctx);
    }
#endif
  
  if (url->log_resp_bodies || url->log_resp_headers)
    {
      if (response_logfiles_set (cctx, url) == -1)
        {
          fprintf (stderr,"%s - error: response_logfiles_set () .\n",
                   __func__);
          return -1;
        }
    }
  else
    {
      curl_easy_setopt (handle, CURLOPT_WRITEFUNCTION,
                        do_nothing_write_func);
    }

  curl_easy_setopt (handle, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt (handle, CURLOPT_SSL_VERIFYHOST, 0);
    
  /* Set the private pointer to be used by the smooth-mode. */
  curl_easy_setopt (handle, CURLOPT_PRIVATE, cctx);

  /* Without the buffer set, we do not get any errors in tracing function. */
  curl_easy_setopt (handle, CURLOPT_ERRORBUFFER, bctx->error_buffer);
  
  /* set ignore_content_length 
  */
  if (url->ignore_content_length) {
      curl_easy_setopt (handle, CURLOPT_IGNORE_CONTENT_LENGTH, 1);
  }
  
  #if 0
  if (url->upload_file)
    {
      if (! url->upload_file_ptr)
        {
          if (! (url->upload_file_ptr = fopen (url->upload_file, "rb")))
            {
              fprintf (stderr, 
                       "%s - error: failed to open() %s with errno %d.\n", 
                       __func__, url->upload_file, errno);
              return -1;
            }
        }
      
      /* Enable uploading */
      
      curl_easy_setopt(handle, CURLOPT_UPLOAD, 1);
      
      /* 
         Do we want to use our own read function ? On windows - MUST.
         curl_easy_setopt(handle, CURLOPT_READFUNCTION, read_callback);
      */
      
      /* Now specify which file to upload */
      curl_easy_setopt(handle, CURLOPT_READDATA, 
                       url->upload_file_ptr);
      
      /* Provide the size of the upload */
      curl_easy_setopt(handle, CURLOPT_INFILESIZE, 
                       (long) url->upload_file_size);

      if (url->transfer_limit_rate)
        {
          curl_easy_setopt(handle, CURLOPT_MAX_SEND_SPEED_LARGE,
                           (curl_off_t) url->transfer_limit_rate);
        }
    }
  #endif

  /* GF  */
  if (url->upload_file)
  {
      if (upload_file_stream_init (cctx, url) < 0)
          return -1;
  }
  else
  {
      if (url->transfer_limit_rate)
      {
          curl_easy_setopt(handle, CURLOPT_MAX_RECV_SPEED_LARGE,
                           (curl_off_t) url->transfer_limit_rate);
      }
  }

  /* 
     Application (url) specific setups, like HTTP-specific, FTP-specific, etc. 
  */
  if (setup_curl_handle_appl (cctx, url) == -1)
    {
      fprintf (stderr,
               "%s - error: setup_curl_handle_appl () failed .\n",
               __func__);
      return -1;
    }

  return 0;
}
  
/****************************************************************************************
* Function name - setup_curl_handle_appl
*
* Description - Application/url-type specific setup for a single curl handle (client)
*
* Input -       *cctx- pointer to client context, containing CURL handle pointer;
*               *url - pointer to url-context, containing all url-related information;
* Return Code/Output - On Success - 0, on Error -1
****************************************************************************************/
int setup_curl_handle_appl (client_context*const cctx, url_context* url)
{
    batch_context* bctx = cctx->bctx;
    CURL* handle = cctx->handle;
    
    cctx->is_https = (url->url_appl_type == URL_APPL_HTTPS);
    
    if (url->url_appl_type == URL_APPL_HTTPS ||
        url->url_appl_type == URL_APPL_HTTP)
    {
        
        /* ******** HTTP-SPECIFIC INITIALIZATION ************** */
        
      /* 
         Follow possible HTTP-redirection from header Location of the 
         3xx HTTP responses, like 301, 302, 307, etc. It also updates the url, 
         thus no need to parse header Location. Great job done by the libcurl 
         people.
      */
      curl_easy_setopt (handle, CURLOPT_FOLLOWLOCATION, 1);
      curl_easy_setopt (handle, CURLOPT_UNRESTRICTED_AUTH, 1);
      
      /* Enable infinitive (-1) redirection number. */
      curl_easy_setopt (handle, CURLOPT_MAXREDIRS, -1);

      /* 
         Setup the User-Agent header, configured by user. The default is MSIE-6 header.
       */
      curl_easy_setopt (handle, CURLOPT_USERAGENT, bctx->user_agent);

      /*
        Setup the custom (HTTP) headers, if appropriate.
      */
      if (url->custom_http_hdrs && url->custom_http_hdrs_num)
        {
          curl_easy_setopt (handle, CURLOPT_HTTPHEADER, 
                            url->custom_http_hdrs);
        }
      
      /* 
         Enable cookies. This is important for various authentication schemes. 
      */
      if (! url->url_ind)
        {
          curl_easy_setopt (handle, CURLOPT_COOKIEFILE, "");
        }
      
      if (url->req_type == HTTP_REQ_TYPE_POST)
        {
          /* 
             Make POST, using post buffer, if requested. 
          */
            if (url->upload_file && url->upload_file_ptr && (!cctx->post_data || !cctx->post_data[0]))
            {
                curl_easy_setopt(handle, CURLOPT_POST, 1);
            }
            else if (cctx->post_data || url->mpart_form_post)
            {
              /* 
                 Sets POST as the HTTP request method using either:
                 - POST-fields;
                 - multipart form-data as in RFC 1867;
              */
              if (init_client_url_post_data (cctx, url) == -1)
                {
                  fprintf (stderr,
                           "%s - error: init_client_url_post_data() failed.\n",
                           __func__);
                  return -1;
                }
            }
            else
            {
              fprintf (stderr, "%s - error: post_data is NULL.\n", __func__);
              return -1;
            }
        }
      else if (url->req_type == HTTP_REQ_TYPE_PUT)
        {
          if (!url->upload_file || ! url->upload_file_ptr)
            {            
              fprintf (stderr, 
                       "%s - error: upload file is NULL or cannot be opened.\n", 
                       __func__);
              return -1;
            }
          else
            {
              // Upload is enabled earlier.

              /* 
                 HTTP PUT method.
                 Note, target URL for PUT should include a file
                 name, not only a directory 
              */
              curl_easy_setopt(handle, CURLOPT_PUT, 1);
            }
        }
      else if (url->req_type == HTTP_REQ_TYPE_HEAD)
		{
			  /* 
				 HTTP HEAD method.
				 Note, no other info need to put
			 */
			 curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "HEAD");
		}
	  else if (url->req_type == HTTP_REQ_TYPE_DELETE)
		{
			 /* 
				HTTP DELETE method.
				Note, no toher info need to put 
			*/
			curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, "DELETE");
		}

      if (url->web_auth_method)
        {
          if (!url->web_auth_credentials)
            {
              if (!url->username || !url->password)
                {
                  return -1;
                }

              char web_userpwd[256];
              sprintf (web_userpwd, "%s:%s", url->username, url->password);
              curl_easy_setopt(handle, CURLOPT_USERPWD, web_userpwd);
            }
          else
            {
              curl_easy_setopt(handle, CURLOPT_USERPWD, url->web_auth_credentials);
            }
          curl_easy_setopt(handle, CURLOPT_HTTPAUTH, url->web_auth_method);
        }

      if (url->proxy_auth_method)
        {
          if (!url->proxy_auth_credentials)
            {
              if (!url->username || !url->password)
                {
                  return -1;
                }

              char proxy_userpwd[256];
              sprintf (proxy_userpwd, "%s:%s", url->username, url->password);
              curl_easy_setopt(handle, CURLOPT_PROXYUSERPWD, proxy_userpwd);
            }
          else
            {
              curl_easy_setopt(handle, CURLOPT_PROXYUSERPWD, url->proxy_auth_credentials);
            }
          curl_easy_setopt(handle, CURLOPT_PROXYAUTH, url->proxy_auth_method);

        }

    }
    else if (url->url_appl_type == URL_APPL_FTP ||
             url->url_appl_type == URL_APPL_FTPS)
    {

      /***********  FTP-SPECIFIC INITIALIZATION. *****************/

      if (url->ftp_active)
        {
          curl_easy_setopt(handle, 
                           CURLOPT_FTPPORT, 
                           bctx->ip_addr_array [cctx->client_index]);
        }

      /*
        Send custom FTP headers after the transfer.
      */
      if (url->custom_http_hdrs && url->custom_http_hdrs_num)
        {
          curl_easy_setopt (handle, CURLOPT_POSTQUOTE, 
                            url->custom_http_hdrs);
        }
    }

  return 0;
}

/**********************************************************************
* Function name - response_logfiles_set
*
* Description - Opens a logfile for responses to be used for a certain client
*               and a certain url. A separate file to be opened for headers 
*               and bodies. Sets the files to the logging mechanism of
*               libcurl.
* 
* Input -       *cctx - pointer to client context
*               *url  - pointer to url context
* Return Code/Output - On Success - 0, on Error -1
***********************************************************************/
int response_logfiles_set (client_context* cctx, url_context* url)
{
  CURL* handle = cctx->handle;

  if (url->log_resp_bodies && url->dir_log)
    {
      // open the file
      char body_file[256];
      memset (body_file, 0, sizeof (body_file));

      snprintf (body_file, sizeof (body_file) -1, 
                "%s/cl-%Zu-cycle-%ld.body",
                url->dir_log,
                cctx->client_index,
                cctx->cycle_num
                );

      if (cctx->logfile_bodies)
        {
          fclose (cctx->logfile_bodies);
          cctx->logfile_bodies = NULL;
        }

      if (!(cctx->logfile_bodies = fopen (body_file, "w")))
        {
          fprintf (stderr, "%s - error: fopen () failed with errno %d.\n",
                   __func__, errno);
          return -1;
        }
      
      curl_easy_setopt (handle, CURLOPT_WRITEDATA, cctx->logfile_bodies);
      curl_easy_setopt (handle, CURLOPT_WRITEFUNCTION, writefunction);
    }

  if (url->log_resp_headers && url->dir_log)
    {
      // open the file
      char hdr_file[256];
      memset (hdr_file, 0, sizeof (hdr_file));

      snprintf (hdr_file, sizeof (hdr_file) -1, 
                "%s/cl-%Zu-cycle-%ld.hdr",
                url->dir_log,
                cctx->client_index,
                cctx->cycle_num
                );

      if (cctx->logfile_headers)
        {
          fclose (cctx->logfile_headers);
          cctx->logfile_headers = NULL;
        }

      if (!(cctx->logfile_headers = fopen (hdr_file, "w")))
        {
          fprintf (stderr, "%s - error: fopen () failed with errno %d.\n",
                   __func__, errno);
          return -1;
        }
       curl_easy_setopt (handle, CURLOPT_WRITEHEADER, cctx->logfile_headers);
       curl_easy_setopt (handle, CURLOPT_HEADERFUNCTION, writefunction);
    }
  return 0;
}

/**********************************************************************
* Function name - init_client_url_post_data
*
* Description - Initialize client post form buffer to be used for a url POST-ing
* 
* Input -       *cctx - pointer to client context
*               *url - pointer to url context
* Return Code/Output - On Success - 0, on Error -1
***********************************************************************/
int init_client_url_post_data (client_context* cctx, url_context* url)
{
  if (url->form_str)
    {
      if (init_client_formed_buffer (cctx, 
                                     url,
                                     cctx->post_data, 
                                     cctx->post_data_len) == -1)
        {
          fprintf (stderr, "%s - error: init_client_formed_buffers() failed.\n",
                   __func__);
          return -1;
        }
      
      curl_easy_setopt (cctx->handle, CURLOPT_POSTFIELDS, cctx->post_data);
    }
  else if (url->mpart_form_post)
    {
      curl_easy_setopt (cctx->handle, CURLOPT_HTTPPOST, url->mpart_form_post);
    }
  else
    {
      fprintf (stderr, 
               "%s - error: neither url->form_str (POSTFIELDS) nor "
               "url->mpart_form_post (multipart form data, RFC1867) are initialized.\n",
               __func__);
      return -1;
    }

  return 0;
}

/***********************************************************************
* Function name - init_client_formed_buffer
*
* Description - Initialize client buffer controlled by FORM_STRING. The buffers
*                to be used for POST-ing credentials/tokens or for GET passed tokens.
* 
* Input -       *cctx       - pointer to client context
*               *url        - pointer to url context
*               *buffer     - the client buffer
*               *buffer_len - size of the client buffer
* Return Code/Output - On Success - 0, on Error -1
*************************************************************************/
static int init_client_formed_buffer (client_context* cctx, 
                                      url_context* url,
                                      char* buffer,
                                      size_t buffer_len)
{
  int i = 0;

  if (!url->form_str || !url->form_str[0])
    {
 
      fprintf (stderr, "%s - error: FORM_STRING not defined.\n",
               __func__);
      return -1;
    }

  if (!buffer || !buffer_len)
    {
      fprintf (stderr, "%s - error: client used formed buffer is NULL or zero length.\n",
               __func__);
      return -1;
    }

  switch (url->form_usage_type)
    {
    case FORM_USAGETYPE_UNIQUE_USERS_AND_PASSWORDS:
      /*
        For each client init post buffer, containing username and password 
        with uniqueness added via added to the base username and password
        client index.
      */
      snprintf (buffer,
                buffer_len,
                url->form_str,
                url->username,
                i + 1,
                url->password[0] ? url->password : "",
                i + 1);
      break;

    case FORM_USAGETYPE_UNIQUE_USERS_SAME_PASSWORD:
      /* 
         For each client init post buffer, containing username with uniqueness 
         added via added to the base username client index. Password is kept
         the same for all users.
      */
      snprintf (buffer,
                buffer_len,
                url->form_str,
                url->username,
                i + 1,
                url->password[0] ? url->password : "");
      break;

    case FORM_USAGETYPE_SINGLE_USER:
      /* All clients have the same login_username and password.*/
      snprintf (buffer,
                buffer_len,
                url->form_str,
                url->username,
                url->password[0] ? url->password : "");
      break;

    case FORM_USAGETYPE_RECORDS_FROM_FILE:
      {
        if (! url->form_records_array)
          {
            fprintf (stderr,
                     "\"%s\" error: url->form_records_array is NULL.\n", 
                     __func__);
            return -1;
          }

        size_t record_index;

        if (url->form_records_random)
        {
            record_index = (size_t) (url->form_records_num * get_random());
        }
        else if (url->form_records_cycle) /* Added by GF */
        {
            if (++url->form_records_index >= url->form_records_num)
            	url->form_records_index = 0;
            record_index = url->form_records_index;
        }
        else
        {
            record_index = cctx->client_index;
        }

        const form_records_cdata*const fcd = &url->form_records_array[record_index];
        
        snprintf (buffer,
                  buffer_len,
                  url->form_str,
                  fcd->form_tokens[0], 
                  fcd->form_tokens[1] ? fcd->form_tokens[1] : "");
      }
      break;

    case FORM_USAGETYPE_AS_IS:
      strncpy (buffer, url->form_str, buffer_len);
      break;

    default:
      {
        fprintf (stderr,
                 "\"%s\" error: none-valid url->form_usage_type.\n", 
                 __func__);
        return -1;
      }
    }
  
  return 0;
}

#define write_log(ind, data) \
  if (1) {\
    char *end = data+strlen(data)-1;\
    if (*end == '\n')\
      *end = '\0';\
    (void)fprintf(cctx->file_output,"%ld %ld %ld %s%s %s",\
     offs_resp, cctx->cycle_num, cctx->url_curr_index, cctx->client_name,\
     ind, data);\
    if (url_print)\
      (void)fprintf(cctx->file_output," eff-url: url %s",url);\
    if (url_diff)\
      (void)fprintf(cctx->file_output," url: url %s",url_target);\
    (void)fprintf(cctx->file_output,"\n");\
  }

#define write_log_num(ind, num) \
  if (1) {\
    char buf[100];\
    (void)snprintf(buf,sizeof(buf),"%ld",num);\
    write_log(ind,buf);\
  }

#define write_log_ext(ind, num, ext) \
  if (1) {\
    char buf[200];\
    (void)snprintf(buf,sizeof(buf),"%ld %s",num, ext);\
    write_log(ind,buf);\
  }

#define startswith(str, start) !strncmp((char *)str,(char *)start,strlen((char *)start))

/****************************************************************************************
* Function name - client_tracing_function
* 
* Description - Used to log activities of each client to the <batch_name>.log file
*
* Input -       *handle - pointer to CURL handle;
*               type    - type of libcurl information passed, like headers, data, info, etc
*               *data   - pointer to data, like headers, etc
*               size    - number of bytes passed with <data> pointer
*               *userp  - pointer to user-specific data, which in our case is the 
*                         client_context structure
*
* Return Code/Output - On Success - 0, on Error -1
****************************************************************************************/
static int client_tracing_function (CURL *handle, 
                             curl_infotype type, 
                             unsigned char *data, 
                             size_t size, 
                             void *userp)
{
  client_context* cctx = (client_context*) userp;
  char*url_target = NULL, *url_effective = NULL;
  url_context* url_ctx = &cctx->bctx->url_ctx_array[cctx->url_curr_index];
  
#if 0 /* GF moved to end of function */
  if (detailed_logging)
    {
      char detailed_buff[CURL_ERROR_SIZE +1];
      
      if (size <= CURL_ERROR_SIZE)
        {
          memcpy (detailed_buff, data, size);
              
          detailed_buff[size] = '\0';
          fprintf(cctx->file_output, "%s\n", detailed_buff);
        }
    }
#endif

  if (url_logging)
    {
      url_target = url_ctx->url_str;
      
      /* Clients are being redirected back and forth by 3xx redirects. */
      curl_easy_getinfo (handle, CURLINFO_EFFECTIVE_URL, &url_effective);
    }

  const char*const url = url_effective ? url_effective : url_target;
  const int url_print = (url_logging && url) ? 1 : 0;

  /* 
     if we're using an URL_TEMPLATE, the url_str may well have changed, so don't log - GF 
  */
  const int url_diff = (url_print && url_effective && url_target && ! is_template (url_ctx)) ? 
    strcmp(url_effective, url_target) : 0;
  long response_status = 0;

  /* GF */
  /* 
     scan the server response for RESPONSE_TOKENS 
   */
  scan_response(type, (char*) data, size, cctx);

  const unsigned long time_resp = get_tick_count();
  const unsigned long offs_resp = time_resp - cctx->bctx->start_time;

  switch (type)
    {
    case CURLINFO_TEXT:
      if (verbose_logging)
	  if (verbose_logging > 1 || startswith(data,"About") ||
	   startswith(data,"Closing"))
	    write_log("==",(char *)data);
      break;

    case CURLINFO_ERROR:
      write_log("!! ERR",(char *)data);

      cctx->client_state = CSTATE_ERROR;

      stat_err_inc (cctx);
      first_hdrs_clear_all (cctx);
      break;

    case CURLINFO_HEADER_OUT:
      if (verbose_logging > 1)
	  write_log("=>","Send header");

      stat_data_out_add (cctx, (unsigned long) size);

      if (! first_hdr_req (cctx))
        {
          /* First header of the HTTP-request. */
          first_hdr_req_inc (cctx);
          stat_req_inc (cctx); /* Increment number of requests */
        }
      first_hdrs_clear_non_req (cctx);
      break;

    case CURLINFO_DATA_OUT:
      if (verbose_logging > 1)
	  write_log("=>","Send data");

      stat_data_out_add (cctx, (unsigned long) size);
      first_hdrs_clear_all (cctx);
      break;

    case CURLINFO_SSL_DATA_OUT:
      if (verbose_logging > 1) 
	  write_log("=>","Send ssl data");

      stat_data_out_add (cctx, (unsigned long) size);
      first_hdrs_clear_all (cctx);
      break;
      
    case CURLINFO_HEADER_IN:
      /* 
         CURL library assists us by passing here the full HTTP-header, 
         not just parts. 
      */
      stat_data_in_add (cctx, (unsigned long) size);

      {
        long response_module = 0;
        
        curl_easy_getinfo (handle, CURLINFO_RESPONSE_CODE, &response_status);

        if (verbose_logging > 1)
	  write_log_num("<= Recv header:",response_status);
        
        response_module = response_status / (long)100;
        
        switch (response_module)
          {

          case 1: /* 100-Continue and 101 responses */
            if (! first_hdr_1xx (cctx))
              {
	        write_log_num("!! CONT",response_status);

                /* First header of 1xx response */
                first_hdr_1xx_inc (cctx);
                stat_1xx_inc (cctx); /* Increment number of 1xx responses */
                stat_appl_delay_add (cctx, time_resp);
              }
            
            first_hdrs_clear_non_1xx (cctx);
            break;


          case 2: /* 200 OK */
            if (! first_hdr_2xx (cctx))
              {
	        write_log_num("!! OK",response_status);

                /* First header of 2xx response */
                first_hdr_2xx_inc (cctx);
                stat_2xx_inc (cctx); /* Increment number of 2xx responses */

                /* Count into the averages HTTP/S server response delay */
                stat_appl_delay_2xx_add (cctx, time_resp);
                stat_appl_delay_add (cctx, time_resp);
              }
            first_hdrs_clear_non_2xx (cctx);
            break;
       
          case 3: /* 3xx REDIRECTIONS */
            if (! first_hdr_3xx (cctx))
              {
	        write_log_num("!! RDR",response_status);

                /* First header of 3xx response */
                first_hdr_3xx_inc (cctx);
                stat_3xx_inc (cctx); /* Increment number of 3xx responses */
                stat_appl_delay_add (cctx, time_resp);
              }
            first_hdrs_clear_non_3xx (cctx);
            break;

          case 4: /* 4xx Client Error */
              if (! first_hdr_4xx (cctx))
              {
	        write_log_ext("!! ERCL",response_status,data);

                /* First header of 4xx response */
                first_hdr_4xx_inc (cctx);
                stat_4xx_inc (cctx);  /* Increment number of 4xx responses */

                stat_appl_delay_add (cctx, time_resp);
              }
             first_hdrs_clear_non_4xx (cctx);
             break;


          case 5: /* 5xx Server Error */
            if (! first_hdr_5xx (cctx))
              {
	        write_log_ext("!! ERSR",response_status,data);

                /* First header of 5xx response */
                first_hdr_5xx_inc (cctx);
                stat_5xx_inc (cctx);  /* Increment number of 5xx responses */

                stat_appl_delay_add (cctx, time_resp);
              }
            first_hdrs_clear_non_5xx (cctx);
            break;

          default :
	    write_log_num("<= WARNING: wrong response code (FTP?)",
	     response_status);
            /* FTP breaks it: - cctx->client_state = CSTATE_ERROR; */
            break;
          }
      } /* switch of response status */

      if (url_ctx->resp_status_errors_tbl)
        {
          if (response_status < 0 ||
              response_status > URL_RESPONSE_STATUS_ERRORS_TABLE_SIZE ||
              url_ctx->resp_status_errors_tbl[response_status])
            {
              cctx->client_state = CSTATE_ERROR;
            }
        }
      else
      {
        if (response_status < 0 || response_status >= 400)
          {
            /* 401 and 407 responses are just authentication challenges, that 
              virtual client may overcome. */
            if (response_status != 401 && response_status != 407)
              {
                cctx->client_state = CSTATE_ERROR;
              }
          }
      }

      break;

    case CURLINFO_DATA_IN:     
      if (verbose_logging > 1) 
          (void)fprintf(cctx->file_output,
                 "%ld %ld %ld %s<= Recv data: eff-url: %s, url: %s\n", 
                  offs_resp, cctx->cycle_num, cctx->url_curr_index,
		  cctx->client_name,
                  url_print ? url : "", url_diff ? url_target : "");

      stat_data_in_add (cctx,  (unsigned long) size);
      first_hdrs_clear_all (cctx);
      break;

    case CURLINFO_SSL_DATA_IN:
      if (verbose_logging > 1) 
	  write_log("<=","Recv ssl data");

      stat_data_in_add (cctx,  (unsigned long) size);
      first_hdrs_clear_all (cctx);
      break;

    default:
      fprintf (stderr, "default OUT - \n");
    }

  /* 
   GF
   Show the data after the header label
   */
  if (detailed_logging)
  {
      char detailed_buff[CURL_ERROR_SIZE +1]; size_t nbytes;
      
      nbytes = (size <= CURL_ERROR_SIZE)? size : CURL_ERROR_SIZE;
      memcpy (detailed_buff, data, nbytes);
      
      detailed_buff[nbytes] = '\0';
      fprintf(cctx->file_output, "%s%s\n\n", detailed_buff, nbytes < size? "..." : "");
  }

  
  // fflush (cctx->file_output); // Don't do it
  return 0;
}



/****************************************************************************************
* Function name - init_client_contexts
*
* Description - Allocate and initialize client contexts
* 
* Input -       *bctx     - pointer to batch context to be set to all clients  of the batch
*               *log_file - output file to be used by all clients of the batch
* Return Code -  On Success - 0, on Error -1
****************************************************************************************/
static int init_client_contexts (batch_context* bctx,
                                       FILE* log_file)
{
  int i;

  /* 
     Iterate through client contexts and initialize them. 
  */
  for (i = 0 ; i < bctx->client_num_max ; i++)
    {
      client_context* cctx = &bctx->cctx_array[i];

      /* 
         Build client name for logging, based on sequence number and 
         ip-address for each simulated client. 
      */
      cctx->cycle_num = 0;

      if (verbose_logging > 1)
         snprintf(cctx->client_name, sizeof(cctx->client_name) - 1, 
               "%d (%s) ", 
               i + 1, 
               bctx->ip_addr_array[i]);
      else
	 /* Shorten client name for low logging */
         snprintf(cctx->client_name, sizeof(cctx->client_name) - 1, 
               "%d ",i+1);

      /* Mark timer-ids as non-valid. */
      cctx->tid_sleeping = cctx->tid_url_completion = -1;

       /*
         Set index of the client within the batch.
         Useful to get the client's CURL handle from bctx. 
      */
      cctx->client_index = i;
      cctx->url_curr_index = 0; /* Actually zeroed by calloc. */
      
      /* Set output stream for each client to be either batch logfile or stderr. */
      cctx->file_output = stderr_print_client_msg ? stderr : log_file;

      /* 
         Set pointer in client to its batch object. The pointer will be used to get 
         configuration and set back statistics to batch.
      */
      cctx->bctx = bctx;
    }

  return 0;
}

/****************************************************************************************
* Function name - free_batch_data_allocations
*
* Description - Deallocates all  the kings batch allocations
* Input -       *bctx - pointer to batch context to release its allocations
* Return Code/Output - None
****************************************************************************************/
static void free_batch_data_allocations (batch_context* bctx)
{
  int i;

  if (! bctx)
    {
      return;
    }

  op_stat_point_release (&bctx->op_delta);
  op_stat_point_release (&bctx->op_total);
  
  /*
     Free client contexts 
  */
  if (bctx->cctx_array)
  {
      for (i = 0 ; i < bctx->client_num_max ; i++)
      {
          client_context* cctx = &bctx->cctx_array[i];
          
          if (cctx->handle)
          {
              curl_easy_cleanup (cctx->handle);
              cctx->handle = NULL;
          }
          
          /* Free client POST-buffers */ 
          if (cctx->post_data)
          {
              free (cctx->post_data);
              cctx->post_data = NULL;
          }
          
          if (cctx->logfile_headers)
          {
              fclose (cctx->logfile_headers);
              cctx->logfile_headers= NULL;
          }
          
          if (cctx->logfile_bodies)
          {
              fclose (cctx->logfile_bodies);
              cctx->logfile_bodies = NULL;
          }
          
          if (cctx->url_fetch_decision)
          {
              free (cctx->url_fetch_decision);
              cctx->url_fetch_decision = NULL;
          }
      }/* from for */
      
      free(bctx->cctx_array);
      bctx->cctx_array = NULL;
  }
  
  /* 
     Free url contexts
  */
  if (bctx->url_ctx_array)
  {
      /* Free all URL objects */
      
      for (i = 0 ; i < bctx->urls_num; i++)
      {
          url_context* url = &bctx->url_ctx_array[i];
          
          free_url (url, bctx->client_num_max);
      }
      
      /* Free URL context array */
      free (bctx->url_ctx_array);
      bctx->url_ctx_array = NULL;
  }
}

static void free_url (url_context* url, int clients_max)
{
  /* GF */
  free_url_extensions(url);
  
  /* Free url string */
  if (url->url_str)
    {
      free (url->url_str);
      url->url_str = 0;
      url->url_str_len = 0;
    }
  
  /* Free custom HTTP headers. */
  if (url->custom_http_hdrs)
    {
      curl_slist_free_all(url->custom_http_hdrs);
      url->custom_http_hdrs = 0;
    }
  
  if (url->form_str)
    {
      free (url->form_str);
      url->form_str = 0;
    }
  
  /* Free Form records file (credentials). */
  if (url->form_records_file)
    {
      free (url->form_records_file);
      url->form_records_file = 0;
    }
  
  /* Free form_records_array */
  if (url->form_records_array)
    {
      int j;
      for (j = 0; j < clients_max; j++)
        {
          int m;
          for (m = 0; m < FORM_RECORDS_MAX_TOKENS_NUM; m++)
            { 
              if (url->form_records_array[j].form_tokens[m])
                {
                  free (url->form_records_array[j].form_tokens[m]);
                  url->form_records_array[j].form_tokens[m] = 0;
                }
            }
        }
      
      free (url->form_records_array);
      url->form_records_array = 0;
    }
  
  /* Free upload file */
  if (url->upload_file)
    {
      free (url->upload_file);
      url->upload_file = 0;
    }
  
  /* Close file pointer of upload file */
  if (url->upload_file_ptr)
    {
      fclose (url->upload_file_ptr);
      url->upload_file_ptr = 0;
    }
  
  /* Free web-authentication credentials */
  if (url->web_auth_credentials)
    {
      free (url->web_auth_credentials);
      url->web_auth_credentials = 0;
    }
  
  /* Free proxy-authentication credentials */
  if (url->proxy_auth_credentials)
    {
      free (url->proxy_auth_credentials);
      url->proxy_auth_credentials = 0;
    }
  
  if (url->dir_log)
    {
      free (url->dir_log);
      url->dir_log = 0;
    }

  if (url->resp_status_errors_tbl)
    {
      free (url->resp_status_errors_tbl);
      url->resp_status_errors_tbl = 0;
    }
}
  
/*****************************************************************************
* Function name - create_ip_addrs
*
* Description - Adds ip-addresses of batches of loading clients to network adapter/s
* Input -       *bctx_array - pointer to the array of batch contexts
*               bctx_num    - number of batch contexts in <bctx_array>
* Return Code/Output - None
*******************************************************************************/
static int create_ip_addrs (batch_context* bctx_array, int bctx_num)
{
  int batch_index, client_index; /* Batch and client indexes */
  char*** ip_addresses =0;

  /* 
     Add secondary IP-addresses to the "loading" network interface. 
  */
  if (!(ip_addresses = (char***)calloc (bctx_num, sizeof (char**))))
    {
      fprintf (stderr, "%s - error: failed to allocate ip_addresses.\n", __func__);
      return -1;
    }
  
  for (batch_index = 0 ; batch_index < bctx_num ; batch_index++) 
    {
      /* 
         Allocate the array of IP-addresses 
      */
      if (!(ip_addresses[batch_index] = (char**)calloc (bctx_array[batch_index].client_num_max, 
                                               sizeof (char *))))
        {
          fprintf (stderr, 
                   "%s - error: failed to allocate array of ip-addresses for batch %d.\n", 
                   __func__, batch_index);
          return -1;
        }

      batch_context* bctx = &bctx_array[batch_index];

      /* 
         Set them to the batch contexts to remember them. 
      */
      bctx->ip_addr_array = ip_addresses[batch_index]; 

      /* 
         Allocate for each client a buffer and snprintf to it the IP-address string.
      */
      for (client_index = 0; client_index < bctx->client_num_max; client_index++)
        {
          if (ip_addr_str_allocate_init (bctx, 
                                         client_index, 
                                         &ip_addresses[batch_index][client_index]) == -1)
            {

              fprintf (stderr, 
                       "%s - error: ip_addr_str_allocate_init () - failed, batch [%d], client [%d]\n", 
                       __func__, batch_index, client_index);
              return -1;
            }
        }

      /* 
         Add all the addresses to the network interface as the secondary 
         ip-addresses, using netlink userland-kernel interface.
      */
      if (add_secondary_ip_addrs (bctx->net_interface,
                                  bctx->client_num_max, 
                                  (const char** const) ip_addresses[batch_index], 
                                  bctx->cidr_netmask,
                                  bctx->scope) == -1)
        {
          fprintf (stderr, 
                   "%s - error: add_secondary_ip_addrs() - failed for batch = %d\n", 
                   __func__, batch_index);
          return -1;
        }
    }

  return 0;
}

/*****************************************************************************
* Function name - ip_addr_str_allocate_init
*
* Description - Allocates a single string and inits it by printing a text presentation
*               of an IP-address (either IPv4 or IPv6).
*
* Input -       *bctx        - pointer to a batch context
*               client_index - number of client in the client-array
* Input/Output **addr_str - second pointer a string for IP-address, allocated by the
*                           function.
* Return Code/Output - None
*******************************************************************************/
static int ip_addr_str_allocate_init (batch_context* bctx,
                                      int client_index,
                                      char** addr_str)
{
  struct in_addr in_address;
  char ipv6_string[INET6_ADDRSTRLEN+1];
  char* ipv4_string = 0;
  char* ipaddrstr = 0;

  *addr_str = NULL;

  if (! (ipaddrstr = (char *)calloc (bctx->ipv6 ? INET6_ADDRSTRLEN + 1 : 
                                    INET_ADDRSTRLEN + 1, sizeof (char))))
    {
      fprintf (stderr, "%s - allocation of ipaddrstr failed for client %d.\n", 
               __func__, client_index);
      return -1;
    }
  
  if (bctx->ipv6 == 0)
    {
      /* 
         When clients are not using common IP, advance the 
         IPv4-address, using client index as the offset. 
      */
      in_address.s_addr = htonl (bctx->ip_addr_min + 
                                 (bctx->ip_shared_num ? next_ipv4_shared_index(bctx) :  (size_t) client_index));
      
      if (! (ipv4_string = inet_ntoa (in_address)))
        {
          fprintf (stderr, "%s - inet_ntoa() failed for ip_addresses of client [%d]\n", 
                   __func__, client_index) ;
          return -1;
        }
    }
  else
    {
      ///------------------------------------------------ IPv6 -----------------------------------------------------------------///

      /* 
         Non-shared IPv6. Advance the IPv6-address by incrementing previous address. 
      */
      if (!bctx->ip_shared_num)
        {
          if (client_index == 0)
            {
              memcpy (&bctx->in6_prev, &bctx->ipv6_addr_min, sizeof (bctx->in6_prev));
              memcpy (&bctx->in6_new, &bctx->ipv6_addr_min, sizeof (bctx->in6_new));
            }
          else
            {
              if (ipv6_increment (&bctx->in6_prev, &bctx->in6_new) == -1)
                {
                  fprintf (stderr, "%s - ipv6_increment() failed for ip_address of client [%d]\n", 
                           __func__, client_index) ;
                  return -1;
                }
            }
          memcpy (&bctx->in6_prev, &bctx->in6_new, sizeof (bctx->in6_prev));
        }
      else
        {
          /* Shared IP-addresses */
          memcpy (&bctx->in6_prev, &bctx->ipv6_addr_min, sizeof (bctx->in6_prev));
          memcpy (&bctx->in6_new, &bctx->ipv6_addr_min, sizeof (bctx->in6_new));
          
          size_t index_step = next_ipv6_shared_index (bctx);
          
          size_t k;
          for (k = 0; k < index_step; k++)
            {
              if (ipv6_increment (&bctx->in6_prev, &bctx->in6_new) == -1)
                {
                  fprintf (stderr, "%s - ipv6_increment() failed \n", __func__) ;
                  return -1;
                }
              memcpy (&bctx->in6_prev, &bctx->in6_new, sizeof (bctx->in6_prev));
            }
        }
      
      /* All IPv6 addresses */
      if (! inet_ntop (AF_INET6, &bctx->in6_new, ipv6_string, sizeof (ipv6_string)))
        {
          fprintf (stderr, "%s - inet_ntoa() failed for ip_addresses of client [%d]\n", 
                   __func__, client_index) ;
          return -1;
        }
    }
  
  snprintf (ipaddrstr, bctx->ipv6 ? INET6_ADDRSTRLEN : INET_ADDRSTRLEN, 
            "%s", bctx->ipv6 ? ipv6_string : ipv4_string);

  *addr_str = ipaddrstr;

  return 0;
}

/****************************************************************************************
* Function name - rewind_logfile_above_maxsize
*
* Description - Rewinds the file pointer, when reaching configurable max-size
*
* Input -       *filepointer - file pointer to control and rewind, when necessary
* Return Code/Output - On Success - 0, on Error -1
****************************************************************************************/
int rewind_logfile_above_maxsize (FILE* filepointer)
{
  long position = -1;

  if (!filepointer)
    return -1;

  if (filepointer == stderr)
    return 0;

  if ((position = ftell (filepointer)) == -1)
    {
      fprintf (stderr, 
               "%s - error: ftell () failed with errno = %d\n", __func__, errno);
      return 0;
    }

  if (position > (logfile_rewind_size* 1024*1024))
    {
      rewind (filepointer);
      fprintf (stderr, "%s - logfile with size %ld rewinded.\n", __func__, position);
    }

  return 0;
}

/****************************************************************************************
* Function name - ipv6_increment
*
* Description - Increments the source IPv6 address to provide the next
* 
* Input -       *src - pointer to the IPv6 address to be used as the source
* Input/Output  *dest - pointer to the resulted incremented address
* Return Code/Output - On Success - 0, on Error -1
****************************************************************************************/
static int ipv6_increment(const struct in6_addr *const src, 
                          struct in6_addr *const dest)
{
  uint32_t temp = 1;
  int i;

  for (i = 15; i > 1; i--) 
    {
      temp += src->s6_addr[i];
      dest->s6_addr[i] = temp & 0xff;
      temp >>= 8;
    }

  if (temp != 0)
    {
      fprintf (stderr, "%s - error: passing the scope.\n "
               "Check you IPv6 range to be within the same scope.\n", __func__);
      return -1;
    }

  dest->s6_addr[1] = src->s6_addr[1];
  dest->s6_addr[0] = src->s6_addr[0];

  return 0;
}

/*****************************************************************************
* Function name - create_thr_subbatches 
*
* Description - Cuts a batch to several sub-batches, each to be used by a 
*               separate thread.
*
* Input -       *bc_arr        - pointer to an array of batch contexts
*               subbatches_num - number sub-batches to produce (threads)
*
* Return Code/Output - On Success - 0, on Error -1
*******************************************************************************/
static int create_thr_subbatches (batch_context *bc_arr, int subbatches_num)
{
  if (! bc_arr)
  {
      return -1;
  }
  
  if (subbatches_num < 2 || subbatches_num > BATCHES_MAX_NUM)
  {
      fprintf (stderr, "%s - error: number of subbatches (%d) "
               "should not be less than 2 or above BATCHES_MAX_NUM (%d)\n", 
               __func__, subbatches_num, BATCHES_MAX_NUM);
      return -1;
  }
  
  if (bc_arr[0].ip_shared_num > 1 && subbatches_num < bc_arr[0].ip_shared_num)
  {
      fprintf (stderr, "%s - error: subbatches should deal either with a single shared IP "
               "or with at least a single shared IP per subbatch\n", 
               __func__);
      return -1;   
  }

  batch_context master;
  memcpy (&master, &bc_arr[0], sizeof (master));

  if (master.client_num_max < subbatches_num)
  {
      fprintf (stderr, "%s - error: wrong input CLIENT_NUM_MAX is less than "
               "the subbatches number (%d).\n", __func__, subbatches_num);
      return -1;
  }

  int c_num_max = 0;


  int i;
  for (i = 0 ; i < subbatches_num ; i++) 
  {
      sprintf (bc_arr[i].batch_name, "%s_%d", master.batch_name, i);
      sprintf (bc_arr[i].batch_logfile, "%s.log", bc_arr[i].batch_name);
      sprintf (bc_arr[i].batch_statistics, "%s.txt", bc_arr[i].batch_name);
      
      if (i != subbatches_num)
      {
          bc_arr[i].client_num_max = master.client_num_max / subbatches_num;
          c_num_max += bc_arr[i].client_num_max; 
      }
      else
      {
          bc_arr[i].client_num_max = master.client_num_max - c_num_max;
      }
      
      if (master.client_num_start)
      {
          bc_arr[i].client_num_start = master.client_num_start / subbatches_num;
          
          if (! bc_arr[i].client_num_start)
              bc_arr[i].client_num_start = 1;
      }
      
      if (master.clients_rampup_inc)
      {
          bc_arr[i].clients_rampup_inc = master.clients_rampup_inc / subbatches_num;
          
          if (! bc_arr[i].clients_rampup_inc)
              bc_arr[i].clients_rampup_inc = 1;
      }
      
      strcpy(bc_arr[i].net_interface, master.net_interface);
      
      bc_arr[i].ipv6 = master.ipv6;
      
      if (! master.ip_shared_num)
      {
          if (! bc_arr[i].ipv6)
          {
              bc_arr[i].ip_addr_min = i ? bc_arr[i - 1].ip_addr_max : master.ip_addr_min;
              bc_arr[i].ip_addr_max = bc_arr[i].ip_addr_min + bc_arr[i].client_num_max;
          }
          else
          {
              // ========= IPv6 range ======== //
              
              if (! i)
              {
                  bc_arr[i].ipv6_addr_min = master.ipv6_addr_min; 
              }
              else
              {
                  if (ipv6_increment (&bc_arr[i - 1].ipv6_addr_max, &bc_arr[i].ipv6_addr_min) == -1)
                  {
                      fprintf (stderr, "%s - error: ipv6_increment() failed\n ", __func__);
                      return -1;
                  }
              }
              
              struct in6_addr in6_temp = bc_arr[i].ipv6_addr_min;
              
              int k;
              for (k = 0; k < bc_arr[i].client_num_max; k++)
              {
                  if (ipv6_increment (&in6_temp, 
                                      &bc_arr[i].ipv6_addr_max) == -1)
                  {
                      fprintf (stderr, "%s - error: ipv6_increment() failed\n ", __func__);
                      return -1;
                  }
                  
                  in6_temp = bc_arr[i].ipv6_addr_max;
              }
          }
      }
      else
      {
          //
          // Shared IP-addresses.
          //
          if (! bc_arr[i].ipv6)
          {
              bc_arr[i].ip_addr_min = master.ip_addr_min;
              bc_arr[i].ip_addr_max = master.ip_addr_max;
          }
          else
          {
              bc_arr[i].ipv6_addr_min = master.ipv6_addr_min;
              bc_arr[i].ipv6_addr_max = master.ipv6_addr_max;
          }
      }
      
      bc_arr[i].ip_shared_num = master.ip_shared_num;

      bc_arr[i].cidr_netmask = master.cidr_netmask;

      memcpy (bc_arr[i].scope, master.scope, sizeof (bc_arr[i].scope));

      bc_arr[i].cycles_num = master.cycles_num;

      strncpy (bc_arr[i].user_agent, 
               master.user_agent, 
               sizeof (bc_arr[i].user_agent) -1);

      bc_arr[i].urls_num = master.urls_num;

      if (i)
      {
          if (! (bc_arr[i].url_ctx_array = (url_context *) cl_calloc (bc_arr[i].urls_num, 
                                                                      sizeof (url_context))))
          {
              fprintf (stderr, 
                       "%s - error: failed to allocate URL-context array for %d urls\n", 
                       __func__, bc_arr[i].urls_num);
              return -1;
          }
          memcpy (bc_arr[i].url_ctx_array, master.url_ctx_array, bc_arr[i].urls_num * sizeof (url_context));
          
          int j;
          for (j = 0; j < bc_arr[i].urls_num; j++)
          {
              bc_arr[i].url_ctx_array[j].url_str = strdup(master.url_ctx_array[j].url_str);
          }
      }

      bc_arr[i].url_index = master.url_index;

      bc_arr[i].first_cycling_url = master.first_cycling_url;

      bc_arr[i].last_cycling_url = master.last_cycling_url;

      bc_arr[i].cycling_completed = master.cycling_completed;

      /* Zero the pointer to be initialized. */
      bc_arr[i].multiple_handle = 0;

      /* 
         Allocate the array of IP-addresses. 
         TODO: memory leak of ip-addresses with the batch bc_arr[0] 
      */

      bc_arr[i].ip_addr_array = 0;

      if (!(bc_arr[i].ip_addr_array = (char**)calloc (bc_arr[i].client_num_max,
                                               sizeof (char *))))
        {
          fprintf (stderr, 
                   "%s - error: failed to allocate array of ip-addresses for batch %d.\n", 
                   __func__, i);
          return -1;
        }
      else
        {
          int j;

          for (j = 0; j < bc_arr[i].client_num_max; j++)
            {
              if (ip_addr_str_allocate_init (&bc_arr[i], j, 
	      				     &bc_arr[i].ip_addr_array[j]) == -1)
                {
                  fprintf (stderr, 
                           "%s - error: ip_addr_str_allocate_init () - failed, "
			   "batch [%d], client [%d]\n", 
                           __func__, i, j);
                  return -1;
                }
            }
        }

      if (i)
      {
          bc_arr[i].cctx_array = 0;
          bc_arr[i].free_clients = 0;
          
          /*
            Allocate array of client contexts
          */
          if (!(bc_arr[i].cctx_array =
                (client_context *) cl_calloc (bc_arr[i].client_num_max, 
                                              sizeof (client_context))))
          {
              fprintf (stderr, "\"%s\" - %s - failed to allocate cctx.\n", 
                       bc_arr[i].batch_name, __func__);
              return -1;
          }
          if (master.req_rate)
          {
              /*
                Allocate list of free clients
              */
              if (!(bc_arr[i].free_clients =
                    (int *) calloc (bc_arr[i].client_num_max,sizeof (int))))
              {
                  fprintf (stderr,
                           "\"%s\" - %s - failed to allocate list of free clients.\n", 
                           bc_arr[i].batch_name, __func__);
                  return -1;
              }
              else
              {
                  /*
                    Initialize the list, all clients are free
                    Fill the list in reverse, last memeber is picked first
                  */
                  bc_arr[i].free_clients_count = bc_arr[i].client_num_max;
                  int ix = bc_arr[i].free_clients_count, client_num = 1;
                  while (ix-- > 0)
                      bc_arr[i].free_clients[ix] = client_num++;
              }
          }
      }
      
      /* Zero the pointers to be initialized. */
      bc_arr[i].do_client_num_gradual_increase = 
          master.do_client_num_gradual_increase;
      bc_arr[i].stop_client_num_gradual_increase = 
          master.stop_client_num_gradual_increase;
      
      
      if (create_response_logfiles_dirs (&bc_arr[i]) == -1)
      {
          fprintf (stderr, 
                   "\"%s\" - create_response_logfiles_dirs () failed .\n", 
                   __func__);
          return -1;
      }
      
      if (alloc_client_formed_buffers (&bc_arr[i]) == -1)
      {
          fprintf (stderr, 
                   "\"%s\" - alloc_client_formed_buffers () failed .\n", 
                   __func__);
          return -1;
      }
      
      if (alloc_client_fetch_decision_array (&bc_arr[i]) == -1)
      {
          fprintf (stderr, 
                   "\"%s\" - alloc_client_fetch_decision_array () failed .\n", 
                   __func__);
          return -1;
      }
      
      if (init_operational_statistics (&bc_arr[i]) == -1)
      {
          fprintf (stderr, 
                   "\"%s\" - init_operational_statistics () failed .\n", 
                   __func__);
          return -1;
      }
  }
  
  return 0;
}
