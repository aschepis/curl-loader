/*
*     conf.h
*
* 2006 Copyright (c) 
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
*/

#ifndef CONF_H
#define CONF_H

#include <stddef.h>
#include <linux/limits.h> /* NAME_MAX, PATH_MAX */



/* ----------------------------------------------------------------------------
   Global configuration parameters, comming with the command-line.
*/

/* 
   Configurable time to perform connect. If connection has not been established
   within this time limit the connection attempt in progress will be terminated.  
*/  
extern int connect_timeout;

/* 
   Flag, whether to perform verbose logging. Very usefull for debugging, but files
   tend to become huge. Thus, they can be kept smaller by using 
   logfile_rewind_size.
*/
extern int verbose_logging;

/* 
   Flag, whether to run batches as a sub-batch per thread. 
   To utilize SMP/multi-core HW capabilities a batch can be separated
   to N non-overlapping (by addresses/ports) sub-batches. Each sub-batch
   could be run from a separate thread by using -t <N> command line option.
*/
extern int threads_subbatches_num;

/* 
   Time in seconds between intermediate statistics printouts to
   screen as well as to the statistics file
*/
extern long snapshot_statistics_timeout;

/*
  Output to the logfile will be re-directed to the file's start, thus 
  overwriting previous logged strings. Effectively, keeps the log history
  limited to below number of cycles.
*/
extern long logfile_rewind_size;

/* 
   Whether to print to stdout the body of the downloaded file.
   Used for debugging.
*/
extern int output_to_stdout;

/*
   If to output all client messages to stderr, otherwise to the batch logfile/s. 
*/
extern int stderr_print_client_msg;

/* 
   Used by the smooth loading mode decide, either to continue loading
   attempting a new cycle (when TRUE), or to return error and do not
   continue any more.
*/
extern unsigned long error_recovery_client;

/*
  Loading modes: Storming and Smooth 
*/
enum load_mode
  {
    LOAD_MODE_HYPER = 0, /* Hyper-mode via epoll () */
    LOAD_MODE_SMOOTH = 1,    /* Smooth mode via select () */
  };

#define LOAD_MODE_DEFAULT LOAD_MODE_HYPER

extern int loading_mode;

/* 
   Whether to include url name string to all log outputs. May be useful,
   normally used with verbose logging, like '-v -u' in command line.
*/
extern int url_logging;
extern int detailed_logging;

extern int warnings_skip;

/*
   Name of the configuration file. 
*/
extern char config_file[PATH_MAX + 1];

/*
   Name of the proxy.
*/
extern char config_proxy[PATH_MAX];


/*
  HTTP requests: GET, POST and PUT.
  3xx redirections are supported valid options for each above case.
*/
typedef enum appl_req_type
{
  HTTP_REQ_TYPE_FIRST= 0,

  HTTP_REQ_TYPE_GET = 1,
  HTTP_REQ_TYPE_POST = 2,
  HTTP_REQ_TYPE_PUT = 3,
  HTTP_REQ_TYPE_HEAD = 4,
  HTTP_REQ_TYPE_DELETE = 5,

  HTTP_REQ_TYPE_LAST = 7,

  // FTP, etc
} appl_req_type;


/* 
   Parses command line and fills configuration params.
*/
int parse_command_line (int argc, char *argv []);


struct batch_context;
int parse_config_file (char* const filename, 
                       struct batch_context* bctx_array, 
                       size_t bctx_array_size);

int create_response_logfiles_dirs (struct batch_context* bctx);
int alloc_client_formed_buffers (struct batch_context* bctx);
int alloc_client_fetch_decision_array (struct batch_context* bctx);
int init_operational_statistics(struct batch_context* bctx);

/*
  Prints out usage of the program.
*/
void print_help ();

#endif /* CONF_H */
