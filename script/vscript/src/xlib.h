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
#ifndef _XLIB_H_
#define _XLIB_H_

#include <util/varr.h>
#include <util/vbuckethash.h>

struct tagAST_BASE;

/* *** extension methods - internal interface *** */

typedef enum {
  XMETHODLIB_OK,
  XMETHODLIB_METHOD_INTERNAL_ERROR,
  XMETHODLIB_METHOD_ID_REPEATED,
  XMETHODLIB_METHOD_ALREADY_DEFINED,
}
  XMETHODLIB_STATUS;

typedef enum {
  XMETHOD_ASYNCH,
  XMETHOD_CALLBACK,
  XMETHOD_NOTIMPLEMENTED,
}
  XMETHOD_TYPE;

typedef struct tagXMETHODACTION {
	XMETHOD_TYPE action_type;
	int   method_id;
   	void *callback_ptr;
}  XMETHODACTION;

typedef struct tagXMETHODLIB {
 
  int	open_method;	// for cl, for runtime or both.

  VBUCKETHASH hash; // map name to function declaration,
  
  VBUCKETHASH unique_id; // maintains unique external function id.
  
  VARR		  map_id_to_action; // map internal function id to an XMETHODACTION

} XMETHODLIB;

XMETHODLIB *XMETHODLIB_init(int open_mode);

int XMETHODLIB_free(XMETHODLIB *xmethods);

struct  tagAST_FUNCTION_DECL *XMETHODLIB_find(XMETHODLIB *xmethods, struct tagAST_BASE *fcall);

XMETHODACTION * XMETHODLIB_lookup_action(XMETHODLIB *xmethods, size_t idx );

#endif

