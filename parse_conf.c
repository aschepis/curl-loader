/* 
*     parse_conf.c
*
* 2006-2007 Copyright (c) 
* Robert Iakobashvili, <coroberti@gmail.com>
* All rights reserved.*
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

// must be first include
#include "fdsetsize.h"

#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdarg.h>

#include <ctype.h>
#include <sys/types.h>
#include <sys/uio.h>

#include "conf.h"
#include "batch.h"
#include "client.h"
#include "cl_alloc.h"
#include "url.h"

extern char * strcasestr(const char *, const char *);

#define EXPLORER_USERAGENT_STR "Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)" 
#define BATCH_MAX_CLIENTS_NUM 4096

#define NON_APPLICABLE_STR ""
#define NON_APPLICABLE_STR_2 "N/A"

#define REQ_GET "GET"
#define REQ_POST "POST"
#define REQ_PUT "PUT"
#define REQ_HEAD "HEAD"
#define REQ_DELETE "DELETE"

#define FT_UNIQUE_USERS_AND_PASSWORDS "UNIQUE_USERS_AND_PASSWORDS"
#define FT_UNIQUE_USERS_SAME_PASSWORD "UNIQUE_USERS_SAME_PASSWORD"
#define FT_SINGLE_USER "SINGLE_USER"
#define FT_RECORDS_FROM_FILE "RECORDS_FROM_FILE"
#define FT_AS_IS "AS_IS"

#define AUTH_BASIC "BASIC"
#define AUTH_DIGEST "DIGEST"
#define AUTH_GSS_NEGOTIATE "GSS_NEGOTIATE"
#define AUTH_NTLM "NTLM"
#define AUTH_ANY "ANY"

static int random_seed = -1;
static char random_state[256];

static unsigned char 
resp_status_errors_tbl_default[URL_RESPONSE_STATUS_ERRORS_TABLE_SIZE];


/*
  value - supposed to be a null-terminated string.
*/
typedef int (*fparser) (batch_context*const bctx, char*const value);

/*
  Used to map a tag to its value parser function.
*/
typedef struct tag_parser_pair
{
    char* tag; /* string name of the param */
    fparser parser;
} tag_parser_pair;

/* 
 * Declarations of tag parsing functions.
*/

/*
 * GENERAL section tag parsers. 
*/
static int batch_name_parser (batch_context*const bctx, char*const value);
static int clients_num_max_parser (batch_context*const bctx, char*const value);
static int clients_num_start_parser (batch_context*const bctx, char*const value);
static int clients_rampup_inc_parser (batch_context*const bctx, char*const value);
static int interface_parser (batch_context*const bctx, char*const value);
static int netmask_parser (batch_context*const bctx, char*const value);
static int ip_addr_min_parser (batch_context*const bctx, char*const value);
static int ip_addr_max_parser (batch_context*const bctx, char*const value);
static int ip_shared_num_parser (batch_context*const bctx, char*const value);
static int cycles_num_parser (batch_context*const bctx, char*const value);
static int run_time_parser (batch_context*const bctx, char*const value);
static int user_agent_parser (batch_context*const bctx, char*const value);
static int urls_num_parser (batch_context*const bctx, char*const value);
static int dump_opstats_parser (batch_context*const bctx, char*const value);
static int req_rate_parser (batch_context*const bctx, char*const value);

/*
 * URL section tag parsers. 
*/
static int url_parser (batch_context*const bctx, char*const value);
static int url_short_name_parser (batch_context*const bctx, char*const value);
static int url_use_current_parser (batch_context*const bctx, char*const value);
static int url_dont_cycle_parser (batch_context*const bctx, char*const value);
static int header_parser (batch_context*const bctx, char*const value);
static int request_type_parser (batch_context*const bctx, char*const value);

static int username_parser (batch_context*const bctx, char*const value);
static int password_parser (batch_context*const bctx, char*const value);
static int form_usage_type_parser (batch_context*const bctx, char*const value);
static int form_string_parser (batch_context*const bctx, char*const value);
static int form_records_file_parser (batch_context*const bctx, char*const value);

static int upload_file_parser (batch_context*const bctx, char*const value);

static int multipart_form_data_parser (batch_context*const bctx, char*const value);

static int web_auth_method_parser (batch_context*const bctx, char*const value);
static int web_auth_credentials_parser (batch_context*const bctx, char*const value);
static int proxy_auth_method_parser (batch_context*const bctx, char*const value);
static int proxy_auth_credentials_parser (batch_context*const bctx, char*const value);

static int fresh_connect_parser (batch_context*const bctx, char*const value);

static int timer_tcp_conn_setup_parser (batch_context*const bctx, char*const value);
static int timer_url_completion_parser (batch_context*const bctx, char*const value);
static int timer_after_url_sleep_parser (batch_context*const bctx, char*const value);

static int ftp_active_parser (batch_context*const bctx, char*const value);
static int log_resp_headers_parser (batch_context*const bctx, char*const value);
static int log_resp_bodies_parser (batch_context*const bctx, char*const value);
static int response_status_errors_parser (batch_context*const bctx, char*const value);
static int transfer_limit_rate_parser (batch_context*const bctx, char*const value);

static int fetch_probability_parser (batch_context*const bctx, char*const value);
static int fetch_probability_once_parser (batch_context*const bctx, char*const value);

static int form_records_random_parser (batch_context*const bctx, char*const value);
static int form_records_file_max_num_parser(batch_context*const bctx, char*const value);

/* GF url-set parsers.  */
static int url_template_parser(batch_context* const bctx, char* const value);
static int url_token_parser(batch_context* const bctx, char* const value);
static int url_token_file_parser(batch_context* const bctx, char* const value);
static int response_token_parser(batch_context* const bctx, char* const value);
static int form_records_cycle_parser(batch_context* const bctx, char* const value);
static int random_seed_parser (batch_context*const bctx, char*const value);

static int ignore_content_length (batch_context*const bctx, char*const value);
static int url_random_range (batch_context*const bctx, char*const value);
static int url_random_token (batch_context*const bctx, char*const value);

/*
 * The mapping between tag strings and parsing functions.
 */
static const tag_parser_pair tp_map [] =
{
    /*------------------------ GENERAL SECTION ------------------------------ */
    {"BATCH_NAME", batch_name_parser},
    {"CLIENTS_NUM_MAX", clients_num_max_parser},
    {"CLIENTS_NUM_START", clients_num_start_parser},
    {"CLIENTS_RAMPUP_INC", clients_rampup_inc_parser},
    {"INTERFACE", interface_parser},
    {"NETMASK", netmask_parser},
    {"IP_ADDR_MIN", ip_addr_min_parser},
    {"IP_ADDR_MAX", ip_addr_max_parser},
    {"IP_SHARED_NUM", ip_shared_num_parser},
    {"CYCLES_NUM", cycles_num_parser},
    {"RUN_TIME", run_time_parser},
    {"USER_AGENT", user_agent_parser},
    {"URLS_NUM", urls_num_parser},
    {"DUMP_OPSTATS", dump_opstats_parser},
    {"REQ_RATE", req_rate_parser},
    

    /*------------------------ URL SECTION -------------------------------- */

    {"URL", url_parser},
    {"URL_SHORT_NAME", url_short_name_parser},
    {"URL_USE_CURRENT", url_use_current_parser},
    {"URL_DONT_CYCLE", url_dont_cycle_parser},
    {"HEADER", header_parser},
    {"REQUEST_TYPE", request_type_parser},

    {"USERNAME", username_parser},
    {"PASSWORD", password_parser},
    {"FORM_USAGE_TYPE", form_usage_type_parser},
    {"FORM_STRING", form_string_parser},
    {"FORM_RECORDS_FILE", form_records_file_parser},

    {"UPLOAD_FILE", upload_file_parser},

    {"MULTIPART_FORM_DATA", multipart_form_data_parser},

    {"WEB_AUTH_METHOD", web_auth_method_parser},
    {"WEB_AUTH_CREDENTIALS", web_auth_credentials_parser},
    {"PROXY_AUTH_METHOD", proxy_auth_method_parser},
    {"PROXY_AUTH_CREDENTIALS", proxy_auth_credentials_parser},

    {"FRESH_CONNECT", fresh_connect_parser},

    {"TIMER_TCP_CONN_SETUP", timer_tcp_conn_setup_parser},
    {"TIMER_URL_COMPLETION", timer_url_completion_parser},
    {"TIMER_AFTER_URL_SLEEP", timer_after_url_sleep_parser},

    {"FTP_ACTIVE", ftp_active_parser},
    {"LOG_RESP_HEADERS", log_resp_headers_parser},
    {"LOG_RESP_BODIES", log_resp_bodies_parser},
    {"RESPONSE_STATUS_ERRORS", response_status_errors_parser},

    {"TRANSFER_LIMIT_RATE", transfer_limit_rate_parser},

    {"FETCH_PROBABILITY", fetch_probability_parser},
    {"FETCH_PROBABILITY_ONCE", fetch_probability_once_parser},

    {"FORM_RECORDS_RANDOM", form_records_random_parser},
    {"FORM_RECORDS_FILE_MAX_NUM", form_records_file_max_num_parser},
    
    /* GF */
    {"URL_TEMPLATE", url_template_parser},
    {"URL_TOKEN", url_token_parser},
    {"URL_TOKEN_FILE", url_token_file_parser},
    {"RESPONSE_TOKEN", response_token_parser},
    {"FORM_RECORDS_CYCLE", form_records_cycle_parser},
    {"RANDOM_SEED", random_seed_parser},

    {"IGNORE_CONTENT_LENGTH", ignore_content_length},
    {"URL_RANDOM_RANGE", url_random_range},
    {"URL_RANDOM_TOKEN", url_random_token},

    {NULL, 0}
};

static int validate_batch (batch_context*const bctx);
static int validate_batch_general (batch_context*const bctx);
static int validate_batch_url (batch_context*const bctx);

static int post_validate_init (batch_context*const bctx);
static int load_form_records_file (batch_context*const bctx, url_context* url);
static int load_form_record_string (char*const input, 
                                    size_t input_length,
                                    form_records_cdata* form_record,
                                    size_t record_num,
                                    char** separator);

static int add_param_to_batch (char*const input, 
                               size_t input_length,
                               batch_context*const bctx, 
                               int*const batch_num);

static int pre_parser (char** ptr, size_t* len);
static url_appl_type url_schema_classification (const char* const url);
static char* skip_non_ws (char*ptr, size_t*const len);
static char* eat_ws (char*ptr, size_t*const len);
static int is_ws (char*const ptr);
static int is_non_ws (char*const ptr);

static int find_first_cycling_url (batch_context* bctx);
static int find_last_cycling_url (batch_context* bctx);
static int netmask_to_cidr (char *dotted_ipv4);
static int print_correct_form_usagetype (form_usagetype ftype, char* value);
static int parse_timer_range (char* input, 
                              size_t input_len, 
                              long* first_val, 
                              long* second_val);

static int upload_file_streams_alloc(batch_context* batch);

/****************************************************************************************
* Function name - find_tag_parser
*
* Description - Makes a look-up of a tag value parser function for an input tag-string 
* 
* Input -       *tag - pointer to the tag string, coming from the configuration file
* Return Code/Output - On success - parser function, on failure - NULL
****************************************************************************************/
static fparser find_tag_parser (const char* tag)
{
    size_t index;

    for (index = 0; tp_map[index].tag; index++)
    {
        if (!strcmp (tp_map[index].tag, tag))
            return tp_map[index].parser;
    }    
    return NULL;
}

/****************************************************************************************
* Function name - add_param_to_batch
*
* Description - Takes configuration file string of the form TAG = value and extacts
*               loading batch configuration parameters from it.
* 
* Input -       *str_buff   - pointer to the configuration file string of the form TAG = value
*               str_len     - length of the <str_buff> string
*               *bctx_array - array of the batch contexts
* Input/Output  batch_num   - index of the batch to fill and advance, when required.
*                             Still supporting multiple batches in one batch file.
*
* Return Code/Output - On success - 0, on failure - (-1)
****************************************************************************************/
static int add_param_to_batch (char*const str_buff, 
                               size_t  str_len,
                               batch_context*const bctx_array, 
                               int*const batch_num)
{
  if (!str_buff || !str_len || !bctx_array)
    return -1;

  /*We are not eating LWS, as it supposed to be done before... */
    
  char* equal = NULL;

  if ( ! (equal = strchr (str_buff, '=')))
    {
      fprintf (stderr, 
               "%s - error: input string \"%s\" is short of '=' sign.\n", 
               __func__, str_buff) ;
      return -1;
    }
  else
    {
      *equal = '\0'; /* The idea from Igor Potulnitsky */
    }

  long string_length = (long) str_len;
  long val_len = 0;
  if ((val_len = string_length - (long)(equal - str_buff) - 1) < 0)
    {
      *equal = '=' ;
      fprintf(stderr, "%s - error: in \"%s\" a valid name should follow '='.\n", 
               __func__, str_buff);
      return -1;
    }

  /* remove TWS */
  str_len = strlen (str_buff) + 1;
  char* str_end = skip_non_ws (str_buff, &str_len);
  if (str_end)
      *str_end = '\0';
  
  /* Lookup for value parsing function for the input tag */
  fparser parser = 0;
  if (! (parser = find_tag_parser (str_buff)))
  {
      fprintf (stderr, "%s - error: unknown tag %s.\n"
               "\nATTENTION: If the tag not misspelled, read README.Migration file.\n\n",
               __func__, str_buff);
      return -1;
  }

  /* Removing LWS, TWS and comments from the value */
  size_t value_len = (size_t) val_len;
  char* value = equal + 1;

  if (pre_parser (&value, &value_len) == -1)
  {
      fprintf (stderr,"%s - error: pre_parser () failed for tag %s and value \"%s\".\n",
               __func__, str_buff, equal + 1);
      return -1;
  }

  if (!strlen (value))
    {
      fprintf (stderr,"%s - warning: tag %s has an empty value string.\n",
               __func__, str_buff);
      return 0;
    }

  /* Remove quotes from the value */
  if (*value == '"')
  {
      value++, value_len--;
      if (value_len < 2)
      {
          return 0;
      }
      else
      {
          if (*(value +value_len-2) == '"')
          {
              *(value +value_len-2) = '\0';
              value_len--;
          }
      }
  }
  
  if (strstr (str_buff, tp_map[0].tag))
  {
      /* On string "BATCH_NAME" - next batch and move the number */
       ++(*batch_num);
  }

  if ((*parser) (&bctx_array[*batch_num], value) == -1)
    {
      fprintf (stderr,"%s - parser failed for tag %s and value %s.\n",
               __func__, str_buff, equal + 1);
      return -1;
    }

  return 0;
}

/****************************************************************************************
* Function name - load_form_record_string
*
* Description - Parses string with credentials <user>SP<password>, allocates at virtual 
*               client memory and places the credentials to the client post buffer.
* 
* Input -       *input        - pointer to the credentials file string
*               input_len     - length of the <input> string
*
* Input/Output  *form_record  - pointer to the form_records_cdata array
*               record_num    - index of the record ?
*               *separator    - the separating symbol initialized by the first string and 
*                               further used.
* Return Code/Output - On success - 0, on failure - (-1)
****************************************************************************************/
static int load_form_record_string (char*const input, 
                                    size_t input_len,
                                    form_records_cdata* form_record, 
                                    size_t record_num,
                                    char** separator)
{
  static const char* separators_supported [] =
    {
      ",",
      ":",
      ";",
      " ", 
    /*"@", we need @ for email addresses */
      "/", 
      0
    };
  char* sp = NULL;
  int i;

  if (!input || !input_len)
    {
      fprintf (stderr, "%s - error: wrong input\n", __func__);
      return -1;
    }
  
  /* 
     Figure out the separator used by the first string analyses 
  */
  if (! record_num)
    {
      for (i = 0; separators_supported [i]; i++)
        {
          if ((sp = strchr (input, *separators_supported [i])))
            {
              *separator = (char *) separators_supported [i]; /* Remember the separator */
              break;
            }
        }

      if (!separators_supported [i])
        {
          fprintf (stderr,
                   "%s - failed to locate in the first string \"%s\" \n" 
                   "any supported separator.\nThe supported separators are:\n",
               __func__, input);

          for (i = 0; separators_supported [i]; i++)
            {
              fprintf (stderr,"\"%s\"\n", separators_supported [i]);
            }
          return -1;
        }
    }

  char * token = 0, *strtokp = 0;
  size_t token_count  = 0;

  for (token = strtok_r (input, *separator, &strtokp); 
       token != 0;
       token = strtok_r (0, *separator, &strtokp))
    {
      size_t token_len = strlen (token);

      if (! token_len)
        {
          fprintf (stderr, "%s - warning: token is empty. \n", __func__);
        }
      else if (token_len >= FORM_RECORDS_TOKEN_MAX_LEN)
        {
          fprintf (stderr, "%s - error: token is above the allowed "
                   "FORM_RECORDS_TOKEN_MAX_LEN (%d). \n", 
                   __func__, FORM_RECORDS_TOKEN_MAX_LEN);
        }
      else
        {
          if (! (form_record->form_tokens[token_count] = 
                 calloc (token_len +1, sizeof (char))))
            {
              fprintf (stderr, "%s - error: calloc() failed with errno %d\n", 
                       __func__, errno);
              return -1;
            }
          else
            {
              strcpy (form_record->form_tokens[token_count], token);
            }
        }

      if (++token_count >= FORM_RECORDS_MAX_TOKENS_NUM)
        {
          fprintf (stderr, "%s - warning: tokens number is above" 
                   " FORM_RECORDS_MAX_TOKENS_NUM (%d). \n", 
                   __func__, FORM_RECORDS_MAX_TOKENS_NUM);
          break;
        }
    }


  return 0;
}

