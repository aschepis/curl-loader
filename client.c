/*
*     client.c
*
* 2006-2007 Copyright (c) 
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

// must be the first include
#include "fdsetsize.h"


#include "client.h"
#include "batch.h"


/*
  Accessors to the flags-counters of headers.
  Incrementing counters for the flags-counters of headers.
*/
int first_hdr_req (client_context* cctx)
{
  return cctx->first_hdr_req;
}
void first_hdr_req_inc (client_context* cctx)
{
  cctx->first_hdr_req++;
}
int first_hdr_1xx (client_context* cctx)
{
  return cctx->first_hdr_1xx;
}
void first_hdr_1xx_inc (client_context* cctx)
{
  cctx->first_hdr_1xx++;
}
int first_hdr_2xx (client_context* cctx)
{
  return cctx->first_hdr_2xx;
}
void first_hdr_2xx_inc (client_context* cctx)
{
  cctx->first_hdr_2xx++;
}
int first_hdr_3xx (client_context* cctx)
{
  return cctx->first_hdr_3xx++;
}
void first_hdr_3xx_inc (client_context* cctx)
{
  cctx->first_hdr_3xx++;
}

int first_hdr_4xx (client_context* cctx)
{
  return cctx->first_hdr_4xx;
}
void first_hdr_4xx_inc (client_context* cctx)
{
  cctx->first_hdr_4xx++;
}
int first_hdr_5xx (client_context* cctx)
{
  return cctx->first_hdr_5xx;
}
void first_hdr_5xx_inc (client_context* cctx)
{
  cctx->first_hdr_5xx++;
}


/*
  Reseting to zero counters for the flags-counters of headers.
*/
void first_hdrs_clear_all (client_context* cctx)
{
  cctx->first_hdr_req = cctx->first_hdr_1xx = cctx->first_hdr_2xx = 
    cctx->first_hdr_3xx = cctx->first_hdr_4xx = cctx->first_hdr_5xx = 0;
}
void first_hdrs_clear_non_req (client_context* cctx)
{
  cctx->first_hdr_1xx = cctx->first_hdr_2xx = cctx->first_hdr_3xx = 
    cctx->first_hdr_4xx = cctx->first_hdr_5xx = 0;
}
void first_hdrs_clear_non_1xx (client_context* cctx)
{
  cctx->first_hdr_req = cctx->first_hdr_2xx = cctx->first_hdr_3xx = 
    cctx->first_hdr_4xx = cctx->first_hdr_5xx = 0;
}
void first_hdrs_clear_non_2xx (client_context* cctx)
{
  cctx->first_hdr_req = cctx->first_hdr_3xx = cctx->first_hdr_4xx = 
    cctx->first_hdr_5xx = 0;
}
void first_hdrs_clear_non_3xx (client_context* cctx)
{
  cctx->first_hdr_req = cctx->first_hdr_2xx = cctx->first_hdr_4xx = cctx->first_hdr_5xx = 0;
}
void first_hdrs_clear_non_4xx (client_context* cctx)
{
  cctx->first_hdr_req = cctx->first_hdr_2xx = cctx->first_hdr_3xx = cctx->first_hdr_5xx = 0;
}
void first_hdrs_clear_non_5xx (client_context* cctx)
{
  cctx->first_hdr_req = cctx->first_hdr_2xx = cctx->first_hdr_4xx = cctx->first_hdr_3xx = 0;
}



void stat_data_out_add (client_context* cctx, unsigned long bytes)
{
  cctx->st.data_out += bytes;

  cctx->is_https ? (cctx->bctx->https_delta.data_out += bytes) :
    (cctx->bctx->http_delta.data_out += bytes);
}

void stat_data_in_add (client_context* cctx, unsigned long bytes)
{
  cctx->st.data_in += bytes;
  
  cctx->is_https ? (cctx->bctx->https_delta.data_in += bytes) :
    (cctx->bctx->http_delta.data_in += bytes);
}

void stat_err_inc (client_context* cctx)
{
  cctx->st.other_errs++;
  cctx->is_https ? cctx->bctx->https_delta.other_errs++ :
    cctx->bctx->http_delta.other_errs++;
}

