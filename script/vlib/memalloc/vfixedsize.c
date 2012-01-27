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

#include <memalloc/vfixedsize.h>


void *VFIXEDHEAP_interface_malloc(struct tagVCONTEXT *ctx, size_t sz);
void  VFIXEDHEAP_interface_free(struct tagVCONTEXT *ctx,  void *buf);
void *VFIXEDHEAP_interface_realloc(struct tagVCONTEXT *ctx, void *buf, size_t sz);
int   VFIXEDHEAP_interface_stats(struct tagVCONTEXT *ctx, struct tagVCONTEXT_MEM_STATS *out);
int   VFIXEDHEAP_interface_reset(struct tagVCONTEXT *ctx);

V_STATIC_INLINE VFIXEDHEAP_CHUNK_INFO * VFIXEDHEAP_CHUNK_init(VFIXEDHEAP *alloc);
V_STATIC_INLINE int VFIXEDHEAP_CHUNK_free( VFIXEDHEAP *alloc, void *ptr );
V_STATIC_INLINE int VFIXEDHEAP_update_num_free( VFIXEDHEAP *alloc, void *ptr, int delta);

V_EXPORT int VFIXEDHEAP_init(VCONTEXT *ctx,VFIXEDHEAP *alloc,size_t elmsize, size_t alignment, size_t maxcount) 
{
	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	alloc->base_interface.malloc = VFIXEDHEAP_interface_malloc;
	alloc->base_interface.free	 = VFIXEDHEAP_interface_free;
	alloc->base_interface.realloc = VFIXEDHEAP_interface_realloc;
	
	alloc->base_interface.error_callback = ctx->error_callback;
	if (!alloc->base_interface.error_callback) {
		alloc->base_interface.error_callback = VCONTEXT_default_memory_error_callback;
	}
	
	alloc->base_interface.statistics = VFIXEDHEAP_interface_stats;
	alloc->base_interface.reset = VFIXEDHEAP_interface_reset;

	elmsize = VUTIL_align( elmsize, alignment );

    alloc->ctx = ctx;
	alloc->elmsize = elmsize;
	alloc->alignment = alignment;
	alloc->elmmaxcount_per_chunk = maxcount;
	alloc->chunk_size = elmsize * maxcount;
	alloc->chunk_size = VUTIL_align( alloc->chunk_size, alignment);
	alloc->current = 0;
	alloc->freelist = 0;

	if (VARR_init( ctx, &alloc->info_sizes, sizeof(VFIXEDHEAP_CHUNK_INFO), 1)) {
		return -1;
	}

	alloc->num_free = 0;
	
	return 0;
}


V_EXPORT void VFIXEDHEAP_reset( VFIXEDHEAP *alloc )
{
	size_t i;
	VFIXEDHEAP_CHUNK_INFO *info;
	
	for(i=0;i<alloc->info_sizes.elmcount;i++) 
	{
		info  = (VFIXEDHEAP_CHUNK_INFO *) VARR_at( &alloc->info_sizes, i );

		info->num_free = 0;
		info->alloc_data = info->start_buffer;
	}

	alloc->freelist = 0;

	if ( alloc->info_sizes.elmcount ) {
		alloc->current = (VFIXEDHEAP_CHUNK_INFO *) VARR_at( &alloc->info_sizes, 0 );
	} else {
		alloc->current = 0;
	}

	
	alloc->num_free = 0;;
}

V_EXPORT void VFIXEDHEAP_free( VFIXEDHEAP *alloc )
{
	size_t i;
	VFIXEDHEAP_CHUNK_INFO *info;
	
	for(i=0;i<alloc->info_sizes.elmcount;i++) 
	{
		info  = (VFIXEDHEAP_CHUNK_INFO *) VARR_at( &alloc->info_sizes, i );

		V_FREE( alloc->ctx, info->start_buffer);
	}

	VARR_free( &alloc->info_sizes );
	alloc->freelist = 0;
	alloc->current = 0;

	alloc->num_free = 0;
}

V_STATIC_INLINE int VFIXEDHEAP_free_mem( VFIXEDHEAP *alloc, void *ptr )
{
	/* check alignment of pointer */
	if (!VUTIL_ptr_is_aligned(ptr,alloc->alignment)) {
		V_CONTEXT_ERROR(&alloc->base_interface,VCONTEXT_ERROR_FREE_PTR_NOT_ALLOCATED,0);
		return 0;
	}

	/* add entry to free list */
    * ((void **) ptr) = alloc->freelist;
    alloc->freelist = ptr;

	if (VFIXEDHEAP_update_num_free(alloc, ptr, 1)) {
		/* the node is now empty - we have to free it */
		VFIXEDHEAP_CHUNK_free( alloc, ptr );
	}
	return 0;
}


V_STATIC_INLINE void * VFIXEDHEAP_malloc( VFIXEDHEAP *alloc )
{
	VFIXEDHEAP_CHUNK_INFO  *current; 
		
	if (alloc->freelist) {
	  
	  void *ret = alloc->freelist;
	  alloc->freelist = * ((void **) ret);

	  VFIXEDHEAP_update_num_free(alloc, ret, -1);
	  return ret;
	}

new_entry:
	
	current = alloc->current;
	if (current) {
		if (current->alloc_data < current->eof_buffer) {
			void *ret = current->alloc_data;
			current->alloc_data += alloc->elmsize;
			return ret;
		}
	}

	/* allocate new arena and make it current */
	alloc->current = VFIXEDHEAP_CHUNK_init( alloc );
	if (alloc->current) {
		goto new_entry;
	}

	/* out of memory condition */
	return 0;
}

