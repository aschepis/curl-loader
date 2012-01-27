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

#include <memalloc/vzonealloc.h>

V_STATIC_INLINE size_t	VZONEHEAP_CHUNK_free_count(VZONEHEAP_CHUNK *heap);
V_STATIC_INLINE void *	VZONEHEAP_CHUNK_alloc(VZONEHEAP_CHUNK *current, size_t sz, size_t alignment);
V_STATIC_INLINE int		VZONEHEAP_add_chunk( VZONEHEAP *heap);
V_STATIC_INLINE void *	VZONEHEAP_allocate_big(VZONEHEAP *heap, size_t sz);
V_STATIC_INLINE void	VZONEHEAP_init_stc( VZONEHEAP *heap );

void *VZONEHEAP_malloc(VZONEHEAP *heap, size_t sz);
void *VZONEHEAP_realloc(VZONEHEAP *heap, void *ptr, size_t sz);

void *VZONEHEAP_interface_malloc(struct tagVCONTEXT *ctx, size_t sz);
void *VZONEHEAP_interface_realloc(struct tagVCONTEXT *ctx, void *buf, size_t sz);
void  VZONEHEAP_interface_free(struct tagVCONTEXT *ctx, void *buf);
int   VZONEHEAP_interface_reset(struct tagVCONTEXT *ctx);


V_EXPORT int VZONEHEAP_init(VCONTEXT *ctx, VZONEHEAP *heap, size_t chunksize, size_t alignment)
{
	heap->base_interface.malloc = VZONEHEAP_interface_malloc;
	heap->base_interface.realloc = VZONEHEAP_interface_realloc;
	heap->base_interface.free = VZONEHEAP_interface_free;
	heap->base_interface.reset = VZONEHEAP_interface_reset;
	heap->base_interface.statistics = 0;

	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	if (ctx->error_callback) {
		heap->base_interface.error_callback = ctx->error_callback;
	} else {
		heap->base_interface.error_callback = VCONTEXT_default_memory_error_callback;
	}

	heap->ctx = ctx;
	heap->alignment = alignment;
	heap->chunk_size = chunksize;

	VZONEHEAP_init_stc(heap);
	return 0;
}

V_EXPORT int VZONEHEAP_free(VZONEHEAP *heap)
{
    VSLIST_entry *entry, *next;

    VSLIST_FOREACH_SAVE( entry, next, &heap->chunks ) {
		V_FREE( heap->ctx, entry );
    }
    VSLIST_FOREACH_SAVE( entry, next, &heap->bigallocs ) {
		V_FREE( heap->ctx, entry );	
    }
	VZONEHEAP_init_stc(heap);
	return 0;
}

void *VZONEHEAP_interface_malloc(struct tagVCONTEXT *ctx, size_t sz)
{
	void *ret;
	VZONEHEAP *alloc = (VZONEHEAP *) ctx;

	ret = VZONEHEAP_malloc(alloc, sz);
	if (!ret) {	
		V_CONTEXT_ERROR(ctx, VCONTEXT_ERROR_OUT_OF_MEMORY, 0 );
		return 0;
	}
	return ret;
}

void VZONEHEAP_interface_free(struct tagVCONTEXT *ctx,  void *buf)
{
	V_UNUSED(ctx);
	V_UNUSED(buf);
}

/* Ups, realloc is not supported
   Realloc needs to copy existing area into new area, meaning that we 
   have to keep track of size of memory block, meaning that 
   we have to add an arena header to each allocation.
   Adding another 8 bytes is a bit costly, just for this purpose,

   There are enough uses where you don't need realloc, so
   this is a limitation.
 */
void *VZONEHEAP_interface_realloc(struct tagVCONTEXT *ctx, void *buf, size_t sz)
{
	V_UNUSED(ctx);
	V_UNUSED(buf);
	V_UNUSED(sz);
	return 0;
#if 0

	void *ret = VZONEHEAP_interface_malloc( ctx, sz );
	if (ret) {
		/* !!!
		 * very very bad, this will crash: must know the source size, at least till end of heap 
		 * have to add a size header before pointer
		 * !!!
		 */
		memcpy(ret, buf, sz);
		return buf;
	} else {
		V_CONTEXT_ERROR(ctx, VCONTEXT_ERROR_OUT_OF_MEMORY, 0 );
	}
#endif
	return 0;
}

