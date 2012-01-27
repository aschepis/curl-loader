/*
*     batch.h
*
* 2006-2007 Copyright (c) 
* Robert Iakobashvili, <coroberti@gmail.com>
* Michael Moser,  <moser.michael@gmail.com>                 
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
*/


#ifndef BATCH_H
#define BATCH_H

#include <stddef.h>
#include <netinet/in.h>
#include <pthread.h>

#include <curl/curl.h>

#include "timer_queue.h"
#include "timer_node.h"
#include "url.h"
#include "statistics.h"

#define BATCH_NAME_SIZE 64
#define BATCH_NAME_EXTRA_SIZE 12
#define POST_BUFFER_SIZE 256

#define CUSTOM_HDRS_MAX_NUM 64

typedef enum form_usagetype
{
    FORM_USAGETYPE_START = 0,

    FORM_USAGETYPE_UNIQUE_USERS_AND_PASSWORDS,
    FORM_USAGETYPE_UNIQUE_USERS_SAME_PASSWORD,
    FORM_USAGETYPE_SINGLE_USER,
    FORM_USAGETYPE_RECORDS_FROM_FILE,
    FORM_USAGETYPE_AS_IS,

    FORM_USAGETYPE_END,
} form_usagetype;

struct client_context;
struct event_base;
struct event;

