/*
*     url.h
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


#ifndef URL_H
#define URL_H

#include <stddef.h>

#include <curl/curl.h>


#define URL_SHORT_NAME_LEN 12
#define URL_AUTH_STR_LEN 64

#define URL_RESPONSE_STATUS_ERRORS_TABLE_SIZE 600


/*
  Application types of URLs.
*/
typedef enum url_appl_type
{
  URL_APPL_UNDEF = 0, /* set by calloc */
  URL_APPL_HTTP,
  URL_APPL_HTTPS,
  URL_APPL_FTP,
  URL_APPL_FTPS,
  URL_APPL_SFTP,
  URL_APPL_TELNET,
}url_appl_type;

typedef enum authentication_method
  {
    AUTHENTICATION_NO = CURLAUTH_NONE, /* set by calloc */
    AUTHENTICATION_BASIC = CURLAUTH_BASIC,
    AUTHENTICATION_DIGEST = CURLAUTH_DIGEST,
    AUTHENTICATION_GSS_NEGOTIATE = CURLAUTH_GSSNEGOTIATE,
    AUTHENTICATION_NTLM = CURLAUTH_NTLM,
    AUTHENTICATION_ANY = CURLAUTH_ANY,
  } authentication_method;

/* 
 * Currently, we are using mainly username and password, but up to 16 
 * tokens/records are available. 
*/
#define FORM_RECORDS_MAX_TOKENS_NUM 16
#define FORM_RECORDS_TOKEN_MAX_LEN 64
#define FORM_RECORDS_SEQ_NUM_LEN 7 /* Up to 10 000 000 clients */

typedef struct form_records_cdata
{
    /*
      On access test, that the form_tokens have been allocated. 
    */
    char* form_tokens[FORM_RECORDS_MAX_TOKENS_NUM];
} form_records_cdata;



/*
  GF
  Structures for storing a set of URLs built from an URL_TEMPLATE such as
  http://www.abc.com/group/%s/user/%s/account
  Each URL is built by replacing the %s's with a token from a file specified
  by URL_TOKEN_FILE, or from an URL_TOKEN obtained in a prior server response.
  This allows each virtual client to make a unique request.
  The token file may also specify a cookie to accompany the request.
*/
typedef struct urle
{
   char*	string;
    char*	cookie;

} urle;

typedef struct url_set
{
   int		n_urles;
   urle*	urles;
   int		index;

} url_set;

/*
  The URL_TEMPLATE is stored here, for both URL_TOKEN and URL_TOKEN_FILE cases,
  although both cannot be used together.
*/
typedef struct
{
    char* string;	/* template string, eg http://www.abc.com/group/%s/user/%s/account */
    int   n_cents;	/* number of %s's in the template */
    int   n_tokens;	/* number of URL_TOKENS parsed so far */
   char** names;	/* URL_TOKEN names */
   char** values;	/* scratch space for collecting token values */

} url_template;

/*
  This url has this many RESPONSE_TOKENS that we must scan for
*/
typedef struct
{
    int n_tokens;
}url_response;
	


