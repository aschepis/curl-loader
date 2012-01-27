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
#include <util/vbasedefs.h>
#include <stdio.h>


V_EXPORT void  VCONTEXT_default_memory_error_callback(struct tagVCONTEXT *ctx, int error, void *ptr)
{
	char arg_msg[120];

	V_UNUSED(ctx);
	sprintf(arg_msg,"error=%d ptr=%p", error, ptr); 

	fprintf(stderr, "MEMERROR: %s [%s]\n", VERROR_get_message( error ), arg_msg);
}

void *V_malloc(struct tagVCONTEXT *ctx, size_t sz)
{
	void *ret;

	ret = malloc(sz);
	if (!ret) {
 		V_CONTEXT_ERROR(ctx, VCONTEXT_ERROR_OUT_OF_MEMORY, 0 );
	}
	return ret;
}

void V_free(struct tagVCONTEXT *ctx,  void *buf)
{
	V_UNUSED(ctx);
	free(buf);
}

void *V_realloc(struct tagVCONTEXT *ctx, void *buf, size_t sz)
{
	void *ret;

	ret = realloc(buf, sz);
	if (!ret) {
		V_CONTEXT_ERROR(ctx, VCONTEXT_ERROR_OUT_OF_MEMORY, 0 );
	}
	return ret;
}

V_EXPORT void VCONTEXT_init_null( VCONTEXT *ctx )
{
  ctx->free = V_free;
  ctx->malloc = V_malloc;
  ctx->realloc = V_realloc;
  ctx->error_callback = VCONTEXT_default_memory_error_callback;
  ctx->statistics = 0; /* would be standard library implementation specific, want that ?*/
}


static VCONTEXT default_ctx = { V_malloc, V_free, V_realloc, 0, 0, VCONTEXT_default_memory_error_callback };


static VCONTEXT *vlibDefaultContextPtr = &default_ctx;


V_EXPORT VCONTEXT *VCONTEXT_get_default_ctx()
{
	return vlibDefaultContextPtr;
}

V_EXPORT void VCONTEXT_set_default_ctx( VCONTEXT *ctx ) 
{
	vlibDefaultContextPtr = ctx;
}

