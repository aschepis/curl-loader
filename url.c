/*
*     url.c
*
* 2007 Copyright (c) 
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

#include <stdlib.h>
#include <errno.h>

#include "url.h"

double get_random ();

int
current_url_completion_timeout (unsigned long *timeout, 
                                url_context* url, 
                                unsigned long now)
{
  (void) now;

  if (!timeout || ! url)
    {
      fprintf(stderr, "%s error: wrong input.\n", __func__);
      return -1;
    }

  if (! url->timer_url_completion_hrange)
    {
      *timeout = url->timer_url_completion_lrange;
      return 0;
    }

  *timeout = url->timer_url_completion_lrange + 
          (unsigned long) (url->timer_url_completion_hrange * get_random());

  return 0;
}

int
current_url_sleeping_timeout (unsigned long *timeout, 
                              url_context* url, 
                              unsigned long now)
{
  (void) now;

  if (!timeout || ! url)
    {
      fprintf(stderr, "%s error: wrong input.\n", __func__);
      return -1;
    }
  
  if (! url->timer_after_url_sleep_hrange)
    {
      *timeout = url->timer_after_url_sleep_lrange;
      return 0;
    }

  *timeout = url->timer_after_url_sleep_lrange + 
          (unsigned long) (url->timer_after_url_sleep_hrange * get_random());

  return 0;
}