/*
  url_context - structure, that concentrates our knowledge 
  about the url to fetch (download, upload, etc).
*/
typedef struct url_context
{
   /* URL buffer, string containing the url */
  char* url_str;

  /* URL buffer length*/
  size_t url_str_len;

  /* The short-cut URL name to be used in Load Status GUI.  */
  char url_short_name [URL_SHORT_NAME_LEN + 1];

  /*
    The flag can be set instead of URL string provisioning. If true, 
    the current url will be used. 
    Useful e.g. when POST-ing to the form, obtained by the previous 
    GET fetching. Cannot be used for the first URL, because there is
    no any "currect" url with CURL handle till at least a single fetch
    has been done.
  */
  int url_use_current;

  /*
    If true, such url is done only once and not cycled. 
    Useful for a single login/logoff operation. A url with such flag to be
    fetched only, when all previous cycling URLs have been accomplished.
  */
  int url_dont_cycle;

  /* 
     Number of custom  HTTP headers in array.
  */
  size_t custom_http_hdrs_num;
  
  /* 
     The list of custom  HTTP headers.
  */
  struct curl_slist *custom_http_hdrs;

  /* 
     Request type/method. Used currently for HTTP only and can be:
     REQ_TYPE_GET_AND_POST, REQ_TYPE_POST, REQ_TYPE_GET
  */
  size_t req_type;

     /* 
      The username to be used to access a URL by filling the POST 
      form or in GET url.
   */
  char username[URL_AUTH_STR_LEN];

  /* 
     The password to be used to access a URL by filling the POST form or 
     in GET url.
  */
  char password[URL_AUTH_STR_LEN];

  /* 
     The type of <form_str>. Valid types are: 

     FORM_USAGETYPE_UNIQUE_USERS_AND_PASSWORDS
     - like "user=%s%d&password=%s%d";

     FORM_USAGETYPE_UNIQUE_USERS_SAME_PASSWORD 
     - like "user=%s%d&password=%s";

     FORM_USAGETYPE_SINGLE_USER  - like "user=%s&password=%s";

     FORM_USAGETYPE_RECORDS_FROM_FILE
     Record file enables up to 16 record tokens to be used. Thus, an appropriate
     form to include up to 16 form names with "=%s" to be filled by the record tokens
     from a file:
     "form1=%s&form2=%s&form3=%s.... &form16=%s", which for user and password form could look like:
     "user=%s&password=%s

     FORM_USAGETYPE_AS_IS - use the string provided AS IS;
   */
  int form_usage_type;

  /* 
     The string to be used as the base for login post message 
  */
  char* form_str;

  /*
    The file with strings like "user:password", where separator may be 
    ',', ';', ':', '@', '/' and ' ' (space). The file may be created as a dump of DB tables 
    of users and passwords.
  */
  char* form_records_file;

  /*
    The array of form records with clients data (cdata). 
    form_records_array[N] is for client number N and contains cdata tokens
    to be used e.g. in POST-ing forms
  */
  form_records_cdata* form_records_array;

  /*
    Number of records in the above array of form records, containing client 
    data (cdata). 
    Normally, to be the same size as the number of clients, but it can 
    be larger as well.
  */
  size_t form_records_num;

  /* The limit of records, passed by configuration. If no limit set and the field is
     zero, we are taking number of clients as the possible maximum */
  size_t form_records_file_max_num;


  /* 
     If 1 set, records are used in a random fashion and not "bound"
     to each client using index (index of client == index of record). 
  */
  size_t form_records_random;

  /*
    Name of the file (with a path, if required) to upload.
  */
  char* upload_file;

  /* Size of the file to upload in bytes. */
  off_t upload_file_size;

  /* File pointer to the open upload file */
  FILE* upload_file_ptr;


  /* Structures for multipart/formdata HTTP POST (rfc1867-style posts) */
  struct curl_httppost* mpart_form_post;
  struct curl_httppost* mpart_form_last;

  /* Web authentication method. If 0 - no Web authentication */
  authentication_method  web_auth_method;
  
  /* 
     Username:password. If NULL, username and password are
     combined to make it.
  */
  char* web_auth_credentials;

  /* Proxy authentication method. If 0 - no proxy authentication */
  authentication_method  proxy_auth_method;
  
  /* 
     Username:password. If NULL, username and password are
     combined to make it.
  */
  char* proxy_auth_credentials;


    /*
      When true, an existing connection will be closed and connection
      will be re-established (attempted).
    */
  long fresh_connect; 

    /*
     Maximum time to establish TCP connection with a server (including resolving).
     If zero, the global connect_timeout default is taken.
   */
  long connect_timeout;

   /*
     Time to accomplish fetching  of a url.
   */
  unsigned long timer_url_completion_lrange;
  unsigned long timer_url_completion_hrange;
  

  /* 
     Time to relax/sleep after fetching this url (msec). The timeout
     actually emulates user behavior. A user normally needs time to 
     read the page retrived prior to making another click.
   */
  unsigned long timer_after_url_sleep_lrange;
  unsigned long timer_after_url_sleep_hrange;

  /* When positive, means ftp-active. The default is ftp-passive. */
  int ftp_active;

  /* Logs headers of HTTP responses to files, when true. */
  int log_resp_headers;

  /* Logs bodies of HTTP responses to files, when true. */
  int log_resp_bodies;

  /* 
     Upper limit for download/upload rate in
     bytes/sec. 
  */
  curl_off_t transfer_limit_rate;

  /*
    Percent probability, that a client will fetch the url.
    Should be from 0 up to 100%, where zero means 100% probability.
  */
  int fetch_probability;

  /*
    Client should make the decision "to fetch or not to fetch" a url
    only once (at the first cycle), remember the decision and to
    follow the once-made decision at all next cycles.
  */
  int fetch_probability_once;


  /************* Assisting Elements    *************/

  /* Application type of url, e.g. HTTP, HTTPS, FTP, etc */
  url_appl_type url_appl_type;

  /*
    Our index in the url array
  */
  long url_ind;

  /* 
     Directory name to be used for logfiles of responses 
     (headers and bodies). 
  */
  char* dir_log;

  /*
    An optional table of response status errors. If a response status is 404,
    when resp_status_errors_tbl[404] is true, and the response is considered 
    as an error. 
    By default 4xx responses without 401 and 407 and all 5xx responses are 
    considered as errors.
  */
  unsigned char *resp_status_errors_tbl;
  

  /*GF */
   url_set set;
   url_template template;
   url_response response;

    /*
      Keep a separate upload-file offset for each client
      Replaces the single upload_file_ptr.
    */
    /* file descriptor for open upload file */
    int upload_descriptor;
    /* allocated, one for each client */
    off_t* upload_offsets;
  
  /*
    Allows form records to be used sequentially as clients cycle.
    See form_records_array above.
   */
  int form_records_cycle;
  size_t form_records_index;

   /*
    Governs how urls are chosen from the url_set, not implemented yet
    */
   int url_cycling;

   /*
    Ignore the content length of the response. The server would normally
    close the connection.
    */
   int ignore_content_length;

   /*
    Randomize a part of the url specified by a token
    */
   int random_lrange;
   int random_hrange;
   char* random_token;

} url_context;


/* GF */
#define is_template(url)	(url->template.string != 0)


int
current_url_completion_timeout (unsigned long *timeout,
                                url_context* url, 
                                unsigned long now);
int
current_url_sleeping_timeout (unsigned long *timeout, 
                              url_context* url, 
                              unsigned long now);

#endif /* URL_H */