int VFIXEDHEAP_interface_reset(struct tagVCONTEXT *ctx)
{
	VFIXEDHEAP *alloc = (VFIXEDHEAP *) ctx;
	VFIXEDHEAP_reset( alloc );
	return 0;
}

void *VFIXEDHEAP_interface_malloc(struct tagVCONTEXT *ctx, size_t sz)
{
	void *ret;
	VFIXEDHEAP *alloc = (VFIXEDHEAP *) ctx;

	if (alloc->elmsize < sz) {
		V_CONTEXT_ERROR(ctx, VCONTEXT_ERROR_REQUEST_TOO_BIG, 0);
		return 0;
	}

	ret = VFIXEDHEAP_malloc((VFIXEDHEAP *) ctx);
	if (!ret) {
		V_CONTEXT_ERROR(ctx, VCONTEXT_ERROR_OUT_OF_MEMORY, 0 );
		return 0;
	}
	return ret;
}


void VFIXEDHEAP_interface_free(struct tagVCONTEXT *ctx,  void *buf)
{
	VFIXEDHEAP *alloc = (VFIXEDHEAP *) ctx;

	VFIXEDHEAP_free_mem(alloc, buf);
}


void *VFIXEDHEAP_interface_realloc(struct tagVCONTEXT *ctx, void *buf, size_t sz)
{
	VFIXEDHEAP *alloc = (VFIXEDHEAP *) ctx;

	if (alloc->elmsize < sz) {
		V_CONTEXT_ERROR(ctx, VCONTEXT_ERROR_REQUEST_TOO_BIG, 0);
		return 0;
	}

	return buf;
}

int  VFIXEDHEAP_interface_stats(struct tagVCONTEXT *ctx, struct tagVCONTEXT_MEM_STATS *out)
{
	VFIXEDHEAP *alloc = (VFIXEDHEAP *) ctx;
	

	out->bigblocks_count = VARR_size( &alloc->info_sizes );
	//out->reserved_count = 
	//out->inuse_count =  
	return 0;
}

V_EXPORT int VFIXEDHEAP_check( VFIXEDHEAP *alloc )
{
	VFIXEDHEAP_CHUNK_INFO *info;
	size_t i;
	int    current_found = 0;
	size_t num_free_count = 0;
	
	for(i=0;i<alloc->info_sizes.elmcount;i++) 
	{
		info  = (VFIXEDHEAP_CHUNK_INFO *) VARR_at( &alloc->info_sizes, i );

		if (! (info->start_buffer <= info->alloc_data && 
			   info->alloc_data <= info->eof_buffer) ) {
			return 0;
		}

		if (info == alloc->current) {
			current_found = 1;
		}
		num_free_count += alloc->num_free;
	}

	if (!current_found && alloc->current) {
		return 0;
	}

	if (num_free_count !=  alloc->num_free) {
		return 0;
	}


	return 1;
}

static VFIXEDHEAP_CHUNK_INFO * VFIXEDHEAP_CHUNK_init(VFIXEDHEAP *alloc)
{
	VFIXEDHEAP_CHUNK_INFO ret;

	ret.alloc_data = ret.start_buffer = V_MALLOC(alloc->ctx, alloc->chunk_size );
	if (!ret.alloc_data) {
		return 0;
	}
	
	ret.eof_buffer  = ret.alloc_data + alloc->chunk_size;
	ret.num_free  = 0;
	
	/* add chunk info to alloc control structure */
	if (!VARR_push_back( &alloc->info_sizes, &ret, sizeof(ret) )) {
		return (VFIXEDHEAP_CHUNK_INFO * ) 
					VARR_at( &alloc->info_sizes, VARR_size(&alloc->info_sizes) - 1 );
	}

	V_FREE(alloc->ctx, ret.alloc_data );
	return 0;
}

static int VFIXEDHEAP_CHUNK_free( VFIXEDHEAP *alloc, void *ptr )
{
	size_t i;
	VFIXEDHEAP_CHUNK_INFO *info;
	
	for(i=0;i<alloc->info_sizes.elmcount;i++) 
	{
		info  = (VFIXEDHEAP_CHUNK_INFO *) VARR_at( &alloc->info_sizes, i );
		if (VUTIL_ptr_in_range(ptr, info->start_buffer, info->eof_buffer)) {
			
			V_FREE( alloc->ctx, info->start_buffer );
			VARR_delete_at( &alloc->info_sizes,  i );
			return 0;
		}
	}
	/* error condition - array of big blocks corrupted */
	return -1;
}


static int VFIXEDHEAP_update_num_free( VFIXEDHEAP *alloc, void *ptr, int delta)
{
	size_t i;
	VFIXEDHEAP_CHUNK_INFO *info;
	

	for(i=0;i<alloc->info_sizes.elmcount;i++) 
	{
		info  = (VFIXEDHEAP_CHUNK_INFO *) VARR_at( &alloc->info_sizes, i );
		if (VUTIL_ptr_in_range(ptr, info->start_buffer, info->eof_buffer)) {
	
			info->num_free += delta;
			return info->num_free == 0;
		}
	}

	alloc->num_free += delta;

	return 0;
}
