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

#include <memalloc/vtrackalloc.h>

void *VTRACKALLOC_interface_malloc(struct tagVCONTEXT *ctx, size_t sz);
void VTRACKALLOC_interface_free(struct tagVCONTEXT *ctx,  void *buf);
void *VTRACKALLOC_interface_realloc(struct tagVCONTEXT *ctx, void *buf, size_t sz);
int  VTRACKALLOC_interface_stats(struct tagVCONTEXT *ctx, struct tagVCONTEXT_MEM_STATS *out);
int VTRACKALLOC_interface_reset(struct tagVCONTEXT *ctx);

V_EXPORT int VTRACKALLOC_init(VCONTEXT *ctx,VTRACKALLOC *alloc) 
{
	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	alloc->base_interface.malloc = VTRACKALLOC_interface_malloc;
	alloc->base_interface.free	 = VTRACKALLOC_interface_free;
	alloc->base_interface.realloc = VTRACKALLOC_interface_realloc;
	
	alloc->base_interface.error_callback = ctx->error_callback;
	if (!alloc->base_interface.error_callback) {
		alloc->base_interface.error_callback = VCONTEXT_default_memory_error_callback;
	}
	
	alloc->base_interface.statistics = VTRACKALLOC_interface_stats;
	alloc->base_interface.reset = VTRACKALLOC_interface_reset;

	alloc->ctx = ctx;
	
	VDRING_init( &alloc->links);
	
	return 0;
}


V_EXPORT void VTRACKALLOC_reset( VTRACKALLOC *ctx)
{
    VDRING *cur,*next;
    VTRACKALLOC *alloc = (VTRACKALLOC *) ctx;
	
    VDRING_FOREACH_SAVE( cur, next, &alloc->links ) {
		V_FREE( alloc->ctx, cur );	
    }
}	


V_EXPORT void VTRACKALLOC_free( VTRACKALLOC *alloc )
{
	VTRACKALLOC_reset( alloc );
}

int VTRACKALLOC_interface_reset(struct tagVCONTEXT *ctx)
{
	VTRACKALLOC *alloc = (VTRACKALLOC *) ctx;
	VTRACKALLOC_reset( alloc );
	return 0;
}

void *VTRACKALLOC_interface_malloc(struct tagVCONTEXT *ctx, size_t sz)
{
	VTRACKALLOC *alloc = (VTRACKALLOC *) ctx;
	VTRACKALLOC_ARENA * ret;
	
	alloc = (VTRACKALLOC *) ctx;
	
	ret = (VTRACKALLOC_ARENA *) V_MALLOC(alloc->ctx, sz + sizeof(VTRACKALLOC_ARENA) ); 
	if (!ret) {
	    return 0;
	}

	VDRING_push_back( &alloc->links, &ret->link);
	
	return ret + 1;
}


void VTRACKALLOC_interface_free(struct tagVCONTEXT *ctx,  void *buf)
{
    VTRACKALLOC *alloc = (VTRACKALLOC *) ctx;
    VTRACKALLOC_ARENA *arena =  ((VTRACKALLOC_ARENA *) buf) - 1; 

    VDRING_unlink( &arena->link );
    V_FREE( alloc->ctx, arena ); 
}


void *VTRACKALLOC_interface_realloc(struct tagVCONTEXT *ctx, void *buf, size_t sz)
{
    VTRACKALLOC *alloc = (VTRACKALLOC *) ctx;
    VTRACKALLOC_ARENA *arena =  ((VTRACKALLOC_ARENA *) buf) - 1; 

    VDRING_unlink( &arena->link );
	arena = V_REALLOC(alloc->ctx, arena, sz + sizeof(VTRACKALLOC_ARENA) );
	if (!arena) {
		return 0;
	}
	VDRING_push_back( &alloc->links, &arena->link);
	
	return arena + 1;
}

int  VTRACKALLOC_interface_stats(struct tagVCONTEXT *ctx, struct tagVCONTEXT_MEM_STATS *out)
{
	V_UNUSED(ctx);
	V_UNUSED(out);
	return 0;
}

V_EXPORT int VTRACKALLOC_check( VTRACKALLOC *alloc )
{
	V_UNUSED(alloc);
	return 1;
}