/****************************************************************************************
* Function name - pre_parser
*
* Description - Prepares value token from the configuration file to parsing. Removes LWS,
*               cuts off comments, removes TWS or after quotes closing, removes quotes.
* 
* Input/Output - **ptr - second pointer to value string
*                *len  - pointer to the length of the value string
* Return Code/Output - On success - 0, on failure - (-1)
****************************************************************************************/
static int pre_parser (char** ptr, size_t* len)
{
    char* value_start = NULL;
    char* quotes_closing = NULL;
    char* value_end = NULL;

    /* remove LWS */
    if ( ! (value_start = eat_ws (*ptr, len)))
    {
        fprintf (stderr, "%s - error: only LWS found in the value \"%s\".\n", 
                 __func__, value_start);
        return -1;
    }

    /* Cut-off the comments in value string, starting from '#' */
    char* comments = NULL;
    if ((comments = strchr (value_start, '#')))
    {
        *comments = '\0'; /* The idea from Igor Potulnitsky */
        if (! (*len = strlen (value_start)))
        {  
            fprintf (stderr, "%s - error: value \"%s\" has only comments.\n", 
                     __func__, value_start);
            return -1;
        }
    }

    /* Everything after quotes closing or TWS */
    
    if (*value_start == '"')
    {
        /* Enable usage of quotted strings with wight spaces inside, line User-Agent strings. */

        if (*(value_start + 1))
        {
            if ((quotes_closing = strchr (value_start + 1, '"')))
                value_end = quotes_closing + 1;
        }
        else
        {
            value_end = value_start;
        }
    }
     
    /* If not quotted strings, thus, cut the value on the first white space */ 
    if (!value_end)
        value_end = skip_non_ws (value_start, len);

    if (value_end)
    {
        *value_end = '\0';
    }

    *ptr = value_start;
    *len = strlen (value_start) + 1;
  
    return 0;
}

/*******************************************************************************
* Function name - parse_timer_range
*
* Description - Parses potential timer ranges with values looking either as 
*                  "1000" or "1000-2000"
* 
* Input-    *input - pointer to value string
*                  input_len - length of the value string, pointed by <input>
* Input/Output - *first_val - used to return the first long value
*                               *second_val - used to return the second long value, which is optional
*
* Return Code/Output - On success - 0, on failure - (-1)
*********************************************************************************/
static int parse_timer_range (char* input,
                              size_t input_len,
                              long* first_val,
                              long* second_val)
{
  if (!input || !input_len || !first_val || !second_val)
    {
      fprintf (stderr, "%s - error: wrong input\n", __func__);
      return -1;
    }

  const char separator = '-';
  char* second = 0;
  char* sep = 0;
  sep = strchr (input, separator);

  if (sep)
  {
      *sep = '\0';
      
      if ((sep - input < (int)input_len) && (*(sep + 1)))
      {
          second = sep + 1;
      }
      else
      {
          *sep = separator;
          fprintf (stderr, "%s - error: wrong input %s. "
                   "Separator %c exists, but no value after the separator.\n", 
                   __func__, input, separator);
          return -1 ;
      }
  }
  
  *first_val = atol (input);

  if (*first_val < 0)
  {
      fprintf (stderr, "%s - error: wrong input %s. "
               "Only non-negative values are allowed.\n", 
               __func__, input);
      return -1;
  }
  
  if (sep)
  {
      *second_val = atol (second);
      
      if (sep && *second_val < 0)
      {
          fprintf (stderr, "%s - error: wrong input %s. "
                   "Only non-negative values are allowed.\n", 
                   __func__, second);
          return -1;
      }
      
      if (sep && *first_val >= *second_val)
      {
          fprintf (stderr, "%s - error: wrong input. "
                   "First value (%ld) should be less then the second (%ld).\n"
                   "Switch the order.\n", __func__, *first_val, *second_val);
          return -1 ;
      }
  }
  
  return 0;
}

/******************************************************************************
* Function name - eat_ws
*
* Description - Eats leading white space. Returns pointer to the start of 
*                    the non-white-space or NULL. Returns via len a new length.
* 
* Input -               *ptr - pointer to the url context
* Input/Output- *len - pointer to a lenght 
* Return Code/Output - Returns pointer to the start of the non-white-space or NULL
*******************************************************************************/
char* eat_ws (char* ptr, size_t*const len)
{
  if (!ptr || !*len)
    return NULL;

  while (*len && is_ws (ptr))
    ++ptr, --(*len);

  return *len ? ptr : NULL;
}

/******************************************************************************
* Function name - skip_non_ws
*
* Description - Skips non-white space. Returns pointer to the start of 
*                    the white-space or NULL. Returns via len a new length.
* 
* Input -               *ptr - pointer to the url context
* Input/Output- *len - pointer to a lenght 
* Return Code/Output - Returns pointer to the start of the white-space or NULL
*******************************************************************************/
static char* skip_non_ws (char*ptr, size_t*const len)
{
  if (!ptr || !*len)
    return NULL;

  while (*len && is_non_ws (ptr))
    ++ptr, --(*len);

  return *len ? ptr : NULL;
}