int   VZONEHEAP_interface_reset(struct tagVCONTEXT *ctx)
{
	VZONEHEAP *alloc = (VZONEHEAP *) ctx;
	return VZONEHEAP_free(alloc);
}

void *VZONEHEAP_malloc(VZONEHEAP *heap, size_t sz)
{
    size_t sza;

    sza = VUTIL_align( sz, heap->alignment );
    if (sza > heap->chunk_size) {
		/* allocate directly */
		return VZONEHEAP_allocate_big(heap, sz);
    }

    if (!heap->current) {
alloc_chunk:    
		if (VZONEHEAP_add_chunk(heap)) {
			V_CONTEXT_ERROR( &heap->base_interface, VCONTEXT_ERROR_OUT_OF_MEMORY, 0);
			return 0;
		}   
    }
   
    if (VZONEHEAP_CHUNK_free_count(heap->current) > sza) {
		void *rt = VZONEHEAP_CHUNK_alloc(heap->current,sza, heap->alignment); 
		heap->user_allocated_count += sza;
		return rt;
    } else {
		/* what do we now ? - one option is to move on and allocate another chunk - not the best thing to do. */
		goto alloc_chunk; 
    }
    return 0;
}

V_STATIC_INLINE void VZONEHEAP_init_stc( VZONEHEAP *heap )
{
	VSLIST_init( &heap->bigallocs );
	VSLIST_init( &heap->chunks );

	heap->current = 0;
    heap->user_allocated_count = 0;
    heap->total_allocated_count = 0;
}


V_STATIC_INLINE int VZONEHEAP_add_chunk( VZONEHEAP *heap)
{
    VZONEHEAP_CHUNK *chunk = V_MALLOC(heap->ctx, sizeof(VZONEHEAP_CHUNK) + heap->chunk_size );
    if (!chunk) {
		return -1;
    }

    chunk->alloc_data = (V_UINT8 *) (chunk + 1);
	chunk->eof_buffer =  ((V_UINT8 *) chunk->alloc_data) + heap->chunk_size;
    chunk->alloc_data = VUTIL_ptr_align( chunk->alloc_data, heap->alignment );

    heap->current = chunk;
    VSLIST_push_back( &heap->chunks, &chunk->link );
    
    heap->total_allocated_count += heap->chunk_size;

    return 0;
}

V_STATIC_INLINE void *VZONEHEAP_allocate_big(VZONEHEAP *heap, size_t sz)
{
    size_t hdralign; 
    V_UINT8 *ret;    
    VZONEHEAP_HDR_BIGALLOC *hdr;

    hdralign = VUTIL_align( sizeof(VZONEHEAP_HDR_BIGALLOC), heap->alignment ); 
    ret = V_MALLOC( heap->ctx, sz + hdralign );     
    if (!ret) {
		return 0;
    }
 
    hdr = (VZONEHEAP_HDR_BIGALLOC *) ret ;

    hdr->size = sz; 
    VSLIST_push_back( &heap->bigallocs, &hdr->link );
    
    heap->total_allocated_count += sz + hdralign;

    return ret + hdralign;
 
}

V_STATIC_INLINE size_t VZONEHEAP_CHUNK_free_count(VZONEHEAP_CHUNK *heap)
{
    return heap->eof_buffer - heap->alloc_data;
}

V_STATIC_INLINE void * VZONEHEAP_CHUNK_alloc(VZONEHEAP_CHUNK *current, size_t sz, size_t alignment)
{
    void *ret = current->alloc_data;

    current->alloc_data += sz;
    current->alloc_data = VUTIL_ptr_align( current->alloc_data, alignment );
    if (current->alloc_data > current->eof_buffer) {
		current->alloc_data = current->eof_buffer;	
	}
    return ret;
}