void stat_url_timeout_err_inc (client_context* cctx)
{
  cctx->st.url_timeout_errs++;
  cctx->is_https ? cctx->bctx->https_delta.url_timeout_errs++ :
    cctx->bctx->http_delta.url_timeout_errs++;
}
void stat_req_inc (client_context* cctx)
{
  cctx->st.requests++;
  cctx->is_https ? cctx->bctx->https_delta.requests++ :
    cctx->bctx->http_delta.requests++;
}
void stat_1xx_inc (client_context* cctx)
{
  cctx->st.resp_1xx++;
  cctx->is_https ? cctx->bctx->https_delta.resp_1xx++ :
    cctx->bctx->http_delta.resp_1xx++;
}
void stat_2xx_inc (client_context* cctx)
{
  cctx->st.resp_2xx++;
  cctx->is_https ? cctx->bctx->https_delta.resp_2xx++ :
    cctx->bctx->http_delta.resp_2xx++;
}
void stat_3xx_inc (client_context* cctx)
{
  cctx->st.resp_3xx++;
  cctx->is_https ? cctx->bctx->https_delta.resp_3xx++ :
    cctx->bctx->http_delta.resp_3xx++;
}
void stat_4xx_inc (client_context* cctx)
{
  cctx->st.resp_4xx++;
  cctx->is_https ? cctx->bctx->https_delta.resp_4xx++ :
    cctx->bctx->http_delta.resp_4xx++;
}
void stat_5xx_inc (client_context* cctx)
{
  cctx->st.resp_5xx++;
  cctx->is_https ? cctx->bctx->https_delta.resp_5xx++ :
    cctx->bctx->http_delta.resp_5xx++;
}

void stat_appl_delay_add (client_context* cctx, unsigned long resp_timestamp)
{
  if (resp_timestamp > cctx->req_sent_timestamp)
    {
      if (cctx->is_https)
        {
          cctx->bctx->https_delta.appl_delay = 
            (cctx->bctx->https_delta.appl_delay * cctx->bctx->https_delta.appl_delay_points +
             resp_timestamp - cctx->req_sent_timestamp) / ++cctx->bctx->https_delta.appl_delay_points;
        }
      else
        {
          cctx->bctx->http_delta.appl_delay = 
            (cctx->bctx->http_delta.appl_delay * cctx->bctx->http_delta.appl_delay_points +
             resp_timestamp - cctx->req_sent_timestamp) / ++cctx->bctx->http_delta.appl_delay_points;
        }
    }
}
void stat_appl_delay_2xx_add (client_context* cctx, unsigned long resp_timestamp)
{
    if (resp_timestamp > cctx->req_sent_timestamp)
    {
      if (cctx->is_https)
        {
          cctx->bctx->https_delta.appl_delay_2xx = 
            (cctx->bctx->https_delta.appl_delay_2xx * cctx->bctx->https_delta.appl_delay_2xx_points +
             resp_timestamp - cctx->req_sent_timestamp) / ++cctx->bctx->https_delta.appl_delay_2xx_points;
        }
      else
        {
          cctx->bctx->http_delta.appl_delay_2xx = 
            (cctx->bctx->http_delta.appl_delay_2xx * cctx->bctx->http_delta.appl_delay_2xx_points +
             resp_timestamp - cctx->req_sent_timestamp) / ++cctx->bctx->http_delta.appl_delay_2xx_points;
        }
    }
}

void dump_client (FILE* file, client_context* cctx)
{
  
  if (!file || !cctx)
	 return;

  fprintf (file, 
           "%s,cycles:%ld,state:%d,b-in:%lld,b-out:%lld,req:%ld,1xx:%ld,2xx:%ld,3xx:%ld,4xx:%ld,5xx:%ld,err:%ld,T-err:%ld\n", 
           cctx->client_name, cctx->cycle_num, cctx->client_state, 
           cctx->st.data_in,  cctx->st.data_out, cctx->st.requests, 
           cctx->st.resp_1xx, cctx->st.resp_2xx, cctx->st.resp_3xx, cctx->st.resp_4xx, cctx->st.resp_5xx, 
           cctx->st.other_errs, cctx->st.url_timeout_errs);
  fflush (file);
}