/**********************
  struct batch_context

  Batch is a group of clients with the same characteristics and loading
  behavior.

  The structure is used to keep all batch-relevant configuration and run-time
  information.
*/
typedef struct batch_context
{

  /*------------------------ GENERAL SECTION ------------------------------ */

  /* Some non-empty name of a batch load without empty spaces, tabs, etc */
  char batch_name[BATCH_NAME_SIZE];

  /* Logfile <batch-name>.log */
  char batch_logfile[BATCH_NAME_SIZE+BATCH_NAME_EXTRA_SIZE];

  /* Statistics file <batch-name>.txt */
  char batch_statistics[BATCH_NAME_SIZE+BATCH_NAME_EXTRA_SIZE];

  /* Operational statistics file <batch-name>.ops */
  char batch_opstats[BATCH_NAME_SIZE+BATCH_NAME_EXTRA_SIZE];

  /* Maximum number of clients (each client with its own IP-address) in the batch */
  int client_num_max;

  /* Number of clients to start with */
  int client_num_start;



  /* 
     Clients added per second for the loading start phase.
  */
  long clients_rampup_inc;
  
   /* Name of the network interface to be used for loading, e.g. "eth0", "eth1:16" */
  char net_interface[16];

  /* Flag: 0 means IPv4, 1 means IPv6 */ 
  int ipv6;
  
  /* Minimal IPv4-address of a client in the batch (host order). */
  long ip_addr_min;
  /* Maximum IPv4-address of a client in the batch (host order).*/
  long ip_addr_max;

  /* 
     Shared ip-addresses to be used by clients. Former ip-common for the single
     common for all clients ip-address.
   */
  int ip_shared_num;

  size_t ipv4_shared_index;

  /* 
     CIDR netmask number from 0 to 128, like 16 or 24, etc. If the input netmask is
     a dotted IPv4 address, we convert it to CIDR by calculating number of 1 bits.
  */
  int cidr_netmask;

  /* "global", "host", "link", for IPV6 only "site" */
  char scope[16];

  /* Minimal IPv6-address of a client in the batch. */
  struct in6_addr ipv6_addr_min;

  /* Miximum IPv6-address of a client in the batch. */
  struct in6_addr ipv6_addr_max;

  struct in6_addr in6_prev, in6_new;

  size_t ipv6_shared_index;
 
   /* 
      Number of cycles to repeat the urls downloads and afterwards sleeping 
      cycles. Zero means run it until time to run is exhausted.
   */
  long cycles_num;

  /*
      Time to run in msec.  Zero means time to run is infinite.
  */
  unsigned long run_time;

  /*
      Client fixed request rate per second.  Zero means send request after
      receiving reply.
  */
  int req_rate;

   /* 
      User-agent string to appear in the HTTP 1/1 requests.
  */
  char user_agent[256];


  /*------- URL SECTION - fetching urls ----- */

  /* Number of total urls, should be more or equal to 1 */
  int urls_num;
  
  /* Array of all url contexts */
  url_context* url_ctx_array;

  /* 
     Index of the parsed url in url_ctx_array below.
  */
  int url_index;

  /* 
     Index of the first cycling url. Minimal index of url not marked
     as <dont_cycle>.
  */
  int first_cycling_url;

  /* 
     Index of the last cycling url. Maximum index of url not marked
     as <dont_cycle>.
  */
  int last_cycling_url;

  /* Indicates, that all cycling operations have been done */
  int cycling_completed;


  /*------------------------- ASSISTING SECTION ----------------------------*/

  /* 
     The sequence num of a batch in batch_array. Zero batch is the batch group 
     leader and is in charge for statistics presentation on behave of all other batches
  */
  size_t batch_id;

  /* Thread id, filled by pthread_create (). Used by pthread_join () syscall */
  pthread_t thread_id;

  /* Multiple handle for curl. Contains all curl handles of a batch */
  CURLM *multiple_handle;
  
   /* Assisting array of pointers to ip-addresses */
  char** ip_addr_array;

  /* Current parsing state. Used on reading and parsing conf-file. */ 
  size_t batch_init_state; 

  /* Common error buffer for all batch clients */
  char error_buffer[CURL_ERROR_SIZE];

  /* Array of all client contexts for the batch */
  struct client_context* cctx_array;

  /* Number of clients free to send fixed rate requests */
  int free_clients_count;

  /* List of clients free to send fixed rate requests */
  int* free_clients;

  /* Indicates that request scheduling is over */
  int requests_completed;

  /* Request rate timer invocation sequence number within a second */
  int req_rate_timer_invocation;

  /* Counter used mainly by smooth mode: active clients */
  int active_clients_count;

  /* Number of clients "sleeping" their after url timeout. */
  int sleeping_clients_count;

  /* 
     Whether to do gradual increase of loading clients to prevent
     a simulteneous huge flow of client requests to server.
  */
  int do_client_num_gradual_increase;

  int stop_client_num_gradual_increase;

  /* 
     Number of already scheduled clients. Used to schedule new
     clients in a gradual fashion, when <clients_rampup_inc> is positive. 
  */
  int clients_current_sched_num;

  /*  Waiting queue timeouts in smooth mode */
  timer_queue* waiting_queue;

  /* The timer-node for timer testing the logfile size */
  timer_node logfile_timer_node;

  /* The timer-node for timer to add more clients during ramp-up period. */
  timer_node clients_num_inc_timer_node;

  /* The timer-node for timer checking user-input at a screen (stdin). */
  timer_node screen_input_timer_node;

  /* The timer-node for fixed request rate timer. */
  timer_node req_rate_timer_node;

  /* Event base from event_init () of libevent. */
  struct event_base* eb;

  /* Pointer to structure used by lebevent. */
  struct event* timer_event;

  /* Pointer to structure used by lebevent. */
  struct event* timer_next_load_event;


  /*--------------- STATISTICS  --------------------------------------------*/

  /* The file to be used for statistics output */
  FILE* statistics_file;

  /* The file to be used for operational statistics output */
  FILE* opstats_file;

  /* Dump operational statistics indicator, 0: no dump */
  int dump_opstats;

  /* Timestamp, when the loading started */
  unsigned long start_time; 

  /* The last timestamp */
  unsigned long last_measure;

  /* HTTP counters since the last measurements */
  stat_point http_delta;
  /* HTTP counters since the loading started */
  stat_point http_total;

  /* HTTPS counters since the last measurements */
  stat_point https_delta;
  /* HTTPS counters since the loading started */
  stat_point https_total;

  /* Operations statistics */
  op_stat_point op_delta;
  op_stat_point op_total;

  /* Count of response times dumped before new-line,
   used to limit line length */
  int ct_resps;

} batch_context;


int is_batch_group_leader (batch_context* bctx);

size_t next_ipv4_shared_index (batch_context* bctx);
size_t next_ipv6_shared_index (batch_context* bctx);




#endif /* BATCH_H */
