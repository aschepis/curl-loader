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

#include <memalloc/vfixedpool.h>
#include <util/vutil.h>

/*
 arena header in allocated block
	 - ptr to start of big block (slub does not need this since we know the start of page address)
	 - Require alignment ???

  option - return memory when needed or not4 ?????

 */


typedef struct {
	VDRING	link;

	V_UINT32 freelist_idx;
	size_t	elmfree;
	V_UINT8	*buffer; 
} 
	VIXEDPOOL_MEMORY_BLOCK;


void *VFIXEDPOOL_malloc_mem(struct tagVCONTEXT *ctx, size_t sz);
void  VFIXEDPOOL_free_mem(struct tagVCONTEXT *ctx,  void *buf);
void *VFIXEDPOOL_realloc_mem(struct tagVCONTEXT *ctx, void *buf, size_t sz);

static void					   *VIXEDPOOL_MEMORY_BLOCK_malloc(VIXEDPOOL_MEMORY_BLOCK *elm, size_t elmsize);
static VIXEDPOOL_MEMORY_BLOCK  *VIXEDPOOL_MEMORY_BLOCK_new(VFIXEDPOOL *pool );
static int						VIXEDPOOL_MEMORY_BLOCK_is_free(VIXEDPOOL_MEMORY_BLOCK *elm, void *buf);
static int						VIXEDPOOL_MEMORY_BLOCK_free( VFIXEDPOOL *pool, VIXEDPOOL_MEMORY_BLOCK *elm, void *buf );




V_EXPORT int VFIXEDPOOL_init( VCONTEXT	 *ctx,
							  VFIXEDPOOL *pool,
							  size_t	  alignment, 
							  size_t	  elmsize, 							  
							  size_t	  elmmaxcount, 
							  int		  is_dynamic)
{
	VIXEDPOOL_MEMORY_BLOCK *cur;

	if (!alignment) {
		alignment = V_DEFAULT_STRUCT_ALIGNMENT;
	}

	elmsize = VUTIL_align( elmsize, alignment );

	pool->ctx = ctx;
	pool->base_interface.malloc  = VFIXEDPOOL_malloc_mem;
	pool->base_interface.free    = VFIXEDPOOL_free_mem;
	pool->base_interface.realloc = VFIXEDPOOL_realloc_mem;
	pool->base_interface.error_callback = V_default_memory_error_callback;

	pool->elmsize = elmsize;
	pool->elmmaxcount_per_block = elmmaxcount;
	pool->poolsize = elmsize * elmmaxcount;
	pool->poolscount = pool->elmcount = 0;
	pool->alignment = alignment;

	VDRING_init( &pool->root );

	cur = VIXEDPOOL_MEMORY_BLOCK_new( pool );
	if (cur) {
		return -1;
	}

	pool->is_grow_heap = is_dynamic ? 1 : 0;
	

	VDRING_push_back( &pool->root, &cur->link );

	return 0;
}

V_EXPORT void VFIXEDPOOL_free( VFIXEDPOOL *pool )
{
	VDRING *cur;

	VDRING_FOREACH( cur, &pool->root ) {

	   V_FREE( pool->ctx, VDRING_unlink(cur) );
	}
}


void *VFIXEDPOOL_malloc_mem(struct tagVCONTEXT *ctx, size_t sz)
{	
	VFIXEDPOOL *pool = (VFIXEDPOOL * ) ctx;
	VDRING *cur;
	VIXEDPOOL_MEMORY_BLOCK *elm;
	void *ret = 0;


	if (sz > pool->elmsize) {
		return 0;
	}


retry:

	VDRING_FOREACH( cur, &pool->root ) {
	   elm = (VIXEDPOOL_MEMORY_BLOCK *) cur;

	   ret =  VIXEDPOOL_MEMORY_BLOCK_malloc( elm, pool->elmsize );
	   if (ret) {
		   return ret;
	   }
	}

	if (!ret) {

		if (pool->is_grow_heap) {
				
			/* allocate a new pool entry */
			elm = VIXEDPOOL_MEMORY_BLOCK_new( pool );
			if (!elm) {
				return 0;
			}

			VDRING_push_back( &pool->root, &elm->link );
			goto retry;
		}
	}

	if (ret) {
		pool->elmcount++;
	} else {

		if (ctx->error_callback) {
			ctx->error_callback( ctx, VCONTEXT_OUT_OF_MEMORY, 0 );
		}
	}

	return ret;
}


