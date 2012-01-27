/*
*
* 2007 Copyright (c) 
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

#ifndef _VERROR_H_
#define _VERROR_H_

#ifdef  __cplusplus
extern "C" {
#endif

/** 
 * @brief enumeration of error conditions
 */
typedef enum {
  VCONTEXT_ERROR_OUT_OF_MEMORY,
  VCONTEXT_ERROR_INVALID_HEAP,
  VCONTEXT_ERROR_DOUBLE_FREE,
  VCONTEXT_ERROR_FREE_PTR_NOT_ALLOCATED,
  VCONTEXT_ERROR_REQUEST_TOO_BIG,
  
  VERROR_LAST,	

} VERROR;

/**
 * @brief turn erro code to string message.
 */
V_EXPORT char* VERROR_get_message( int error );

#ifdef  __cplusplus
}
#endif

#endif