/******************************************************************************
* Function name - is_ws
*
* Description - Determines, whether a char pointer points to a white space
* Input -               *ptr - pointer to the url context
* Return Code/Output - If white space - 1, else 0
*******************************************************************************/
static int is_ws (char*const ptr)
{
  return (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') ? 1 : 0;
}

/******************************************************************************
* Function name - is_non_ws
*
* Description - Determines, whether a char pointer points to a non-white space
* Input -               *ptr - pointer to the url context
* Return Code/Output - If non-white space - 1, else 0
*******************************************************************************/
static int is_non_ws (char*const ptr)
{
  return ! is_ws (ptr);
}

/*
**
** TAG PARSERS IMPLEMENTATION
**
*/
static int batch_name_parser (batch_context*const bctx, char*const value)
{
    strncpy (bctx->batch_name, value, BATCH_NAME_SIZE);
    return 0;
}
static int clients_num_max_parser (batch_context*const bctx, char*const value)
{
    bctx->client_num_max = 0;
    bctx->client_num_max = atoi (value);
    
    /* fprintf (stderr, "\nclients number is %d\n", bctx->client_num_max); */
    if (bctx->client_num_max < 1)
    {
        fprintf (stderr, "%s - error: clients number (%d) is out of the range\n", 
                 __func__, bctx->client_num_max);
        return -1;
    }
    return 0;
}
static int clients_num_start_parser (batch_context*const bctx, char*const value)
{
    bctx->client_num_start = 0;
    bctx->client_num_start = atoi (value);
    
    /* fprintf (stderr, "\nclients number is %d\n", bctx->client_num_start); */
    if (bctx->client_num_start < 0)
    {
        fprintf (stderr, "%s - error: clients starting number (%d) is out of the range\n", 
                 __func__, bctx->client_num_start);
        return -1;
    }
    return 0;
}
static int interface_parser (batch_context*const bctx, char*const value)
{
    strncpy (bctx->net_interface, value, sizeof (bctx->net_interface) -1);
    return 0;
}
static int netmask_parser (batch_context*const bctx, char*const value)
{
    /* CIDR number of non-masked first bits -16, 24, etc */

  if (! strchr (value, '.') && !strchr (value, ':'))
    {
      /* CIDR number of non-masked first bits -16, 24, etc */
      bctx->cidr_netmask = atoi (value);
    }
  else
    {
      bctx->cidr_netmask = netmask_to_cidr (value);
    }
  
  if (bctx->cidr_netmask < 1 || bctx->cidr_netmask > 128)
    {
      fprintf (stderr, 
               "%s - error: network mask (%d) is out of range. Expecting from 1 to 128.\n", 
               __func__, bctx->cidr_netmask);
      return -1;
    }

  return 0;
}
static int ip_addr_min_parser (batch_context*const bctx, char*const value)
{
    struct in_addr inv4;
    memset (&inv4, 0, sizeof (struct in_addr));

    bctx->ipv6 = strchr (value, ':') ? 1 : 0;

    if (inet_pton (bctx->ipv6 ? AF_INET6 : AF_INET, 
                   value, 
                   bctx->ipv6 ? (void *)&bctx->ipv6_addr_min : (void *)&inv4) == -1)
      {
        fprintf (stderr, 
                 "%s - error: inet_pton ()  failed for ip_addr_min %s\n", 
                 __func__, value);
        return -1;
      }
    
    if (!bctx->ipv6)
      {
        bctx->ip_addr_min = ntohl (inv4.s_addr);
      }

    return 0;
}
static int ip_addr_max_parser (batch_context*const bctx, char*const value)
{
  struct in_addr inv4;
  memset (&inv4, 0, sizeof (struct in_addr));
  
  bctx->ipv6 = strchr (value, ':') ? 1 : 0;

  if (inet_pton (bctx->ipv6 ? AF_INET6 : AF_INET, 
                 value, 
                 bctx->ipv6 ? (void *)&bctx->ipv6_addr_max : (void *)&inv4) == -1)
    {
      fprintf (stderr, 
               "%s - error: inet_pton ()  failed for ip_addr_max %s\n", 
               __func__, value);
      return -1;
    }
  
  if (!bctx->ipv6)
    {
      bctx->ip_addr_max = ntohl (inv4.s_addr);
    }

  return 0;
}
static int ip_shared_num_parser (batch_context*const bctx, char*const value)
{
  bctx->ip_shared_num = atol (value);
  if (bctx->ip_shared_num <= 0)
    {
      fprintf (stderr, 
               "%s - error: a positive number is expected as the value"
               "for tag IP_SHARED_NUM\n", __func__);
      return -1;
    }
    return 0;
}

static int cycles_num_parser (batch_context*const bctx, char*const value)
{
    bctx->cycles_num = atol (value);
    if (bctx->cycles_num < 0)
    {
        bctx->cycles_num = LONG_MAX - 1;
    }
    return 0;
}

static int run_time_parser (batch_context*const bctx, char*const value)
{
    char *token = 0, *strtokp = 0;
    const char *delim = ":";
    long dhms[4];
    int ct = 0;
    const int max_ct = sizeof(dhms)/sizeof(*dhms);
    char value_buf[100];
      // preserve value from destruction by strtok
    (void)strncpy(value_buf,value,sizeof(value_buf));
    (void)memset((char *)dhms,0,sizeof(dhms));
    for (token = strtok_r(value_buf,delim,&strtokp);
       token != 0; token = strtok_r(0,delim,&strtokp), ct++)
    {
        if (ct >= max_ct)
	{
            (void)fprintf(stderr, 
                "%s - error: a value in the form [[[D:]H:]M:]S is expected"
                " for tag RUN_TIME\n", __func__);
            return -1;
        }
	int i = 0;
	while (++i < max_ct)
           dhms[i - 1] = dhms[i];
        dhms[max_ct - 1] = atol(token);
    }
    long run_time = (60 * (60 * (24 * dhms[0] + dhms[1]) + dhms[2]) + dhms[3])*
       1000; // msecs
    if (run_time < 0)
    {
        run_time = LONG_MAX - 1;
    }
    bctx->run_time = (unsigned long)run_time;
    return 0;
}

static int clients_rampup_inc_parser (batch_context*const bctx, char*const value)
{
    bctx->clients_rampup_inc = atol (value);
    if (bctx->clients_rampup_inc < 0)
    {
        fprintf (stderr, 
                 "%s - error: clients_rampup_inc (%s) should be a zero or positive number\n", 
                 __func__, value);
        return -1;
    }
    return 0;
}
static int user_agent_parser (batch_context*const bctx, char*const value)
{
    if (strlen (value) <= 0)
    {
        fprintf(stderr, "%s - warning: empty USER_AGENT "
                "\"%s\", taking the defaults\n", __func__, value);
        return 0;
    }
    strncpy (bctx->user_agent, value, sizeof(bctx->user_agent) - 1);
    return 0;
}
static int urls_num_parser (batch_context*const bctx, char*const value)
{
    bctx->urls_num = atoi (value);
    
    if (bctx->urls_num < 1)
    {
        fprintf (stderr, 
                 "%s - error: urls_num (%s) should be one or more.\n",
                 __func__, value);
        return -1;
    }    
    /* Preparing the staff to load URLs and handles */
    if (! (bctx->url_ctx_array = 
           (url_context *) cl_calloc (bctx->urls_num, sizeof (url_context))))
    {
        fprintf (stderr, 
                 "%s - error: failed to allocate URL-context array for %d urls\n", 
                 __func__, bctx->urls_num);
        return -1;
    }

    bctx->url_index = -1;  /* Starting from the 0 position in the arrays */

    return 0;
}

static int dump_opstats_parser (batch_context*const bctx, char*const value)
{
    if (value[0] == 'Y' || value[0] == 'y' ||
      value[0] == 'N' || value[0] == 'n')
    	bctx->dump_opstats = (value[0] == 'Y' || value[0] == 'y');
    else
    {
        fprintf (stderr, 
           "%s - error: DUMP_OPSTATS value (%s) must start with Y|y|N|n.\n",
                 __func__, value);
        return -1;
    }    
    return 0;
}

static int req_rate_parser (batch_context*const bctx, char*const value)
{
    bctx->req_rate = atol (value);
    if (bctx->req_rate < 0)
    {
        bctx->req_rate = 0;
    }
    return 0;
}

static int url_parser (batch_context*const bctx, char*const value)
{
    size_t url_length = 0;

    bctx->url_index++;

    if ((int)bctx->url_index >= bctx->urls_num)
    {
        fprintf (stderr, 
                 "%s - error: number of urls above the value of URLS_NUM value.\n",
                 __func__);
        return -1;
    }
    
    if (! (url_length = strlen (value)))
    {
        if (! bctx->url_index)
        {
            fprintf(stderr, "%s - error: empty url is OK not as the first url\n", __func__);
            return -1;
        }

        /* Inherits application type of the primary url */
        bctx->url_ctx_array[bctx->url_index].url_appl_type = 
            bctx->url_ctx_array[bctx->url_index -1].url_appl_type;
    }
    else 
    {
        if (! (bctx->url_ctx_array[bctx->url_index].url_str = 
               (char *) calloc (url_length +1, sizeof (char))))
        {
            fprintf (stderr,
                     "%s - error: allocation failed for url string \"%s\"\n", 
                     __func__, value);
            return -1;
        }

        bctx->url_ctx_array[bctx->url_index].url_str_len = url_length +1;

        strcpy(bctx->url_ctx_array[bctx->url_index].url_str, value);

        bctx->url_ctx_array[bctx->url_index].url_appl_type = 
            url_schema_classification (value);
    }
    
    bctx->url_ctx_array[bctx->url_index].url_ind = bctx->url_index;
    
    return 0;
}

static int url_short_name_parser (batch_context*const bctx, char*const value)
{
    size_t url_name_length = 0;
        
    if ((url_name_length = strlen (value)) <= 0)
    {
        fprintf(stderr, "%s - warning: empty url short name is OK\n ", __func__);
        return 0;
    }

    strncpy(bctx->url_ctx_array[bctx->url_index].url_short_name, value, 
            sizeof (bctx->url_ctx_array[bctx->url_index].url_short_name) -1);
    
    return 0;
}
static int url_use_current_parser (batch_context*const bctx, char*const value)
{
  long url_use_current_flag = 0;
  url_use_current_flag = atol (value);

  if (url_use_current_flag < 0 || url_use_current_flag > 1)
    {
      fprintf (stderr, 
               "%s - error: URL_USE_CURRENT should be "
               "either 0 or 1 and not %ld.\n", __func__, url_use_current_flag);
      return -1;
    }

  bctx->url_ctx_array[bctx->url_index].url_use_current = url_use_current_flag;

  return 0;
}
static int url_dont_cycle_parser (batch_context*const bctx, char*const value)
{
  long url_dont_cycle_flag = 0;
  url_dont_cycle_flag = atol (value);

  if (url_dont_cycle_flag < 0 || url_dont_cycle_flag > 1)
    {
      fprintf (stderr, 
               "%s - error: URL_DONT_CYCLE should be either 0 or 1 and not %ld.\n",
               __func__, url_dont_cycle_flag);
      return -1;
    }

  bctx->url_ctx_array[bctx->url_index].url_dont_cycle = url_dont_cycle_flag;
  return 0;
}
static int header_parser (batch_context*const bctx, char*const value)
{
  size_t hdr_len;
  if (!value || !(hdr_len = strlen (value)))
    {
      fprintf (stderr, "%s - error: wrong input.\n", __func__);
      return -1;
    }
  
  const char colomn = ':';
  url_context* url = &bctx->url_ctx_array[bctx->url_index];

  if (url->url_appl_type == URL_APPL_HTTP || 
      url->url_appl_type == URL_APPL_HTTPS)
    {
      if (!strchr (value, colomn))
        {
          fprintf (stderr, 
                   "%s - error: HTTP protocol requires \"%c\" colomn symbol" 
                   " in HTTP headers.\n", __func__, colomn);
          return -1;
        }
    }

  if (url->custom_http_hdrs_num >= CUSTOM_HDRS_MAX_NUM)
    {
      fprintf (stderr, 
               "%s - error: number of custom HTTP headers is limited to %d.\n", 
               __func__, CUSTOM_HDRS_MAX_NUM);
      return -1;
    }

  if (!(url->custom_http_hdrs = curl_slist_append (url->custom_http_hdrs,
                                                   value)))
    {
      fprintf (stderr, "%s - error: failed to append the header \"%s\"\n", 
               __func__, value);
      return -1;
    }
  
  url->custom_http_hdrs_num++;

  return 0;
}
static int request_type_parser (batch_context*const bctx, char*const value)
{
    if (!strcmp (value, REQ_GET))
    {
        bctx->url_ctx_array[bctx->url_index].req_type = 
          HTTP_REQ_TYPE_GET;
    }
    else if (!strcmp (value, REQ_POST))
    {
        bctx->url_ctx_array[bctx->url_index].req_type = 
          HTTP_REQ_TYPE_POST;
    }
    else if (!strcmp (value, REQ_PUT))
    {
        bctx->url_ctx_array[bctx->url_index].req_type = 
          HTTP_REQ_TYPE_PUT;
    }
    else if (!strcmp (value, REQ_HEAD))
	{
		bctx->url_ctx_array[bctx->url_index].req_type = 
		  HTTP_REQ_TYPE_HEAD;
	}
	else if (!strcmp (value, REQ_DELETE))
	{
		bctx->url_ctx_array[bctx->url_index].req_type = 
		  HTTP_REQ_TYPE_DELETE;
	}
    else
    {
        fprintf (stderr, 
				 "%s - error: REQ_TYPE (%s) is not valid. Use %s, %s, %s, %s or %s.\n", 
				__func__, value, REQ_GET, REQ_POST, REQ_PUT, REQ_HEAD, REQ_DELETE);
        return -1;
    }
    return 0;
}
static int username_parser (batch_context*const bctx, char*const value)
{
  if (strlen (value) <= 0)
    {
      fprintf(stderr, "%s - warning: empty USERNAME \n", 
              __func__);
      return 0;
    }

  strncpy (bctx->url_ctx_array[bctx->url_index].username, 
           value, 
           sizeof(bctx->url_ctx_array[bctx->url_index].username) - 1);

  return 0;
}
static int password_parser (batch_context*const bctx, char*const value)
{
  if (strlen (value) <= 0)
    {
      fprintf(stderr, "%s - warning: empty PASSWORD\n", 
              __func__);
      return 0;
    } 
  strncpy (bctx->url_ctx_array[bctx->url_index].password, 
           value, 
           sizeof(bctx->url_ctx_array[bctx->url_index].password) - 1);
  return 0;
}
static int form_usage_type_parser (batch_context*const bctx, char*const value)
{

  if (!strcmp (value, FT_UNIQUE_USERS_AND_PASSWORDS))
    {
      bctx->url_ctx_array[bctx->url_index].form_usage_type = 
        FORM_USAGETYPE_UNIQUE_USERS_AND_PASSWORDS;
    }
  else if (!strcmp (value, FT_UNIQUE_USERS_SAME_PASSWORD))
    {
      bctx->url_ctx_array[bctx->url_index].form_usage_type = 
        FORM_USAGETYPE_UNIQUE_USERS_SAME_PASSWORD;
    }
  else if (!strcmp (value, FT_SINGLE_USER))
    {
      bctx->url_ctx_array[bctx->url_index].form_usage_type = 
        FORM_USAGETYPE_SINGLE_USER;
    }
  else if (!strcmp (value, FT_RECORDS_FROM_FILE))
    {
      bctx->url_ctx_array[bctx->url_index].form_usage_type = 
        FORM_USAGETYPE_RECORDS_FROM_FILE;
    }
  else if (!strcmp (value, FT_AS_IS))
    {
      bctx->url_ctx_array[bctx->url_index].form_usage_type = 
        FORM_USAGETYPE_AS_IS;
    }
  else
    {
      fprintf(stderr, "%s - error: FORM_USAGE_TYPE to be choosen from:"
              "%s , %s ,\n" "%s , %s , %s \n" ,  __func__, 
              FT_UNIQUE_USERS_AND_PASSWORDS, FT_UNIQUE_USERS_SAME_PASSWORD,
              FT_SINGLE_USER, FT_RECORDS_FROM_FILE, FT_AS_IS);
      return -1;
    }
  
  return 0;
}
static int form_string_parser (batch_context*const bctx, char*const value)
{
  int count_percent_s_percent_d = 0, count_percent_s = 0;
  char* pos_current = NULL;
  const form_usagetype ftype = 
        bctx->url_ctx_array[bctx->url_index].form_usage_type;

  const size_t value_len = strlen (value);
  
  if (value_len)
  	fprintf(stderr, "found form_str %s\n", value);
  else
  	fprintf(stderr, "form_str has zero length\n");

  if (!value_len)
    {
      fprintf(stderr, "%s - error: empty FORM_STRING tag is not supported.\n", 
              __func__);
      return -1;
    }

  if (ftype <= FORM_USAGETYPE_START || ftype >= FORM_USAGETYPE_END)
    {
      fprintf(stderr, "%s - error: please, before FORM_STRING place the "
              "defined FORM_USAGE_TYPE tag with its values to be choosen from:"
              "%s , %s ,\n" "%s , %s , %s \n" , __func__, 
              FT_UNIQUE_USERS_AND_PASSWORDS, FT_UNIQUE_USERS_SAME_PASSWORD,
              FT_SINGLE_USER, FT_RECORDS_FROM_FILE, FT_AS_IS);
      return -1;
    }

  if (strcmp (value, NON_APPLICABLE_STR) || 
      strcmp (value, NON_APPLICABLE_STR_2))
    {
      /*count "%s%d" and "%s" sub-stritngs*/

      pos_current = value;
      while (*pos_current && (pos_current = strstr (pos_current, "%s%d")))
        {
          ++count_percent_s_percent_d;
          ++pos_current;
        }

      pos_current = value;
      while (*pos_current && (pos_current = strstr (pos_current, "%s")))
        {
          ++count_percent_s;
          ++pos_current;
        }

      if (count_percent_s_percent_d == 2 && count_percent_s == 2)
        {
          if (ftype != FORM_USAGETYPE_UNIQUE_USERS_AND_PASSWORDS)
            {
              return print_correct_form_usagetype (ftype, value);
            }
        }
      else if (count_percent_s_percent_d == 1 && count_percent_s == 2)
        {
          if (ftype  != FORM_USAGETYPE_UNIQUE_USERS_SAME_PASSWORD)
            {
              return print_correct_form_usagetype (ftype, value);
            }
        }
      else if (count_percent_s_percent_d == 0 && count_percent_s == 2)
        {
          if (ftype != FORM_USAGETYPE_SINGLE_USER &&
              ftype != FORM_USAGETYPE_RECORDS_FROM_FILE)
            {
              return print_correct_form_usagetype (ftype, value);
            }
        }
      else
        {
          /* GF added 2nd condition to allow more than 2 records from file */
          if (ftype != FORM_USAGETYPE_AS_IS && ftype != FORM_USAGETYPE_RECORDS_FROM_FILE)
            {
              fprintf (stderr, 
                       "\n%s - error: FORM_STRING (%s) is not valid. \n"
                       "Please, use:\n"
                       "- to generate unique users with unique passwords two \"%%s%%d\" , something like " 
                       "\"user=%%s%%d&password=%%s%%d\" \n"
                       "- to generate unique users with the same passwords one \"%%s%%d\" \n"
                       "for users and one \"%%s\" for the password," 
                       "something like \"user=%%s%%d&password=%%s\" \n"
                       "- for a single configurable user with a password two \"%%s\" , something like "
                       "\"user=%%s&password=%%s\" \n",
                       "- to load user credentials (records) from a file two \"%%s\" , something like "
                       "\"user=%%s&password=%%s\" \n and _FILE defined.\n",
                       __func__);
              return -1;
            }
        }

      if (! (bctx->url_ctx_array[bctx->url_index].form_str = 
             calloc (value_len +1, sizeof (char))))
        {
          fprintf(stderr, 
                  "%s - error: failed to allocate memory for FORM_STRING value.\n", 
                  __func__);
          return -1;
        }

      strncpy (bctx->url_ctx_array[bctx->url_index].form_str, value, value_len);
    }

  return 0;
}
static int form_records_file_parser (batch_context*const bctx, char*const value)
{
  struct stat statbuf;
  size_t string_len = 0;

  if (strcmp (value, NON_APPLICABLE_STR))
    {
      /* Stat the file, it it exists. */
      if (stat (value, &statbuf) == -1)
        {
          fprintf(stderr, "%s error: file \"%s\" does not exist.\n",  __func__,value);
          return -1;
        }

      string_len = strlen (value) + 1;
      if (! (bctx->url_ctx_array[bctx->url_index].form_records_file = 
             (char *) calloc (string_len, sizeof (char))))
        {
          fprintf(stderr, 
                  "%s error: failed to allocate memory for form_records_file" 
                  "with errno %d.\n",  __func__, errno);
          return -1;
        }

      strncpy (bctx->url_ctx_array[bctx->url_index].form_records_file, 
               value, 
               string_len -1);

      if (load_form_records_file (bctx, &bctx->url_ctx_array[bctx->url_index]) == -1)
      {
          fprintf(stderr, "%s error: load_form_records_file () failed.\n", __func__);
          return -1;
      }
    }

    return 0;
}

static int form_records_random_parser (batch_context*const bctx, char*const value)
{
  long url_form_records_random_flag = 0;
  url_form_records_random_flag = atol (value);

  if (url_form_records_random_flag < 0 || url_form_records_random_flag > 1)
    {
      fprintf (stderr, 
               "%s - error: FORM_RECORDS_RANDOM should be either 0 or 1 and not %ld.\n",
               __func__, url_form_records_random_flag);
      return -1;
    }

  bctx->url_ctx_array[bctx->url_index].form_records_random =  url_form_records_random_flag;
  return 0;

}

static int form_records_file_max_num_parser(batch_context*const bctx, char*const value)
{
  long max_records = 0;
  max_records = atol (value);

  if (max_records < 0)
    {
      fprintf (stderr, 
               "%s - error: FORM_RECORDS_FILE_MAX_NUM should be a "
               "positive value and not %ld.\n", __func__, max_records);
      return -1;
    }

  if (bctx->url_ctx_array[bctx->url_index].form_records_file)
    {
      fprintf (stderr, 
               "%s - error: FORM_RECORDS_FILE_MAX_NUM should be specified "
               "prior to tag FORM_RECORDS_FILE\n"
               "Please, change the order of the tags in your configuration.\n", __func__);
      return -1;
    }

  bctx->url_ctx_array[bctx->url_index].form_records_file_max_num =  max_records;
  return 0;
}

static int upload_file_parser  (batch_context*const bctx, char*const value)
{
  struct stat statbuf;
  size_t string_len = 0;

  if (strcmp (value, NON_APPLICABLE_STR))
    {
      /* Stat the file, it it exists. */
      if (stat (value, &statbuf) == -1)
        {
          fprintf(stderr, "%s error: file \"%s\" does not exist.\n",  __func__,value);
          return -1;
        }

      string_len = strlen (value) + 1;
      if (! (bctx->url_ctx_array[bctx->url_index].upload_file = 
             (char *) calloc (string_len, sizeof (char))))
        {
          fprintf(stderr, 
                  "%s error: failed to allocate memory with errno %d.\n",
                  __func__, errno);
          return -1;
        }

      strncpy (bctx->url_ctx_array[bctx->url_index].upload_file,
               value, 
               string_len -1);

      bctx->url_ctx_array[bctx->url_index].upload_file_size = statbuf.st_size;
      
      /* GF  */
      if (upload_file_streams_alloc(bctx) < 0)
          return -1;
    }
    return 0;
}

static int multipart_form_data_parser (batch_context*const bctx, char*const value)
{
  char* fieldname = 0, *eq = 0, *content;
  size_t value_len = strlen (value);
  url_context* url = &bctx->url_ctx_array[bctx->url_index];
  
  if (!value_len)
    {
      fprintf(stderr, "%s - error: zero length value passed.\n", __func__);
      return -1;
    }
  
  /* 
     Examples:
         
     "yourname=Michael" 
     "filedescription=Cool text file with cool text inside"
     "htmlcode=<HTML></HTML>;type=text/html"

     "file=@cooltext.txt"
     "coolfiles=@fil1.gif,fil2.txt,fil3.html" 
  */
  
  if (! (eq = strchr (value, '=')))
    {
      fprintf(stderr, "%s - error: no '=' sign in multipart_form_data.\n", __func__);
      return -1;
    }
  
  *eq = '\0';
  fieldname = value;
  
  /* TODO: Test also fieldname not to be empty space */
  if (!strlen (fieldname))
    {
      fprintf(stderr, "%s - error: name prior to = is empty.\n", __func__);
      return -1;
    }
      
  if (eq - value >= (int) value_len)
    {
      fprintf(stderr, "%s - error: no data after = sign.\n", __func__);
      return -1;
    }
  
  content = eq + 1;
  

#define FORM_CONTENT_TYPE_STR ";type=" 
  
  char* content_type = 0;
  size_t content_type_len = 0;
  int files = 0;
  char* pos_current = content;
  size_t files_number = 1;
  const char *comma = ",";

  if (*content == '@')
    {
      files = 1;
      if (content - value >= (int) value_len)
        {
          fprintf(stderr, "%s - error: no filename after  sign '@'.\n", __func__);
          return -1;
        }
      content += 1;

        /* Count the number of files/commas */
      while (*pos_current && (pos_current = strchr (pos_current, *comma)))
        {
          ++files_number;
          ++pos_current;
        }
    }

  content_type = strstr (content, FORM_CONTENT_TYPE_STR);
  
  if (content_type)
    content_type_len = strlen (content_type);
  
  if (content_type && content_type_len)
  {
      if (content_type_len <= strlen (FORM_CONTENT_TYPE_STR))
      {
          fprintf(stderr, "%s - error: content type, if appears should not be empty.\n", 
                  __func__);
          return -1;
      }
      
      *content_type = '\0'; /* place instead of ';' of ';type=' zero - '\0' */
      content_type = content_type + strlen (FORM_CONTENT_TYPE_STR);
      
  }
  
  if (! files)
  {
      if (content_type)
        {
            if (curl_formadd (&url->mpart_form_post,
                              &url->mpart_form_last, 
                              CURLFORM_COPYNAME, fieldname,
                              CURLFORM_COPYCONTENTS, content,
                              CURLFORM_CONTENTTYPE, content_type, 
                              CURLFORM_END))
            {

                fprintf(stderr, "%s - error: curl_formadd () error - no files and content type exits.\n", 
                  __func__);
                return -1;
            }
        }
      else
        {
          /* Default content-type */
            if (curl_formadd (&url->mpart_form_post,
                              &url->mpart_form_last, 
                              CURLFORM_COPYNAME, fieldname,
                              CURLFORM_COPYCONTENTS, content,
                              CURLFORM_END))
            {
                fprintf(stderr, "%s - error: curl_formadd () error - no files and no content type.\n", 
                  __func__);
                return -1;
            }
        }

      return 0;
    }

  /* Coming here, if content is a file or files 'if (*content == '@')' is TRUE */
  
  // We allow content-type only for a single file. 
  if (files_number == 1)
    {
        if (content_type)
        {
            if (curl_formadd (&url->mpart_form_post,
                              &url->mpart_form_last, 
                              CURLFORM_COPYNAME, fieldname,
                              CURLFORM_FILE, content,
                              CURLFORM_CONTENTTYPE, content_type, 
                              CURLFORM_END))
            {
                fprintf(stderr, "%s - error: curl_formadd () error - one file with content type.\n", 
                  __func__);
                return -1;
            }
        }
        else
        {
            if (curl_formadd (&url->mpart_form_post,
                              &url->mpart_form_last, 
                              CURLFORM_COPYNAME,
                              fieldname,
                              CURLFORM_FILE, content,
                              CURLFORM_END))
            {
                fprintf(stderr, "%s - error: curl_formadd () error - one file without content type.\n", 
                  __func__);
                return -1;
            }
        }
    }
  else if (files_number > 1)
  {
      // Multiple files without content type
      struct curl_forms* forms =  NULL;
      size_t j = 0;

      if (! (forms = calloc (files_number + 1, sizeof (struct curl_forms))))
        {
          fprintf(stderr, "%s - error: calloc of curl_forms failed with errno %d\n",
                  __func__, errno);
          return -1;
        }

      for (j = 0; j < files_number + 1; j++)
      {
          forms [j].option = CURLFORM_END;
      }

      char * token = 0, *strtokp = 0;
      size_t token_index = 0;
      
      for (token = strtok_r (content, comma, &strtokp); 
           token != 0;
           token = strtok_r (0, comma, &strtokp))
        {
          size_t token_len = strlen (token);
          
          if (! token_len)
          {
              fprintf (stderr, "%s - warning: token is empty. \n", __func__);
              return -1;
          }
          else
          {
              forms [token_index].option = CURLFORM_FILE;
              forms [token_index].value  = token;
          }
          
          token_index++;
        }

      if (curl_formadd (&url->mpart_form_post,
                        &url->mpart_form_last, 
                        CURLFORM_COPYNAME, fieldname,
                        CURLFORM_ARRAY, forms, 
                        CURLFORM_END))
      {
          fprintf(stderr, "%s - error: curl_formadd () error - multiple files without content type.\n", 
                  __func__);
          return -1;
      }
    }

  return 0;
}

static int web_auth_method_parser (batch_context*const bctx, char*const value)
{
  url_context* url = &bctx->url_ctx_array[bctx->url_index];

  if (!strcmp (value, NON_APPLICABLE_STR))
    {
      url->web_auth_method = AUTHENTICATION_NO;
      return 0;
    }

  if (!strcmp (value, AUTH_BASIC))
      url->web_auth_method = AUTHENTICATION_BASIC;
  else if (!strcmp (value, AUTH_DIGEST))
      url->web_auth_method = AUTHENTICATION_DIGEST;
  else if (!strcmp (value, AUTH_GSS_NEGOTIATE))
    url->web_auth_method = 
      AUTHENTICATION_GSS_NEGOTIATE;
  else if (!strcmp (value, AUTH_NTLM))
    url->web_auth_method = AUTHENTICATION_NTLM;
  else if (!strcmp (value, AUTH_ANY))
    url->web_auth_method = AUTHENTICATION_ANY;
  else
    {
      fprintf (stderr, 
               "\n%s - error: WEB_AUTH_METHOD (%s) is not valid. \n"
               "Please, use: %s, %s \n" "%s, %s, %s\n",
               __func__, value, AUTH_BASIC, AUTH_DIGEST, 
               AUTH_GSS_NEGOTIATE, AUTH_NTLM, AUTH_ANY);
      return -1;
    }
    
  return 0;
}
static int web_auth_credentials_parser (batch_context*const bctx, char*const value)
{
  size_t string_len = 0;

  if (! (string_len = strlen (value)))
    {
      fprintf(stderr, "%s - warning: empty WEB_AUTH_CREDENTIALS\n", 
              __func__);
      return 0;
    }

  string_len++;
  
  if (!(bctx->url_ctx_array[bctx->url_index].web_auth_credentials = 
       (char *) calloc (string_len, sizeof (char))))
    {
      fprintf(stderr, 
                  "%s error: failed to allocate memory for WEB_AUTH_CREDENTIALS" 
                  "with errno %d.\n",  __func__, errno);
      return -1;
    }

  const char separator = ':';
  if (!strchr (value, separator))
    {
      fprintf(stderr, 
                  "%s error: separator (%c) of username and password to be "
              "present in the credentials string \"%s\"\n", 
              __func__, separator, value);
      return -1;
    }

  strncpy (bctx->url_ctx_array[bctx->url_index].web_auth_credentials, 
           value, 
           string_len -1);
  
  return 0;
}
static int proxy_auth_method_parser (batch_context*const bctx, char*const value)
{
  url_context* url = &bctx->url_ctx_array[bctx->url_index];

  if (!strcmp (value, NON_APPLICABLE_STR))
    {
      url->proxy_auth_method = AUTHENTICATION_NO;
      return 0;
    }

  if (!strcmp (value, AUTH_BASIC))
      url->proxy_auth_method = AUTHENTICATION_BASIC;
  else if (!strcmp (value, AUTH_DIGEST))
      url->proxy_auth_method = AUTHENTICATION_DIGEST;
  else if (!strcmp (value, AUTH_GSS_NEGOTIATE))
    url->proxy_auth_method = AUTHENTICATION_GSS_NEGOTIATE;
  else if (!strcmp (value, AUTH_NTLM))
    url->proxy_auth_method = AUTHENTICATION_NTLM;
  else if (!strcmp (value, AUTH_ANY))
    url->proxy_auth_method = AUTHENTICATION_ANY;
  else
    {
      fprintf (stderr, 
               "\n%s - error: PROXY_AUTH_METHOD (%s) is not valid. \n"
               "Please, use: %s, %s \n" "%s, %s, %s\n",
               __func__, value, AUTH_BASIC, AUTH_DIGEST, AUTH_GSS_NEGOTIATE,
               AUTH_NTLM, AUTH_ANY);
      return -1;
    }
  return 0;
}
static int proxy_auth_credentials_parser (batch_context*const bctx, char*const value)
{
  size_t string_len = 0;

  if (! (string_len = strlen (value)))
    {
      fprintf(stderr, "%s - warning: empty PROXY_AUTH_CREDENTIALS\n", 
              __func__);
      return 0;
    }

  string_len++;
  
  if (! (bctx->url_ctx_array[bctx->url_index].proxy_auth_credentials = 
       (char *) calloc (string_len, sizeof (char))))
    {
      fprintf(stderr, 
                  "%s error: failed to allocate memory for PROXY_AUTH_CREDENTIALS" 
                  "with errno %d.\n",  __func__, errno);
      return -1;
    }

  const char separator = ':';
  if (!strchr (value, separator))
    {
      fprintf(stderr, 
                  "%s error: separator (%c) of username and password to be "
              "present in the credentials string \"%s\"\n", 
              __func__, separator, value);
      return -1;
    }

  strncpy (bctx->url_ctx_array[bctx->url_index].proxy_auth_credentials, 
           value, 
           string_len -1);

  return 0;
}

static int fresh_connect_parser (batch_context*const bctx, char*const value)
{
    long boo = atol (value);

    if (boo < 0 || boo > 1)
    {
        fprintf(stderr, 
                "%s error: boolean input 0 or 1 is expected\n", __func__);
        return -1;
    }
    bctx->url_ctx_array[bctx->url_index].fresh_connect = boo;
    return 0;
}

static int timer_tcp_conn_setup_parser (batch_context*const bctx, char*const value)
{
    long timer = atol (value);

    if (timer <= 0 || timer > 50)
    {
        fprintf(stderr, 
                "%s error: input of the timer is expected  to be from " 
		"1 up to 50 seconds.\n", __func__);
        return -1;
    }
    bctx->url_ctx_array[bctx->url_index].connect_timeout= timer;
    return 0;
}
static int timer_url_completion_parser (batch_context*const bctx, 
					char*const value)
{
  long timer_lrange = 0;
  long timer_hrange = 0;
  size_t value_len = strlen (value) + 1;

  if (parse_timer_range (value,
                         value_len,
                         &timer_lrange,
                         &timer_hrange) == -1)
    {
      fprintf(stderr, "%s error: parse_timer_range () failed.\n", __func__);
      return -1;
    }
  
  if (!timer_hrange && timer_lrange > 0 && timer_lrange < 20)
    {
      fprintf(stderr, 
              "%s error: the timer should be either 0 or 20 msec and more, not %ld.\n"
              "Note, that since version 0.31 the timer is in msec and enforced by\n"
              "monitoring time of each url fetching and cancelling, when it \n"
              "takes msec above the timer. Operation statistics provides a view\n"
              "on what happens as URL-Timed out and statistics T-Err counter.\n"
              "To preserve the previous behavior without the timer enforcement,\n"
              "please, place 0 value here.\n",
              __func__, timer_lrange);
      return -1;
    }

    bctx->url_ctx_array[bctx->url_index].timer_url_completion_lrange = 
      timer_lrange;

    bctx->url_ctx_array[bctx->url_index].timer_url_completion_hrange = 
      timer_hrange;

    return 0;
}
static int timer_after_url_sleep_parser (batch_context*const bctx, 
					 char*const value)
{
  long timer_lrange = 0;
  long timer_hrange = 0;
  size_t value_len = strlen (value) + 1;
  
  if (parse_timer_range (value,
                         value_len,
                         &timer_lrange,
                         &timer_hrange) == -1)
    {
      fprintf(stderr, "%s error: parse_timer_range () failed.\n", __func__);
      return -1;
    }
  
    if (!timer_hrange && timer_lrange > 0 && timer_lrange < 20)
      {
        fprintf(stderr, 
                "%s error: the timer should be either 0 or 20 msec and more.\n",
                __func__);
        return -1;
      }

    bctx->url_ctx_array[bctx->url_index].timer_after_url_sleep_lrange = 
      timer_lrange;

    bctx->url_ctx_array[bctx->url_index].timer_after_url_sleep_hrange = 
      timer_hrange;

    return 0;
}

static int ftp_active_parser (batch_context*const bctx, char*const value)
{
  long status = atol (value);
  if (status < 0 || status > 1)
    {
      fprintf(stderr, "%s error: ether 0 or 1 are allowed.\n", __func__);
      return -1;
    }
  bctx->url_ctx_array[bctx->url_index].ftp_active = status;
  return 0;
}

static int log_resp_headers_parser (batch_context*const bctx, char*const value)
{
  long status = atol (value);
  if (status < 0 || status > 1)
    {
      fprintf(stderr, "%s error: ether 0 or 1 are allowed.\n", __func__);
      return -1;
    }
  bctx->url_ctx_array[bctx->url_index].log_resp_headers = status;
  return 0;
}
static int log_resp_bodies_parser (batch_context*const bctx, char*const value)
{
  long status = atol (value);
  if (status < 0 || status > 1)
    {
      fprintf(stderr, "%s error: ether 0 or 1 are allowed.\n", __func__);
      return -1;
    }
  bctx->url_ctx_array[bctx->url_index].log_resp_bodies = status;
  return 0;
}

static int response_status_errors_parser (batch_context*const bctx, 
                                          char*const value)
{
  url_context* url = &bctx->url_ctx_array[bctx->url_index];
 
  /* Allocate the table */
  if (! (url->resp_status_errors_tbl = 
         calloc (URL_RESPONSE_STATUS_ERRORS_TABLE_SIZE, 
                 sizeof (unsigned char))))
    {
      fprintf (stderr, "%s - error: calloc () failed with errno %d.\n", 
              __func__, errno);
      return -1;
    }

  /* Copy the default table */
  memcpy (url->resp_status_errors_tbl, 
          resp_status_errors_tbl_default,
          URL_RESPONSE_STATUS_ERRORS_TABLE_SIZE);

  long status = 0;
  const char *separator =  ",";
  char * token = 0, *strtokp = 0;
  size_t token_len = 0;

  for (token = strtok_r (value, separator, &strtokp); 
       token != 0;
       token = strtok_r (0, separator, &strtokp))
    {
      if ((token_len = strlen (token)) < 2)
        {
          fprintf (stderr, "%s - error: token %s is too short.\n"
                   "Each valid token should start from + or - with a following "
                   "response status number.\n", __func__, token);
          return -1;
        }

      if (*token != '+' && *token != '-')
        {
          fprintf (stderr, "%s - error: token %s does not have leading + or - symbol.\n"
                   "Each valid token should start from + or - with a following "
                   "response status number.\n", __func__, token);
          return -1;
        }

      status = atol (token + 1);

      if (status < 0 || status > URL_RESPONSE_STATUS_ERRORS_TABLE_SIZE)
        {
          fprintf (stderr, "%s - error: token %s non valid.\n"
                   "Each valid token should start from + or - with a following "
                   "response status number in the range from 0 up to %d.\n", 
                   __func__, token, URL_RESPONSE_STATUS_ERRORS_TABLE_SIZE);
          return -1;
        }

      if (*token == '+')
        {
          url->resp_status_errors_tbl[status] = 1;
        }
      else if (*token == '-')
        {
          url->resp_status_errors_tbl[status] = 0;
        }
    }

  return 0;
}

static int transfer_limit_rate_parser (batch_context*const bctx, char*const value)
{
  long rate = atol (value);

  if (rate < 0)
    {
      fprintf(stderr, "%s error: negative rate is not allowed.\n", __func__);
      return -1;
    }

  bctx->url_ctx_array[bctx->url_index].transfer_limit_rate = (curl_off_t) rate;
  return 0;
}

static int fetch_probability_parser (batch_context*const bctx, char*const value)
{
  long probability = atol (value);

  if (probability < 1 || probability > 100)
    {
      fprintf(stderr, "%s error: tag FETCH_PROBABILITY should be with "
              "from 1 up to 100.\n", __func__);
      return -1;
    }

  bctx->url_ctx_array[bctx->url_index].fetch_probability = (int) probability;
  return 0;

}

static int fetch_probability_once_parser (batch_context*const bctx, 
                                          char*const value)
{
  long probability_once = atol (value);

  if (probability_once != 0 && probability_once != 1)
    {
      fprintf (stderr, 
              "%s error: tag FETCH_PROBABILITY_ONCE can be either 0 or 1.\n", 
              __func__);
      return -1;
    }

  bctx->url_ctx_array[bctx->url_index].fetch_probability_once = 
    (int) probability_once;
  return 0;
}

/******************************************************************************
* Function name - url_schema_classification
*
* Description - Makes url analyses to return the type (e.g. http, ftps, telnet, etc)
* 
* Input -      *url - pointer to the url context
* Return Code/Output - On success - a url schema type, on failure - 
*                    (URL_APPL_UNDEF)
*******************************************************************************/
static url_appl_type 
url_schema_classification (const char* const url)
{
  if (!url)
    {
      return  URL_APPL_UNDEF;
    }

#define HTTPS_SCHEMA_STR "https://"
#define HTTP_SCHEMA_STR "http://"
#define FTPS_SCHEMA_STR "ftps://"
#define FTP_SCHEMA_STR "ftp://"
#define SFTP_SCHEMA_STR "sftp://"
#define TELNET_SCHEMA_STR "telnet://"

  if (strstr (url, HTTPS_SCHEMA_STR))
    return URL_APPL_HTTPS;
  else if (strstr (url, HTTP_SCHEMA_STR))
    return URL_APPL_HTTP;
  else if (strstr (url, FTPS_SCHEMA_STR))
    return URL_APPL_FTPS;
  else if (strstr (url, FTP_SCHEMA_STR))
    return URL_APPL_FTP;
  else if (strstr (url, SFTP_SCHEMA_STR))
    return URL_APPL_SFTP;
  else if (strstr (url, TELNET_SCHEMA_STR))
    return URL_APPL_TELNET;

  return  URL_APPL_UNDEF;
}


/******************************************************************************
* Function name - validate_batch
*
* Description - Validates all parameters in the batch. Calls validation 
*               functions for all sections.
* 
* Input -      *bctx - pointer to the initialized batch context to validate
* Return Code/Output - On success - 0, on failure - (-1)
*******************************************************************************/
static int validate_batch (batch_context*const bctx)
{
    if (validate_batch_general (bctx) == -1)
    {
        fprintf (stderr, "%s - error: failed to validate batch section GENERAL.\n", 
                 __func__);
        return -1;
    }

    if (validate_batch_url (bctx) == -1)
    {
        fprintf (stderr, "%s - error: failed to validate batch section URL.\n", 
                 __func__);
        return -1;
    }

    return 0;
}

/******************************************************************************
* Function name - validate_batch_general
*
* Description - Validates section general parameters
* 
* Input -       *bctx - pointer to the initialized batch context to validate
* Return Code/Output - On success - 0, on failure - (-1)
********************************************************************************/
static int validate_batch_general (batch_context*const bctx)
{
    if (!strlen (bctx->batch_name))
    {
        fprintf (stderr, "%s - error: BATCH_NAME is empty.\n", __func__);
        return -1;
    }
    if (bctx->client_num_max < 1)
    {
        fprintf (stderr, "%s - error: CLIENT_NUM_MAX is less than 1.\n", __func__);
        return -1;
    }
    if (bctx->client_num_start < 0)
    {
        fprintf (stderr, "%s - error: CLIENT_NUM_START is less than 0.\n", 
		__func__);
        return -1;
    }
    if (bctx->client_num_start > bctx->client_num_max)
      {
        fprintf (stderr, 
                 "%s - error: CLIENT_NUM_START (%d) is less than CLIENT_NUM_MAX (%d).\n", 
                 __func__, bctx->client_num_start, bctx->client_num_max);
        return -1;
      }
    if (bctx->clients_rampup_inc < 0)
    {
        fprintf (stderr, "%s - error: CLIENTS_RAMPUP_INC is negative.\n",__func__);
        return -1;
    }

    // TODO: validate the existence of the network interface
    if (!strlen (bctx->net_interface))
    {
        fprintf (stderr, "%s - error: INTERFACE name is empty.\n", __func__);
        return -1;
    }

    if (bctx->ipv6)
      {
        if (bctx->cidr_netmask < 1 || bctx->cidr_netmask > 128)
          {
            fprintf (stderr, 
                     "%s - error: IPv6 network mask (%d) is out of the range\n", 
                     __func__, bctx->cidr_netmask);
            return -1;
          }
      }
    else
      {
        if (bctx->cidr_netmask < 1 || bctx->cidr_netmask > 32)
          {
            fprintf (stderr, 
                     "%s - error: IPv4 network mask (%d) is out of the range\n", 
                     __func__, bctx->cidr_netmask);
            return -1;
          }
      }

    if (! bctx->ipv6)
      {
        if (bctx->ip_addr_min && (bctx->ip_addr_min == bctx->ip_addr_max))
          {
            bctx->ip_shared_num =1;
          }
        else
          {
            if (!bctx->ip_shared_num &&
                ((bctx->ip_addr_max - bctx->ip_addr_min + 1) < bctx->client_num_max))
              {
                fprintf (stderr, "%s - error: range of IPv4 addresses is less than number of clients.\n"
                         "Please, increase IP_ADDR_MAX.\n", __func__);
                return -1;
              }
          }
      }
    else
      {
        // IPv6
        if (! memcmp (&bctx->ipv6_addr_min, 
                      &bctx->ipv6_addr_max, 
                      sizeof (bctx->ipv6_addr_min)))
          {
            bctx->ip_shared_num =1;
          } 
      }

    if (bctx->cycles_num < 1)
    {
        fprintf (stderr, "%s - error: CYCLES_NUM is less than 1.\n"
                 "To cycle or not to cycle - this is the question.\n",__func__);
        return -1;
    }

    if (!strlen (bctx->user_agent))
    {
        /* user-agent not provided, taking the defaults */
        strncpy (bctx->user_agent, 
                 EXPLORER_USERAGENT_STR, 
                 sizeof (bctx->user_agent) -1);
    }

    if (bctx->req_rate > bctx->client_num_max)
    {
        fprintf (stderr, "%s - error: REQ_RATE exceeds CLIENTS_NUM_MAX.\n",
                 __func__);
        return -1;
    }
  
    return 0;
}

/******************************************************************************
* Function name - validate_batch_url
*
* Description - Validates section URL parameters
* 
* Input -       *bctx - pointer to the initialized batch context to validate
* Return Code/Output - On success - 0, on failure - (-1)
*******************************************************************************/
static int validate_batch_url (batch_context*const bctx)
{
  if (bctx->urls_num < 1)
    {
      fprintf (stderr, "%s - error: at least a single url is expected "
               "for a valid URL section .\n", __func__);
      return -1;
    }

  int noncycling_cycling = 0, cycling_noncycling = 0;
  int prev_url_cycling = 0;

  int k = 0;
  for (k = 0; k < bctx->urls_num; k++)
    {
      url_context* url = &bctx->url_ctx_array[k];

      /*
        Test, that HTTP methods (req-types) are GET, POST or PUT
      */
      const url_appl_type url_type = url->url_appl_type;
      const int req_type = url->req_type;
      
      if (url_type == URL_APPL_HTTP || url_type == URL_APPL_HTTPS)
        {
          if (req_type < HTTP_REQ_TYPE_FIRST || 
              req_type > HTTP_REQ_TYPE_LAST)
            {
              fprintf (stderr, "%s - error: REQUEST_TYPE is out of valid range .\n", 
                       __func__);
              return -1;
            }

          /* 
             HTTP POST-ing requires either FORM_STRING or 
             MULTIPART_FORM_DATA tags.
          */ 
          if (req_type == HTTP_REQ_TYPE_POST)
            {
                /*
              if (!url->form_str && !url->mpart_form_post)
                {
                  fprintf (stderr, "%s - error: either FORM_STRING or "
                           "MULTIPART_FORM_DATA tags should be defined to "
                           "make HTTP POST\n", __func__);
                  return -1;   
                }
                */

              if (url->form_str && url->mpart_form_post)
                {
                  fprintf (stderr, "%s - error: either FORM_STRING or "
                           "MULTIPART_FORM_DATA tags, but not the both, should be "
                           "defined to make HTTP POST\n", __func__);
                  return -1;   
                }
             }
        }
      
      if (url->form_records_file && !url->form_str)
        {
          fprintf (stderr, "%s - error: empty FORM_STRING, "
                   "when FORM_RECORDS_FILE defined.\n Either disable" 
                   "FORM_RECORDS_FILE or define FORM_STRING\n", __func__);
          return -1;
        }
      
      /*
        Test, that there is only a single continues area of cycling URLs 
        in meanwhile, like this:
        
        don't-cycle - URL;
        cycle -URL;
        cycle -URL;
        don't-cycle - URL;
        don't-cycle - URL;
        
        We are not supporting several regions of cycling right now, like that
        don't-cycle - URL;
        cycle -URL;
        don't-cycle - URL; separates cycling area.
        cycle -URL;
        don't-cycle - URL;
      */
      if (k)
      {
          if (prev_url_cycling && url->url_dont_cycle)
          {
              cycling_noncycling++;
          }
          else if (!prev_url_cycling && !url->url_dont_cycle)
          {
              noncycling_cycling++;
          }
      }

      // Remember this url cycling status to prev_url_cycling tobe used the next time
      prev_url_cycling = url->url_dont_cycle ? 0 : 1;
      
      
      if (! url->url_use_current)
        {
          /*
            Check non-empty URL, when URL_USE_CURRENT is not defined.
          */
          if (!url->url_str || ! url->url_str_len)
            {
              fprintf (stderr, 
                       "%s - error: empty URL in position %d.\n", __func__, k);
              return -1;
            }
        }
      else
        {
          /*
             URL_USE_CURRENT cannot appear in the first URL, 
             nothing is current.
          */
          if (0 == k)
            {
              fprintf (stderr, 
                       "%s - error: empty URL with URL_USE_CURRENT " 
                       "defined cannot appear as the very first url.\n"
                       "There is no any url to take as \"current\"\n", __func__);
              return -1;
            }
          
          /*
              Test, that cycling or not-cycling status of the 
              CURRENT_URLs is the same as for the primary-URL
            */
          int m;
          for (m = k - 1; m >= 0; m--)
            {
              url_context* url_m = &bctx->url_ctx_array[m];

              if (url_m->url_dont_cycle != url_m->url_dont_cycle)
                {
                  fprintf (stderr, 
                           "%s - error: cycling of the primary url and all urls "
                           "afterwards with URL_USE_CURRENT defined should be the same.\n" 
                           "Check tags URL_DONT_CYCLE values. Either cycle or don't cycle\n"
                           "for both the primary and the \"use current\" urls.\n", __func__);
                  return -1;
                }

              if (! url_m->url_use_current)
                break;
            }
        } /* else */

    }

  if (cycling_noncycling > 1 || noncycling_cycling > 1)
  {
      fprintf (stderr, 
               "%s - error: this version supports only a single cycling area.\n"
               "Several non-cycling urls can be before and/or after this cycling area, \n"
               "e.g. for login and logoff purposes\n", 
               __func__);
      return -1;
  }

  return 0;
}

/******************************************************************************
* Function name - create_response_logfiles_dirs
*
* Description - When at least a single URL configuration requires logging of
*                    responses, creates a directory with the same name as for the batch. 
* 
* Input -      *bctx - pointer to the initialized batch context to validate
* Return Code/Output - On success - 0, on failure - (-1)
*******************************************************************************/
int create_response_logfiles_dirs (batch_context* bctx)
{
  int dir_created_flag = 0;
  char dir_log_resp[256];
  const mode_t mode= S_IRWXU|S_IRWXG|S_IRWXO;
  
  memset (dir_log_resp, 0, sizeof (dir_log_resp));

  snprintf (dir_log_resp, sizeof (dir_log_resp) -1, 
                  "./%s", bctx->batch_name);

  int k = 0;
  for (k = 0; k < bctx->urls_num; k++)
    {
      url_context* url = &bctx->url_ctx_array[k];

      if (url->log_resp_bodies || url->log_resp_headers)
        {

          /* Create the directory, if not created before. */
          if (!dir_created_flag)
            {  
              if(mkdir (dir_log_resp, mode) == -1 && errno !=EEXIST )
                {
                  fprintf (stderr, 
                           "%s - error: mkdir () failed with errno %d to create dir \"%s\".\n",
                           __func__, errno, dir_log_resp);
                  return -1;
                }
              dir_created_flag =1;
            }

          /* 
             Create subdirs for responses logfiles, if configured.
          */
          memset (dir_log_resp, 0, sizeof (dir_log_resp));

          snprintf (dir_log_resp, sizeof (dir_log_resp) -1, 
                    "./%s/url%ld/", 
                    bctx->batch_name, 
                    url->url_ind);

          if (mkdir (dir_log_resp, mode) == -1 && errno !=EEXIST )
            {
              fprintf (stderr, "%s - error: mkdir () failed with errno %d.\n",
                       __func__, errno);
              return -1;
            }

          const size_t dir_log_len = strlen (dir_log_resp) + 1;

          if (! (url->dir_log = calloc (dir_log_len, sizeof (char))))
            {
              fprintf (stderr, "%s - error:  calloc () failed with errno %d.\n",
                       __func__, errno);
              return -1;
            }
          else
            {
              strncpy (url->dir_log, dir_log_resp, dir_log_len -1);
            }
        }
    }
  return 0;
}

/******************************************************************************
* Function name - alloc_client_formed_buffers
*
* Description - Allocates client formed buffers to be used for POST-ing. Size of the
*                    buffers is taken as a maximum possible lenght. The buffers will be
*                    initialized for POST-ing for each URL, that is using POST, and 
*                    according to the detailed URL-based configuration, tokens, 
*                    form-type, etc.
* 
* Input -      *bctx - pointer to the initialized batch context to validate
* Return Code/Output - On success - 0, on failure - (-1)
*******************************************************************************/
int alloc_client_formed_buffers (batch_context* bctx)
{
  int k = 0;

  for (k = 0; k < bctx->urls_num; k++)
    {
      url_context* url = &bctx->url_ctx_array[k];
      
      /*
        Allocate posting buffers for clients (login, logoff other posting), 
        if at least a single url contains method HTTP POST and 
        FORM_STRING.
      */
      if (url->req_type == HTTP_REQ_TYPE_POST && url->form_str)
        {
          int i;
          for (i = 0;  i < bctx->client_num_max; i++)
            {
              client_context* cctx = &bctx->cctx_array[i];
              
              if (!cctx->post_data && !cctx->post_data_len)
                {
                  size_t form_string_len = strlen (url->form_str);
                  
                  if (form_string_len)
                    {
                      cctx->post_data_len = form_string_len + 1 +
                        FORM_RECORDS_MAX_TOKENS_NUM*
                        (FORM_RECORDS_TOKEN_MAX_LEN +
                         FORM_RECORDS_SEQ_NUM_LEN);
                      
                      if (! (cctx->post_data = 
                             (char *) calloc (cctx->post_data_len, sizeof (char))))
                        {
                          fprintf (stderr,
                                   "\"%s\" error: failed to allocate client "
				   "post_data buffer.\n", 
                                   __func__) ;
                          return -1;
                        }
                    }
                }
            }
        } /* end of post-ing buffers allocation */
      
      else if ((url->req_type == HTTP_REQ_TYPE_GET && url->form_str) ||  
			  (url->req_type == HTTP_REQ_TYPE_HEAD && url->form_str) || 
			  (url->req_type == HTTP_REQ_TYPE_DELETE && url->form_str))
        {
          int j;
          for (j = 0;  j < bctx->client_num_max; j++)
            {
              client_context* cctx = &bctx->cctx_array[j];
              
              if (!cctx->get_url_form_data && !cctx->get_url_form_data_len)
                {
                  size_t form_string_len = strlen (url->form_str);
                  
                  if (form_string_len)
                    {
                      cctx->get_url_form_data_len = url->url_str_len + 
		        form_string_len + 1 +
                        FORM_RECORDS_MAX_TOKENS_NUM*
                        (FORM_RECORDS_TOKEN_MAX_LEN + FORM_RECORDS_SEQ_NUM_LEN);
                      
                      if (! (cctx->get_url_form_data = 
                             (char *) calloc (cctx->get_url_form_data_len, sizeof (char))))
                        {
                          fprintf (stderr,
                                   "\"%s\" error: failed to allocate client "
				   "get_url_form_data buffer.\n", __func__) ;
                          return -1;
                        }
                    }
                }
            }
        } /* end of get-url-form buffers allocation */
      
    }

  return 0;
}

/******************************************************************************
* Function name - alloc_client_fetch_decision_array
*
* Description - Allocates client URL fetch decision arrays to be used, when 
*                    fetching decision to be done only during the first cycle 
*                    and remembered (in fetch_decision_array).
* 
* Input -      *bctx - pointer to the initialized batch context to validate
* Return Code/Output - On success - 0, on failure - (-1)
*******************************************************************************/
int alloc_client_fetch_decision_array (batch_context* bctx)
{
  int k = 0;
  
  for (k = 0; k < bctx->urls_num; k++)
  {
      url_context* url = &bctx->url_ctx_array[k];
      
      if (url->fetch_probability && url->fetch_probability_once)
      {
          int i;
          for (i = 0;  i < bctx->client_num_max; i++)
          {
              client_context* cctx = &bctx->cctx_array[i];
              
              if (!cctx->url_fetch_decision)
              {
                  if (!(cctx->url_fetch_decision = calloc (bctx->urls_num, sizeof (char))))
                  {
                      fprintf (stderr, "\"%s\" error: failed to allocate client url_fetch_decision buffer.\n", __func__) ;
                      return -1;
                  }
                  memset (cctx->url_fetch_decision, -1, bctx->urls_num);
              }
          }
      }
  }
  return 0;
}

/******************************************************************************
* Function name - init_operational_statistics
*
* Description - Allocates and inits operational statistics structures.
* 
* Input -      *bctx - pointer to the initialized batch context to validate
* Return Code/Output - On success - 0, on failure - (-1)
*******************************************************************************/
int init_operational_statistics(batch_context* bctx)
{
  if (op_stat_point_init(&bctx->op_delta,
                         bctx->urls_num) == -1)
    {
      fprintf (stderr, "%s - error: init of op_delta failed.\n",__func__);
      return -1;
    }
  
  if (op_stat_point_init(&bctx->op_total,
                         bctx->urls_num) == -1)
    {
      fprintf (stderr, "%s - error: init of op_total failed.",__func__);
      return -1;
    }

  return 0;
}


/******************************************************************************
* Function name - post_validate_init
*
* Description - Performs post validate initializations of a batch context.
* 
* Input -       *bctx - pointer to the initialized batch context to validate
* Return Code/Output - On success - 0, on failure - (-1)
*******************************************************************************/
static int post_validate_init (batch_context*const bctx)
{
  /*
    Allocate client contexts, if not allocated before.
  */
  if (!bctx->cctx_array)
    {
      if (!(bctx->cctx_array =
            (client_context *) cl_calloc (bctx->client_num_max, 
                                          sizeof (client_context))))
        {
          fprintf (stderr, "\"%s\" - %s - failed to allocate cctx.\n", 
                   bctx->batch_name, __func__);
          return -1;
        }
      if (bctx->req_rate)
        {
          /*
            Allocate list of free clients
          */
          if (!(bctx->free_clients =
            (int *) calloc (bctx->client_num_max, sizeof (int))))
            {
              fprintf (stderr,
                "\"%s\" - %s - failed to allocate free client list.\n", 
                bctx->batch_name, __func__);
              return -1;
            }
	        else
            {
               /*
                 Initialize the list, all clients are free
                 Fill the list in reverse, last memeber is picked first
               */
               bctx->free_clients_count = bctx->client_num_max;
               int ix = bctx->free_clients_count, client_num = 1;
	             while (ix-- > 0)
                  bctx->free_clients[ix] = client_num++;
            }
        }
    }

  if (create_response_logfiles_dirs (bctx) == -1)
    {
      fprintf (stderr, 
               "\"%s\" - create_response_logfiles_dirs () failed .\n", 
               __func__);
      return -1;
    }

  if (alloc_client_formed_buffers (bctx) == -1)
    {
      fprintf (stderr, 
               "\"%s\" - alloc_client_formed_buffers () failed .\n", 
               __func__);
      return -1;
    }

  if (alloc_client_fetch_decision_array (bctx) == -1)
    {
      fprintf (stderr, 
               "\"%s\" - alloc_client_fetch_decision_array () failed .\n", 
               __func__);
      return -1;
    }
 
  if (init_operational_statistics (bctx) == -1)
    {
      fprintf (stderr, 
               "\"%s\" - init_operational_statistics () failed .\n", 
               __func__);
      return -1;
    }

  /* 
     It should be the last check.
  */
  fprintf (stderr, "\nThe configuration has been post-validated successfully.\n\n");

  /* 
     Check, that this configuration has cycling. 
     Namely, at least a single url is without tag URL_DONT_CYCLE
  */
  const int first_cycling_url = find_first_cycling_url (bctx);

  find_last_cycling_url (bctx);

  if (first_cycling_url < 0)
    {
      fprintf (stderr, "The configuration has not cycling urls defined.\n"
               "Are you sure, that this is what you are planning to do?\n"
               "To make cycling you may wish to remove tag URL_DONT_CYCLE \n"
               "or set it to zero for the urls that you wish to run in cycle.\n"
               " Please, press ENTER to continue or Cntl-C to stop.\n");
             
      getchar ();
    }
  
  return 0;
}

/*******************************************************************************
* Function name - set_default_response_errors_table
*
* Description - Inits by defaults the response codes, considered as errors.
*                          
* Return Code/Output - None
********************************************************************************/
static  void set_default_response_errors_table ()
{
  memset (&resp_status_errors_tbl_default, 
          0, 
          sizeof (resp_status_errors_tbl_default));

  memset (resp_status_errors_tbl_default + 400, 
          1,
          sizeof (resp_status_errors_tbl_default) - 400);

  resp_status_errors_tbl_default[401] = 0;
  resp_status_errors_tbl_default[407] = 0;
}

/*******************************************************************************
* Function name - parse_config_file
*
* Description - Parses configuration file and fills loading batch contexts in array
*
* Input -      *filename       - name of the configuration file to parse.
* Output -     *bctx_array     - array of batch contexts to be filled on parsing
* Input-       bctx_array_size - number of bctx contexts in <bctx_array>
*                          
* Return Code/Output - On Success - number of batches >=1, on Error -1
********************************************************************************/
int parse_config_file (char* const filename, 
                       batch_context* bctx_array, 
                       size_t bctx_array_size)
{
  char fgets_buff[1024*8];
  FILE* fp;
  struct stat statbuf;
  int batch_index = -1;

  /* Check, if the configuration file exists. */
  if (stat (filename, &statbuf) == -1)
    {
      fprintf (stderr,
               "%s - failed to find configuration file \"%s\" with errno %d.\n"
               "If you are using example configurations, note, that "
	       	"directory \"configs\" have been renamed to \"conf-examples\".", 
               __func__, filename, errno);
      return -1;
    }

  if (!(fp = fopen (filename, "r")))
    {
      fprintf (stderr, 
               "%s - fopen() failed to open for reading filename \"%s\", errno %d.\n", 
               __func__, filename, errno);
      return -1;
    }

  set_default_response_errors_table ();

     /* for compatibility with older configurations set default value
        of dump_opstats to 1 ("yes") */
  unsigned i;
  for (i = 0; i < bctx_array_size; i++)
     bctx_array[i].dump_opstats = 1;   

  int line_no = 0;
  while (fgets (fgets_buff, sizeof (fgets_buff) - 1, fp))
    {
      // fprintf (stderr, "%s - processing file string \"%s\n", 
      //         __func__, fgets_buff);

      char* string_buff = NULL;
      size_t string_len = 0;
      line_no++;

      if ((string_len = strlen (fgets_buff)) && 
          (string_buff = eat_ws (fgets_buff, &string_len)))
        {

          if ((batch_index + 1) >= (int) bctx_array_size)
            {
              fprintf(stderr, "%s - error: maximum batches limit (%Zu) reached \n", 
                      __func__, bctx_array_size);
              fclose (fp);
              return -1 ;
            }

          /* Line may be commented out by '#'.*/
          if (fgets_buff[0] == '#')
            {
              // fprintf (stderr, "%s - skipping commented file string \"%s\n", 
              //         __func__, fgets_buff);
              continue;
            }

          if (add_param_to_batch (fgets_buff,
                                  string_len,
                                  bctx_array, 
                                  &batch_index) == -1)
            {
              fprintf (stderr, 
                       "%s - error: add_param_to_batch () failed processing buffer \"%s\"\n",
                       __func__, fgets_buff);
              fclose (fp);
              return -1 ;
            }
        }
    }

  fclose (fp);

  if (! (batch_index + 1))
    {
      fprintf (stderr, 
               "%s - error: failed to load even a single batch.\n",__func__);
      return -1;
    }
  else
    {
      fprintf (stderr, "%s - loaded %d batches\n", 
               __func__, batch_index + 1);
    }

  int k = 0;
  for (k = 0; k < batch_index + 1; k++)
    {
      /* Validate batch configuration */
      if (validate_batch (&bctx_array[k]) == -1)
        {
          fprintf (stderr, 
                   "%s - error: validation of batch %d failed.\n",__func__, k);
          return -1;
        }

      if (post_validate_init (&bctx_array[k]) == -1)
        {
          fprintf (stderr, 
                   "%s - error: post_validate_init () for batch %d failed.\n",
		   __func__, k);
          return -1;
        }
    }

  /* Initialize random number generator */
  if (random_seed < 0)
    {
      struct timeval  tval;
      if (gettimeofday (&tval, NULL) == -1)
        {
          fprintf(stderr, "%s - gettimeofday () failed with errno %d.\n", 
            __func__, errno);
          return -1;
        }
        random_seed = tval.tv_sec * tval.tv_usec;
    }
  (void)initstate((unsigned int)random_seed,random_state,sizeof(random_state));

  return (batch_index + 1);
}


/*******************************************************************************
* Function name - load_form_records_file
*
* Description - Itializes client post form buffers, using credentials loaded 
* 		from file. To be called after batch context validation.
*
* Input -       *bctx - pointer to the batch context
*                          
* Return Code/Output - On Success - number of batches >=1, on Error -1
********************************************************************************/
static int load_form_records_file (batch_context*const bctx, url_context* url)
{
  char fgets_buff[512];
  FILE* fp;
  char* sep = 0; // strtok_r requires a string with '\0' 

  /* 
     Open the file with form records 
  */
  if (!(fp = fopen (url->form_records_file, "r")))
    {
      fprintf (stderr, 
               "%s - fopen() failed to open for reading filename \"%s\", errno %d.\n", 
               __func__, url->form_records_file, errno);
      return -1;
    }

  const size_t max_records = url->form_records_file_max_num ? 
    url->form_records_file_max_num : (size_t)bctx->client_num_max;

  /* 
     Allocate the place to keep form records tokens for clients
  */
  if (! (url->form_records_array =  calloc (max_records, 
                                            sizeof (form_records_cdata))))
  {
      fprintf (stderr, "%s - failed to allocate memory for "
               "url->form_records_array with errno %d.\n", __func__, errno);
      return -1;
  }

  while (fgets (fgets_buff, sizeof (fgets_buff) - 1, fp))
    {
      fprintf (stderr, "%s - processing form records file string \"%s\n", 
               __func__, fgets_buff);

      char* string_buff = NULL;
      size_t string_len = 0;

      if ((string_len = strlen (fgets_buff)) &&
          (string_buff = eat_ws (fgets_buff, &string_len)))
        {
          /*
           GF
           Changed fgets_buff to string_buff below.
           This was causing whitespace and newlines left in form records.
           */
          
          // Line may be commented out by '#'.
          if (string_buff[0] == '#')
            {
              fprintf (stderr, "%s - skipping commented file string \"%s\n", 
                       __func__, fgets_buff);
              continue;
            }

          if (!string_len)
            {
              fprintf (stderr, "%s - skipping empty line.\n", __func__);
              continue;
            }

          if (string_buff[string_len - 2] == '\r')
            {
              string_buff[string_len - 2] = '\0';
            }

          if (string_buff[string_len -1] == '\n')
            {
              string_buff[string_len -1] = '\0';
            }

          if (url->form_records_num >= max_records)
            {
              fprintf (stderr, "%s - error: CLIENTS_NUM (%d) and "
                       "FORM_RECORDS_FILE_MAX_NUM (%Zu) are both less than the number of " 
                       "records in the form_records_file.\n", 
                       __func__, bctx->client_num_max, url->form_records_file_max_num);
              sleep (3);
              break;
            }

          if (load_form_record_string (string_buff,
                                       string_len,
                                       &url->form_records_array[url->form_records_num],
                                       url->form_records_num,
                                       &sep) == -1)
          {
              fprintf (stderr, 
                       "%s - error: load_client_credentials_buffers () failed "
		       "on records line \"%s\"\n", __func__, fgets_buff);
              fclose (fp);
              return -1 ;
          }
          
          url->form_records_num++;
        }
    }

  if (!url->form_records_random && (int)url->form_records_num < bctx->client_num_max)
    {
      fprintf (stderr, 
               "%s - error: CLIENTS_NUM (%d) is above the number " 
               "of records in the form_records_file\nPlease, either decrease "
	       "the CLIENTS_NUM or add more records strings to the file.\n", 
               __func__, bctx->client_num_max);
      fclose (fp);
      return -1 ;
    }

  fclose (fp);
  return 0;
}

/*******************************************************************************
* Function name -  find_first_cycling_url
*
* Description - Finds the first (by index) url, where cycling is required.
*
* Input -       *bctx - pointer to the batch context
*                          
* Return Code/Output - If found - the url non-negative index. Return ( -1),
*                      when no cycling urls configured.
********************************************************************************/
static int find_first_cycling_url (batch_context* bctx)
{
  size_t i;
  int first_url = -1;
  
  for (i = 0;  i < (size_t)bctx->urls_num; i++)
    {
      if (! bctx->url_ctx_array[i].url_dont_cycle)
        {
          first_url = i;
          break;
        }
    }

  bctx->first_cycling_url = first_url;

  if (bctx->first_cycling_url < 0)
    bctx->cycling_completed = 1;

  return bctx->first_cycling_url;
}

/*******************************************************************************
* Function name -  find_last_cycling_url
*
* Description - Finds the last (by index) url, where cycling is required.
*
* Input -       *bctx - pointer to the batch context
*                          
* Return Code/Output - If found - the url non-negative index. Return ( -1),
*                      when no cycling urls configured.
********************************************************************************/
static int find_last_cycling_url (batch_context* bctx)
{
  size_t i = 0;
  int last_url = -1;

  for (i = 0;  i < (size_t)bctx->urls_num; i++)
    {
      if (! bctx->url_ctx_array[i].url_dont_cycle)
        {
          last_url = i;
        }
    }

  bctx->last_cycling_url = last_url;

  if (bctx->last_cycling_url < 0)
    bctx->cycling_completed = 1;

  return bctx->last_cycling_url;
}

/*******************************************************************************
* Function name - netmask_to_cidr
*
* Description - Converts quad-dotted IPv4 address string to the CIDR number
*               from 0 to 32.
*
* Input -       *dotted_ipv4 - quad-dotted IPv4 address string
* Return Code/Output - On Success - CIDR number from 0 to 32, on Error - (-1)
********************************************************************************/
static int netmask_to_cidr (char *dotted_ipv4)
{
  int network = 0;
  int host = 0;
 
  if (inet_pton (AF_INET, dotted_ipv4, &network) < 1) 
    {
      return -1;
    }

  host = ntohl (network);

  int tmp = 0;
  
  while (!(host & (1 << tmp)) && tmp < 32)
    {
      tmp++;
    }

  return (32 - tmp); 
 }

/*******************************************************************************
* Function name -  print_correct_form_usagetype
*
* Description - Outputs explanation about mismatching of FORM_STRING and
*                     FORM_USAGE_TYPE. 
*
* Input -       ftype - type of form usage
*                     *value - the value of FORM_USAGE_TYPE tag
*                          
* Return Code/Output - Returns always -1 as being error output.
********************************************************************************/
static int print_correct_form_usagetype (form_usagetype ftype, char* value)
{
  switch (ftype)
    {
    case FORM_USAGETYPE_UNIQUE_USERS_AND_PASSWORDS:

      fprintf (stderr, 
               "\n%s - error: FORM_STRING value (%s) is not valid. \nPlease, use:\n"
               "- to generate unique users with unique passwords two \"%%s%%d\" "
	       ", something like \"user=%%s%%d&password=%%s%%d\" \n", 
	       __func__, value);
      break;

    case FORM_USAGETYPE_UNIQUE_USERS_SAME_PASSWORD:
      fprintf (stderr, 
               "\n%s - error: FORM_STRING value (%s) is not valid. \nPlease, use:\n"
               "- to generate unique users with the same passwords one \"%%s%%d\" \n"
               "for users and one \"%%s\" for the password," 
               "something like \"user=%%s%%d&password=%%s\" \n", __func__, value);
      break;

    case FORM_USAGETYPE_SINGLE_USER:
      fprintf (stderr, 
               "\n%s - error: FORM_STRING  value (%s) is not valid. \nPlease, use:\n"
               "- for a single configurable user with a password two \"%%s\" "
	       ", something like \"user=%%s&password=%%s\" \n",
	       __func__, value);
      break;

    case FORM_USAGETYPE_RECORDS_FROM_FILE:
      fprintf (stderr, 
               "\n%s - error: FORM_STRING value (%s) is not valid. \nPlease, use:\n"
               "- to load user credentials (records) from a file two \"%%s\" "
	       ", something like \"user=%%s&password=%%s\" \n and _FILE defined.\n", 
	       __func__, value);
      break;

    default:
      break;
    }

  return -1;
}













int update_url_from_set_or_template(CURL* handle, client_context* client, url_context* url);
int randomize_url(CURL *handle, url_context *url);
extern int		scan_response(curl_infotype type, char* data, size_t size, client_context* client);
extern void		free_url_extensions(url_context* url);

static size_t	read_callback(void *ptr, size_t size, size_t nmemb, void *uptr);

static int		pick_url_from_set(CURL* handle, client_context* client, url_context* url);
static int		complete_url_from_response(CURL* handle, client_context* client, url_context* url);
static int		build_url_set(url_set* set, url_template* template, FILE* file, char* fname);
static urle*	build_urle(char* line, url_template* template);

static void		construct_url(char* buf, url_template* template);
static int		install_url(CURL* handle, url_context* url, char* s);

static void		free_url_template(url_template* t);
static void		free_url_set(url_set* set);

static int		keyval_start(int nqueues);
static int		keyval_create(int index, void* context, char* word);
static int		keyval_init(int index, void* context);
static void		keyval_scan(int index, void* context, char* data, int size);
static void		keyval_flush(int index, void* context);
static char*	keyval_lookup(char* word, int index);
static void		keyval_stop();

static char*	string_copy(char* src, char* dst);
static char*	get_line(char* buf, int size, FILE* file);
static char*	get_token(char** line_ptr);
static char*	skip_white(char* s);
static char*	skip_black(char* s);
static char*	skip_quote(char* s);
static int		err_out(const char* func, char* fmt, ...);

#define error(x)	err_out(__func__, x)


/*********************************************************

	Build or choose the next URL for this client

*********************************************************/

/*
  Called from setup_curl_handle_init in loader.c to construct the next url
  from a template, or retrieve the next url from a preconstructed set.
*/
int
update_url_from_set_or_template(CURL* handle, client_context* client, url_context* url)
{
    int result;
	
    /*
      If this url has RESPONSE_TOKENs to scan for, we need to reinitialize them
    */
    if (url->response.n_tokens > 0)
    {
        (void) keyval_init(client->client_index, url);
    }
	
    /*
      If this is an URL and not an URL_TEMPLATE, there's nothing more to do
    */
    if (! is_template (url))
    {
        return 0;
    }
	
    /*
      Try to pick an url from an url_set, or if this wasn't an url_set,
      try to complete the template from server responses
    */
    if ((result = pick_url_from_set(handle, client, url)) == 0)
    {
        result = complete_url_from_response (handle, client, url);
    }
	
    return (result < 0) ? -1 : 0;
}


/*
Randomize a part of the URL based on a specific token
Used for creating a variance for caching server to hit or miss
even with a small set of URLs.
*/
int randomize_url(CURL *handle, url_context *url)
{
    char buf[512];
    char *s = 0;
    char rand_tmp[32];
    long int rand;

    if (url->random_hrange <= 0) 
    {
        return 0;
    }

    if (! url->random_token) 
    {
        return 0;
    }

    if ( ! (s = strcasestr (url->url_str, url->random_token))) 
    {
        return 0;
    }
    
    strncpy(buf, url->url_str, s - url->url_str);
    buf[s - url->url_str] = '\0';

    rand = (random() % (url->random_hrange - url->random_lrange)) + url->random_lrange;

    sprintf(rand_tmp, "%lu", rand);
    strcat(buf, rand_tmp);
    strcat(buf, s + strlen(url->random_token));
    //fprintf(stderr, "%s random url: %s\n", __func__, buf);
    
    //reload the url with the random string in it.
    curl_easy_setopt (handle, CURLOPT_URL, buf);

    return 0;
}


/*
  Choose the next URL from an url set. Return 0 if this isn't an url_set,
  1 on success, and 0 on failure.
*/
static int
pick_url_from_set (CURL* handle, client_context* client, url_context* url)
{
    url_set* set = &url->set; 
    urle* u;
	
    if (set->n_urles == 0)
    {
        return 0; /* this wasn't a pre-constructed url set */
    }
	
    if (!url->url_cycling)
    {
        set->index = client->client_index % set->n_urles;
    }
    else if (++set->index >= set->n_urles) /* will set index to 0 the first time through */
    {
        set->index = 0;
    }	

   u = &set->urles[set->index];

   if (install_url(handle, url, u->string) < 0)
   {
       return -1;
   }
	
   if (u->cookie)
   {
       curl_easy_setopt(handle, CURLOPT_COOKIE, u->cookie);
   }
	
   return 1;
}


/*
  Construct an URL from prior server responses.
*/
static int
complete_url_from_response(CURL* handle, client_context* client, url_context* url)
{
    char **names;
    char **values;
    char buf[512];
    int i;
	
    /*
      This check should have been done at parse time,
      but there's no convenient place to do it
    */
    if (url->template.n_tokens != url->template.n_cents)
    {
        return err_out (__func__, "wrong number of URL_TOKENS for %s", url->template.string);
    }
	
    names = url->template.names;
    values = url->template.values;
	
    for (i = 0; i < url->template.n_tokens; i++)
    {
        if ((values[i] = keyval_lookup(names[i], client->client_index)) == 0)
            return error("missing server response values");
    }
	
    construct_url (buf, &url->template);
	
    if (install_url (handle, url, buf) < 0)
    {
        return -1;
    }
	
    client->previous_type = 0;
    return 1;
}


/*
  Install a string as the official url_str in this url_context.
*/
static int
install_url (CURL* handle, url_context* url, char* s)
{
   /*
     Hand the url to curl right away. If we wait for setup_curl_handle_init
     to do it with url->url_str, another client from another thread might change
     that value first.
   */
    curl_easy_setopt (handle, CURLOPT_URL, s);

    if (url->url_str != 0)
    {
        free(url->url_str);
    }
	
    if ((url->url_str = strdup(s)) == 0)
    {
        return error("cannot allocate space for url_str");
    }
	
    url->url_str_len = strlen(s);
    return 0;
}


/*********************************************************

	Set up an URL template

*********************************************************/

/*
   Called from parse_conf.c to parse an URL_TEMPLATE line
*/
extern int
url_template_parser (batch_context* const batch, char* const string)
{
   extern int url_parser(batch_context* const, char* const); 
   url_context* url;
   char* s;
	
   if (string == 0 || *string == 0)
   {
       return error("missing template string");
   }
	
   /*
     url_parser copies the string to the url_context->url_str field.
     It also initilaizes url_str_len, url_appl_type, and url_ind,
     and increments batch->url_index.
   */
   if (url_parser (batch, string) < 0)
   {
       return error("url_parser failed");
   }
	
   url = &batch->url_ctx_array[batch->url_index];
	
   if ((url->template.string = strdup(string)) == 0)
   {
       return error("cannot allocate template string");
   }
	
   for (s = string; (s = strstr(s, "%s")) != 0; s++)
   {
       url->template.n_cents++;
   }
	
   if (url->template.n_cents == 0)
   {
       return error("invalid URL_TEMPLATE");
   }
	
   if ((url->template.names = (char**) calloc(url->template.n_cents, sizeof(char**))) == 0)
   {
       return error("cannot allocate token-name array");
   }

   if ((url->template.values = (char**) calloc(url->template.n_cents, sizeof(char**))) == 0)
   {
       return error("cannot allocate token-value array");
   }

   return 0;
}


/*********************************************************

	Set up response keys and url keys

*********************************************************/

 /*
   Called in parse_conf.c to parse a RESPONSE_KEY line.
   The keyword is stored by the keyvalue engine, and it is
   tied to the client index and the url context. Later when
   we're scanning the server reponse, we ask the keyval engine
   to scan for all words belonging to the current client and url.
 */
extern int
response_token_parser (batch_context* const batch, char* const word)
{
   url_context* url = &batch->url_ctx_array[batch->url_index];
   int nclients = batch->client_num_max;
   int i;
	
   if (keyval_start(nclients) < 0)
       return -1;
	
   for (i = 0; i < nclients; i++)
   {
       if (keyval_create(i, url, word) < 0)
           return -1;
       
       url->response.n_tokens++;
   }   
   return 0;
}


/*
  Called in parse_conf.c to parse an URL_TOKEN line
*/
extern int
url_token_parser (batch_context* const batch, char* const word)
{
    url_context* url = & batch->url_ctx_array[batch->url_index];
	
    if (url->set.urles != 0)
    {
        return error("cannot have both URL_TOKEN_FILE and URL_TOKEN");
    }
	
    if (url->template.n_cents == 0)
    {
        return error("URL_TOKEN specified without a valid URL_TEMPLATE");
    }
	
   if (url->template.n_tokens >= url->template.n_cents)
   {
       return error("too many URL_TOKENs for this template");
   }
	
   if ((url->template.names[url->template.n_tokens++] = strdup(word)) == 0)
   {
       return error("cannot allocate space for token");
   }
	
   return 0;
}


/*********************************************************

	Scan the server response

*********************************************************/

/*
  Called from client_tracing_function in loader.c
*/
int scan_response(curl_infotype type, char* data, size_t size, client_context* client)
{
    url_context* url = & client->bctx->url_ctx_array[client->url_curr_index];
	
    if (url->response.n_tokens == 0)
    {
        return 0; /* not looking for any RESPONSE_TOKENS */
    }
		
    if (type == CURLINFO_DATA_IN)
    {
        // scan for this url's keyvals, storing results in this client's space
        keyval_scan(client->client_index, url, data, size);
    }
    else if (client->previous_type == CURLINFO_DATA_IN)
    {
        // finish any unterminated values
        keyval_flush(client->client_index, url);
    }
	
    client->previous_type = type;
    return 0;
}



/*********************************************************

	Build an url set from a token file

*********************************************************/
/*
  Called in parse_conf.c to parse an URL_SET_TOKEN_FILE line
*/
extern int
url_token_file_parser(batch_context* const batch, char* const fname)
{
    url_context* url = &batch->url_ctx_array[batch->url_index];
    FILE* file;
	
    if (fname == 0 || *fname == 0)
    {
        return error("missing file name");
    }
	
    if (url->template.n_tokens != 0)
    {
        return error("cannot have both URL_TOKEN_FILE and URL_TOKEN");
    }
	
    if ((file = fopen(fname, "r")) == 0)
    {
        return err_out(__func__, "unable to open file %s", fname);
    }
	
   if (build_url_set(&url->set, &url->template, file, fname) < 0)
   {
       fclose(file);
       return error("build_url_set failed");
   }
	
   fclose(file);
   return 0;
}


#define is_comment(x) ((s = skip_white(x)) == 0 || *s == 0 || *s == '#')

/*
  Build a set of URLs from the template string and token stream.
*/
static int
build_url_set(url_set* set, url_template* template, FILE* file, char* fname)
{
    int nrows;
    int ind;
    int lineno; 
    char f_buf[512];
    char *s;
    urle *u;
    urle *v;
	
    for (nrows = 0; get_line(f_buf, sizeof f_buf, file) != 0; )
        if (!is_comment(f_buf))
            nrows++;
		
    if ((set->urles = (urle*) calloc(nrows, sizeof (urle))) == 0)
        return error("unable to allocate urle array");
	
    set->n_urles = nrows;
    rewind (file);
		
    for (ind = 0, lineno = 1; get_line(f_buf, sizeof f_buf, file) != 0; lineno++)
    {
        if (is_comment(f_buf))
            continue;
		
        if ((u = build_urle(f_buf, template)) == 0)
            return err_out(__func__, "error parsing line %d of file %s", lineno, fname);
		
        v = &set->urles[ind];
        v->string = u->string;
        v->cookie = u->cookie;
		
        ind++;
    }
	
    set->index = -1; /* prepare for first call to pick_url_from_set */
    return 0;
}


static urle*
build_urle (char* line, url_template* template)
{
    char **values = template->values;
    char *line_ptr = line;
    char *cookie;
    char buf[512];
    int i;
    static urle u;
	
	
    for (i = 0; i < template->n_cents; i++)
    {
        if ((values[i] = get_token(&line_ptr)) == 0)
        {
            error("not enough tokens for this url template");
            return 0;
        }
    }
	
    cookie = get_token(&line_ptr); /* optional cookie */
    construct_url(buf, template);
	
    if ((u.string = strdup(buf)) == 0)
        return 0;
	
    if (cookie && (u.cookie = strdup(cookie)) == 0)
    {
        free(u.string);
        return 0;
    }
	
    return &u;
}


/*
  Construct an URL from the template and its list of token values
*/
static void
construct_url (char* buf, url_template* template)
{
    char *s;
    char *b;
    int n = 0;
	
    /* copy the template to the buffer, substituting tokens for %b */
    for (s = template->string, b = buf; *s != 0; s++)
    {
        if (*s != '%')
        {
            *b++ = *s;
        }
        else if (*++s != 's')
        {
            *b++ = '%';
            *b++ = *s;
        }
        else
        {
            b = string_copy(template->values[n++], b);
        }
    }
	
    *b = 0;
}



static int ignore_content_length (batch_context*const bctx, char*const value)
{
    long boo = atol (value);

    if (boo < 0 || boo > 1)
    {
        fprintf(stderr, 
                "%s error: boolean input 0 or 1 is expected\n", __func__);
        return -1;
    }
    bctx->url_ctx_array[bctx->url_index].ignore_content_length = boo;
    return 0;
}

static int url_random_range (batch_context*const bctx, char*const value)
{
  long rand_lrange = 0;
  long rand_hrange = 0;
  size_t value_len = strlen (value) + 1;

  if (parse_timer_range (value, value_len, &rand_lrange, &rand_hrange) == -1)
    {
      fprintf(stderr, "%s error: parse_timer_range () failed.\n", __func__);
      return -1;
    }
  
  if ( rand_hrange <= rand_lrange )
    {
      fprintf(stderr, "%s low value must be < high value\n", __func__);
      return -1;
    }

    bctx->url_ctx_array[bctx->url_index].random_lrange = (int)rand_lrange;
    bctx->url_ctx_array[bctx->url_index].random_hrange = (int)rand_hrange;

    return 0;
}


static int url_random_token (batch_context*const bctx, char*const value)
{
    size_t value_len = strlen (value) + 1;
    url_context* url = &bctx->url_ctx_array[bctx->url_index];

    url->random_token = calloc(value_len, sizeof(char));

    strcpy(url->random_token, value);
    return 0;
}
/*********************************************************
	Flag parsers
*********************************************************/
#if 0
/*
  Called in parse_conf.c to parse an URL_CYCLING line
  NOT IMPLEMENTED YET
*/
extern int
url_cycling_parser(batch_context* const batch, char* const value)
{
    url_context* url = &batch->url_ctx_array[batch->url_index]; int flag = atoi(value);
	
    if ( ! (flag == 0 || flag == 1) )
    {
        fprintf (stderr, "%s - error: URL_CYCLING should be either 0 or 1.\n", __func__);
        return -1;
    }
	
    url->url_cycling = flag;
    return 0;
}
#endif

/*
  Called in parse_conf.c to parse an FORM_RECORDS_CYCLE line
  FORM_RECORDS_CYCLE = 1 is similar to FORM_RECORDS_RANDOM = 1,
  except that the records are assigned round-robin, instead of
  at random. This can be used if you need more predictability.
*/
extern int
form_records_cycle_parser(batch_context* const batch, char* const value)
{
    url_context* url = &batch->url_ctx_array[batch->url_index]; int flag = atoi(value);
	
    if ( ! (flag == 0 || flag == 1) )
    {
        fprintf (stderr, "%s - error: FORM_RECORDS_CYCLE should be either 0 or 1.\n", __func__);
        return -1;
    }
	
    url->form_records_cycle = flag;
    return 0;
}

/*
  Parse a RANDOM_SEED tag.
  RANDOM_SEED = <non-negative number>
  This tags allows setting the random seed to a specified non-negative value.
  With the same seed the pseudo-random numbers used in various parts of the
  curl-loader are generated in the same order, which produces more consistent
  results.  If this tag is absent or the value is negative, the random seed
  is generated based on the current time, i.e. it is different from one run
  of the curl-loader to another.  The same seed is used for all random numbers.
  (Example, RANDOM_SEED = 10).
*/
static int random_seed_parser (batch_context*const bctx, char*const value)
{
  (void)bctx;
  if (!(isdigit(*value) || *value == '-' || *value == '+'))
    {
      fprintf(stderr, "%s error: tag RANDOM_SEED should be numeric\n",
          __func__);
      return -1;
    }
  random_seed = atoi(value);
  return 0;
}
 
  /* Get a random number */
double get_random ()
{
  return (double)random() / (RAND_MAX + 1.0);
}

  /* Get probability: 1-100 */
int get_prob ()
{
  return 1 + (int) (100 * get_random());
}


/*********************************************************
	Per-client file upload streams.
	Allows multiple clients to upload the same file, and
	cycling URLs to continually upload the same file.
*********************************************************/
/*
  Called from upload_file_parser to allocate per-client file upload streams
*/
static int upload_file_streams_alloc(batch_context* batch)
{
    url_context* url = &batch->url_ctx_array[batch->url_index];
	
    if ((url->upload_offsets = (off_t*) calloc(batch->client_num_max, sizeof (off_t))) == 0)
        return error("cannot allocate upload streams");
	
    return 0;
}

/*
  Called from setup_curl_handle_init to initialize a per-client upload file stream,
  and re-initialize the stream for a cycling url.
*/
int upload_file_stream_init (client_context* client, url_context* url)
{
    off_t* offset_ptr = & url->upload_offsets[client->client_index];
    CURL* handle = client->handle;
	
    if (url->upload_file == 0)
    {
        return 0;
    }
		
    if	(
        url->upload_file_ptr == 0 &&
        (url->upload_file_ptr = fopen(url->upload_file, "rb")) == 0
        )
    {
        return err_out(__func__, "fopen(%s) failed, errno = %d", url->upload_file, errno);
    }

    url->upload_descriptor = fileno(url->upload_file_ptr);
    *offset_ptr = 0; /* re-initializes the stream for a cycling url */
	
    curl_easy_setopt(handle, CURLOPT_UPLOAD, 1);
    curl_easy_setopt(handle, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(handle, CURLOPT_READDATA, client);
    curl_easy_setopt(handle, CURLOPT_INFILESIZE, (long) url->upload_file_size);
	
    if (url->transfer_limit_rate)
    {
        curl_easy_setopt(handle, CURLOPT_MAX_SEND_SPEED_LARGE,(curl_off_t) url->transfer_limit_rate);
    }

    return 0;
}


/*
  Callback from libcurl to handle file-upload reads.
*/
static size_t
read_callback(void *ptr, size_t size, size_t nmemb, void* user_supplied)
{
    extern ssize_t pread(int, void *, size_t, off_t);
   
    client_context* client = user_supplied;
    batch_context* batch = client->bctx;
    url_context* url = & batch->url_ctx_array[client->url_curr_index];
    off_t* offset_ptr = & url->upload_offsets[client->client_index];
    int nread;
	
   /*
     Use pread instaed of fseek/fread for thread safety
   */
    if ((nread = pread(url->upload_descriptor, ptr, size * nmemb, *offset_ptr)) < 0)
    {
        err_out(__func__, "pread returned %d", errno);
        return 0;
    }
	
    *offset_ptr += nread;
    return nread;
}


/*********************************************************
	Cleanup
*********************************************************/

#define freeze(x) if (x != 0) {free(x); x = 0;}

/*
  Free url_set and url_response stuff
  Called from free_url in loader.c
*/
void free_url_extensions(url_context* url)
{
    free_url_set(&url->set);
    free_url_template(&url->template);
    freeze(url->upload_offsets);
    keyval_stop();
}

	
static void
free_url_template(url_template* t)
{
    int i;
	
    for (i = 0; i < t->n_tokens; i++)
    {
        freeze(t->names[i]);
    }
		
    freeze(t->names);
    freeze(t->string);
    freeze(t->values); /* the individual values were not allocated */
}


static void
free_url_set(url_set* set)
{
    int i;

    if (set->urles == 0)
        return;
	
    for (i = 0; i < set->n_urles; i++)
    {
        urle* u = &set->urles[i];
		
        freeze(u->string);
        freeze(u->cookie);
    }
	
    freeze(set->urles);
}


/*********************************************************
	Outside keyval interface
*********************************************************/

#define VALUE_SIZE	256

typedef struct keyval
{
    struct	keyval* next;
    void*	context;
	
    struct
    {
        char*	word;
        int		index;
        char	        last_c;
        int		found;

    } key;
	
    struct
    {
        char	       buf[VALUE_SIZE];
        int		index;
        char	        quote;
        int		found;
    } value;
	
} keyval;


typedef struct
{
    keyval*	first;
    keyval*	last;
} keyq;


static keyq* que_array = 0;

static int num_queues = 0;


static int	kv_create(keyval* k, char* word);
static void	kv_scan(keyval* k, char* data, int size);
static void	scan_for_key(keyval* k, char c);
static void	scan_for_value(keyval *k, char c);
static void	add_to_value(keyval* k, char c);
static void	kv_init(keyval* k);
static void	kv_free(keyval* k);
static void	kv_flush(keyval* k);
static void	q_insert(keyq* q, keyval* k);
static int err_out(const char* func, char* fmt, ...);

#define error(x) err_out(__func__, x)

/*
  Allocate an array of keyval queues, one for each possible context
*/
static int
keyval_start(int nqueues)
{
   if (que_array)
       return 0;
		
   if ((que_array = (keyq*) calloc(nqueues, sizeof (keyq))) == 0)
       return error("cannot allocate que_array");
	
   num_queues = nqueues;
   return 0;
}

/*
  Create a keyval for a given keyword
*/
static int
keyval_create(int index, void* context, char* word)
{
    keyval* k;
	
    if (index >= num_queues)
        return error("index out of range");
	
    if ((k = calloc(1, sizeof *k)) == 0)
        return error("cannot allocate keyval");
	
    if (kv_create(k, word) < 0)
        return -1;
	
    k->context = context;
    q_insert(&que_array[index], k);
	
    return 0;
}


/*
  Called when a previously-fetched URL is recycled,
  and we want to capture new response values
*/
static int
keyval_init(int index, void* context)
{
    keyval* k;
	
    if (index >= num_queues)
        return error("index out of range");
	
    for (k = que_array[index].first; k != 0; k = k->next)
    {
        if (k->context == context)
            kv_init (k);
    }
	
    return 0;
}


/*
  Scan the given data for all the keyvals in the index
*/
static void
keyval_scan(int index, void* context, char* data, int size)
{
    keyval* k;
	
    for (k = que_array[index].first; k != 0; k = k->next)
    {
        if (k->context == context)
            kv_scan(k, data, size);
    }
}

static void
keyval_flush(int index, void* context)
{
    keyval* k;
	
    for (k = que_array[index].first; k != 0; k = k->next)
    {
        if (k->context == context)
            kv_flush(k);
    }
}

static char*
keyval_lookup(char* word, int index)
{
    keyval* k;
	
    for (k = que_array[index].first; k != 0; k = k->next)
    {
        if (strcmp(k->key.word, word) == 0)
            return k->value.buf;
    }
    return 0;
}

static void
keyval_stop()
{
    int i; keyval *k, *next;
	
    if (que_array == 0)
        return;
	
    for (i = 0; i < num_queues; i++)
        for (k = que_array[i].first; k != 0; k = next)
        {
            next = k->next;
            kv_free(k);
        }
	
    free(que_array);
    que_array = 0;
}
 
static void
q_insert(keyq* q, keyval* k)
{
    if (q->first == 0)
    {
        q->first = k;
    }
    else if (q->last != 0)
    {
        q->last->next = k;
    }
   
    q->last = k;
    k->next = 0;
}

/*********************************************************
	Low-level keyval implementation,
	for word-boundry-only substring matching
*********************************************************/
static int
kv_create(keyval* k, char* word)
{
    if ((k->key.word = strdup(word)) == 0)
        return error("cannot allocate word");	
    return 0;
}

static void
kv_scan(keyval* k, char* data, int size)
{
    if (k->value.found)
        return;
		
    for ( ; size > 0; data++, size--)
    {
        if (k->key.found == 0)
        {
            scan_for_key(k, *data);
            k->key.last_c = *data;
        }
        else if (k->value.found == 0)
        {
            scan_for_value(k, *data);
        }
        else
            break;
    }
}

/*
  Alphanumeric characters plus @, underscore, and dot.
*/
static const char
alphanum_plus[] =
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/*   0 -  15 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/*  16 -  31 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 	/*  32 -  47 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 	/*  48 -  63 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/*  64 -  79 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 	/*  80 -  95 */
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 	/*  96 - 111 */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 	/* 112 - 127 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 128 - 143 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 144 - 159 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 160 - 175 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 176 - 191 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 192 - 207 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 208 - 223 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 224 - 239 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 	/* 240 - 255 */
};

#define acceptable(x)	(alphanum_plus[(unsigned char) (x)] == 1)

static void
scan_for_key(keyval* k, char c)
{
    if (k->key.index == 0)
    {
        /* No matching so far. If we're continuing a run of the same thing
           (alphanumeric or non), or we've switched to non-alphanumeric,
           just ignore the character and continue the non-matching run.
        */
        if (acceptable(c) == acceptable(k->key.last_c) || !acceptable(c))
            return;
		
        /* Otherwise we have just changed from non-alphanumeric to
           alphanumeric, so we fall through and start matching.
        */
    }
 
    if (k->key.word[k->key.index] == 0)
    {
        /* we're in a matching run, and we've finished matching the keyword */
        if (acceptable(c))
        {
            // if the next character alphanumeric, so the match is spoiled
            k->key.index = 0;
        }
        else
        {
            // otherwise we decalre a match! 
            k->key.found = 1;
        }
        return;
    }
    
    if (c != k->key.word[k->key.index])
    {
        /* Now we're in a matching run, and this character doesn't match,
           so switch to a non-matching run.
        */
        k->key.index = 0;
        return;
    }
	
    /* We've matched another character */
    ++k->key.index;
}

#define is_quote(x) (x == '"' || x == '\'')

/*
  We've found the keyword, and now we're collecting the value string.
  We'll first skip any non-aplhanumerics. Then we'll collect a quoted
  string, or a string of alphanumerics.
*/
static void
scan_for_value(keyval *k, char c)
{
    if (acceptable(c))
    {
        add_to_value(k, c); /* any alphanumeic is welcome */
        return;
    }
		
    if (k->value.quote) /* we're in a quote run */
    {
        if (k->value.quote == c) /* we've found the terminating quote */
            k->value.found = 1;
        else
            add_to_value(k, c); /* collect the character, whatever it is */
        return;
    }
	
    if (is_quote(c) && k->value.index == 0)
    {
        k->value.quote = c; /* start a quote run */
        return;
    }
	
    if (k->value.index > 0) /* we've collected non-quoted characters, and ... */
    {
        k->value.found = 1; /* this non-aplhanumeric ends the collection */
        return;
    }
	
    /* we have an initial non-quote non-alphanumeric, which we'll ignore */
}

static void
add_to_value(keyval* k, char c)
{
    if (k->value.index < VALUE_SIZE - 1)
    {
        k->value.buf[k->value.index++] = c;
        k->value.buf[k->value.index] = 0;
    }
}

static void
kv_init(keyval* k)
{
    k->key.index = 0;
    k->key.last_c = 0;
    k->key.found = 0;
	
    k->value.buf[0] = 0;
    k->value.index = 0;
    k->value.quote = 0;
    k->value.found = 0;
}

static void
kv_free(keyval* k)
{
    if (k != 0)
    {
        free(k->key.word);
        free(k);
    }
}

static void
kv_flush(keyval* k)
{
    if (k->key.found || k->key.word[k->key.index] == 0)
        k->value.found = 1;
}
	

#if 0
/***** bitap version, for more general substring search *****/
	/*
	 Struct for keeping track of a bitap string search.
	 We use chars instead of bits for simplicity, but
	 bits would save space and run faster.
	 */
typedef struct bitap
{
   char* word;	/* pattern or word to be recognized */
   int   size;	/* strlen(word) */
   char* mark;	/* array of size + 1 markers */
} bitap;


typedef enum
{
    init = 0,
    inter,
    found

} state;


#define	VALUE_SIZE	256

typedef struct keyval
{
    struct keyval* next;
    void* context;

    bitap	key;
    char*	data;
    int		size;
    char	value[VALUE_SIZE + 1];
    int		length;
    char	quote;
    state   keystate;
    state   valstate;

} keyval;


static int		bitap_search(bitap* bp, char* stream, int length);
static int		bitap_init(bitap* b, char* word);
static int		bitap_refresh(bitap* b);
static void	bitap_free(bitap* b);

static int kv_init (keyval* k)
{
   k->value[0] = 0;
   k->length = 0;
   k->quote = 0;
   k->keystate = init;
   k->valstate = init;
   return bitap_refresh(&k->key);
}

static void kv_free (keyval* k)
{
   bitap_free(&k->key);
}

static void kv_flush (keyval* k)
{
   if (k->keystate == found)
    {
        k->valstate = found;
        k->value[k->length] = 0;
    }
}

static void kv_scan (keyval* k, char* data, int size)
{
    k->data = data;
    k->size = size;
	
    if (k->keystate != found)
	   scan_for_key(k);
	
    if (k->keystate == found && k->valstate != found)
        scan_for_value(k);
}

static void scan_for_key(keyval* k)
{
    int n = 0;
	
    if (k->data == 0 || k->size == 0 || k->keystate == found)
        return (void) error("bad initial conditions");
	
    n = bitap_search(&k->key, k->data, k->size);
	
    if (n > 0)
    {
	   k->data += n;
	   k->size -= n;
		
	   k->keystate = found;
    }
}

static void scan_for_value(keyval *k)
{
   if (k->data == 0 || k->size == 0 || k->valstate == found)
       return (void) error("bad initial conditions");
	
   if (k->valstate == init)
   {
	   /* skip white space and equals signs */
       for ( ; k->size > 0 && (is_white(*k->data) || *k->data == '='); k->size--, k->data++)
		   ;
		
       if (k->size == 0)
           return; /* we haven't found the beginning of the value yet */
		
       k->valstate = inter;
		
       if (*k->data == '"' || *k->data == '\'')
        {
            k->quote = *k->data++;
            k->size--;
        }

       if (k->size == 0)
           return;
   }
	
   /* at this point we're collecting the value */
   if (k->quote != 0)
    {
		for ( ; k->size > 0 && *k->data != k->quote; k->size--, k->data++)
			if (k->length < VALUE_SIZE)
				k->value[k->length++] = *k->data;
		
		if (k->size > 0)
			k->valstate = found;
    }	
   else
    {
		for ( ; k->size > 0 && !is_white(*k->data); k->size--, k->data++)
			if (k->length < VALUE_SIZE)
				k->value[k->length++] = *k->data;
		
		if (k->size > 0)
			k->valstate = found;
    }
}


static int
is_white(char c)
{
	return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}


/***** Bitap stuff *****/

	static int
bitap_search(bitap* bp, char* stream, int length)
	{
	int i, j; char* b = bp->mark; char* p = bp->word;
	
	for (j = 1; j <= length; j++, stream++)
		{
		for (i = bp->size; i > 0; i--)
			b[i] = b[i - 1] && (p[i - 1] == *stream);
		
		if (b[bp->size] == 1)
			return j;
		}
	
	return 0;
	}

	static int
bitap_init(bitap* b, char* word)
	{
	if (word)
		{
		if ((b->word = strdup(word)) == 0)
			return error("cannot allocate word");
		
		if ((b->mark = calloc((b->size + 1), sizeof (char))) == 0)
			return error("cannot allocate mark array");
		
		b->size = strlen(word);
		}
	
	return bitap_refresh(b);
	}

	static int
bitap_refresh(bitap* b)
	{
	int i;
	
	b->mark[0] = 1;

	for (i = 1; i <= b->size; i++)
		b->mark[i] = 0;
	
	return 0;
	}

	static void
bitap_free(bitap* b)
	{
	free(b->word);
	free(b->mark);
	}

/***** end of bitap version *****/
#endif


/*********************************************************

	Utilities

*********************************************************/
	
/*	 
Copy the source to the destination, and return a pointer to the zero byte 
at the end of the destination
*/
static char* string_copy(char* src, char* dst)
{
    while ((*dst++ = *src++) != 0)
        ;	
    return dst - 1;
}
	
static char* get_line(char* buf, int size, FILE* file)
{
    char* s;
	
    if (fgets(buf, size, file) == 0)
        return 0;

    s = buf + strlen(buf) - 1;
	
    if (*s == '\n' || *s == '\r')
        *s = 0;
	
    return buf;
}

/*
   Get the next token from the source line
*/
static char* get_token (char** line_ptr)
{
    char *s, *p;
     
    if (*line_ptr == 0)
        return 0;
	
    s = skip_white(*line_ptr);
	
    if (s == 0 || *s == 0 || *s == '#')
    {
        *line_ptr = 0;
        return 0;
    }
	
    if (*s == '"' || *s == '\'')
        p = skip_quote(s++);
    else
        p = skip_black(s);
	
    *line_ptr = *p? p + 1 : 0;
    *p = 0;
	
    return s;
}
	
/*
  Move a pointer past any whitespace, or to the end of the string
*/
static char* skip_white (char* s)
{
   if (s != 0)
   {
       while (*s && (*s == ' ' || *s == '\t'))
           s++;
   }	
   return s;
}

/*
  Move a pointer past non-whitespace, or to the end of the string
*/
static char* skip_black (char* s)
{
   if (s != 0)
   {
       while (*s && !(*s == ' ' || *s == '\t'))
           s++;
   }
   return s;
}

/*
  Move a pointer past a quoted phrase, or to the end of the string
*/
static char* skip_quote (char* s)
{
   char quote;
	
   if (s == 0 || *s == 0)
       return s;
	
   quote = *s++;
	
   while (*s && *s != quote)
   {
       s++;
   }
	
   if (*s)
   {
       s++;
   }
   return s;
}

static int err_out (const char* func, char* fmt, ...)
 {
     va_list ap;
     
     fprintf(stderr, "%s error: ", func);
     va_start(ap, fmt);
     vfprintf(stderr, fmt, ap);
     va_end(ap); 
     fprintf(stderr, "\n");
     return -1;
 }