void VFIXEDPOOL_free_mem(struct tagVCONTEXT *ctx,  void *buf)
{
	VFIXEDPOOL *pool = (VFIXEDPOOL * ) ctx;
	VDRING *cur;
	VIXEDPOOL_MEMORY_BLOCK *elm;
	int ret;


	VDRING_FOREACH( cur, &pool->root ) {
	   elm = (VIXEDPOOL_MEMORY_BLOCK *) cur;

	   ret = VIXEDPOOL_MEMORY_BLOCK_free( pool, elm, buf );

	   if (!ret) {

		   if (elm->elmfree == pool->elmmaxcount_per_block) {
			   V_FREE( pool->ctx, VDRING_unlink(cur) );
			   pool->poolscount --;
		   }
		   
	  	   pool->elmcount++;		   
		   return ;
	   }

	   if (ret < 0) {
		   /* error occured */
		   return ;
	   }

	}

	if (ctx->error_callback) {
		/* error - pointer not in heep */
		ctx->error_callback(ctx, VCONTEXT_FREE_PTR_NOT_ALLOCATED, buf);
	}

}

void *VFIXEDPOOL_realloc_mem(struct tagVCONTEXT *ctx, void *buf, size_t sz)
{
	VFIXEDPOOL *pool = (VFIXEDPOOL * ) ctx;
	if (sz > pool->elmsize) {
		VFIXEDPOOL_free_mem( ctx, buf );
		/* todo ERROR */
		return 0;
	}
	return buf;
}

static VIXEDPOOL_MEMORY_BLOCK *VIXEDPOOL_MEMORY_BLOCK_new(VFIXEDPOOL *pool )
{
	VIXEDPOOL_MEMORY_BLOCK * ret; 
	V_UINT32 i;
	V_UINT32 *ptr;

	if (!pool->ctx) {
		return 0;
	}
	
	ret = V_MALLOC(pool->ctx, sizeof(VIXEDPOOL_MEMORY_BLOCK) + pool->elmsize * pool->elmmaxcount_per_block + pool->alignment);
	if (!ret) {
		return 0;
	}
	ret->buffer = VUTIL_ptr_align(  (V_UINT8 *) (ret + 1),  pool->alignment );

	ret->elmfree = pool->elmmaxcount_per_block;
	
	for(i = 0; i < ret->elmfree; i++ ) {
		ptr = (V_UINT32 *) (ret->buffer + (i * pool->elmsize));
		*ptr = i < ret->elmfree ? ( (i + 1) * pool->elmsize): 0;		
	}

	pool->poolscount ++;

	return ret;
}

int	VIXEDPOOL_MEMORY_BLOCK_free( VFIXEDPOOL *pool, VIXEDPOOL_MEMORY_BLOCK *elm, void *buf )
{	
	int ret = 1;

	if ((V_UINT8 *) buf > elm->buffer && 
		(V_UINT8 *) buf < (elm->buffer + pool->poolsize)) {

		if (VIXEDPOOL_MEMORY_BLOCK_is_free(elm, buf)) {
			if (pool->base_interface.error_callback) {
				pool->base_interface.error_callback( &pool->base_interface, VCONTEXT_DOUBLE_FREE, buf );
			}
			return -1;
		}

		/* pointer must be aligned on elmsize */
		if ( ( ((V_UINT8 *) buf - elm->buffer) % pool->elmsize) != 0) {
			if (pool->base_interface.error_callback) {
				pool->base_interface.error_callback( &pool->base_interface, VCONTEXT_FREE_PTR_NOT_ALLOCATED, buf );
			}
			return -1;
		}

		* ((V_UINT32 *) buf) = elm->freelist_idx;
		elm->freelist_idx = ((V_UINT8 *) buf - elm->buffer);

		elm->elmfree ++;

		ret = 0;
	}

	return ret;
}



static void *VIXEDPOOL_MEMORY_BLOCK_malloc(VIXEDPOOL_MEMORY_BLOCK *elm, size_t elmsize)
{
   void *ret = 0;

   if (elm->freelist_idx != (size_t) -1) {
	
	   ret  = elm->buffer + elm->freelist_idx;
	   elm->freelist_idx = * ((V_UINT32 *) (elm->buffer + elm->freelist_idx));	   
	   elm->elmfree --;	   

   }
   return ret;
}

static int	VIXEDPOOL_MEMORY_BLOCK_is_free(VIXEDPOOL_MEMORY_BLOCK *elm, void *buf)
{
	V_UINT32 pos;

	if (elm->freelist_idx != (size_t) -1) {

		pos = elm->freelist_idx;
		while (pos != -1) {

			if ((elm->buffer + pos) == (V_UINT8 *) buf) {
				return 1;
			}

			pos = * ((V_UINT32 *) (elm->buffer + pos));
		}
		

	}
	return 0;

}


