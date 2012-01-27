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

#ifndef _VCONTEXT_H_
#define _VCONTEXT_H_


#ifdef  __cplusplus
extern "C" {
#endif


struct tagVCONTEXT;
struct tagVCONTEXT_MEM_STATS;

typedef void   (*VCONTEXT_ON_ERROR)(struct tagVCONTEXT *ctx, int error, void *ptr);

typedef void * (*VCONTEXT_MALLOC)  (struct tagVCONTEXT *ctx, size_t size);
typedef void * (*VCONTEXT_REALLOC) (struct tagVCONTEXT *ctx, void *buf, size_t size);
typedef void   (*VCONTEXT_FREE)	   (struct tagVCONTEXT *ctx, void *buf );
typedef int    (*VCONTEXT_MEMSTATS)(struct tagVCONTEXT *ctx, struct tagVCONTEXT_MEM_STATS *out);
typedef int    (*VCONTEXT_RESET)   (struct tagVCONTEXT *ctx);


/** 
 * @brief Interface for memory allocation.
 * The object contains function pointer to function that implement dynamic memory allocation.
 * Each object in this library that allocates memory stores a pointer to this interface,
 * 
 * This interface enables us to implement per user context Heaps.
 */
typedef struct tagVCONTEXT
{
	/* mandatory interface */
	VCONTEXT_MALLOC	  malloc;  /** allocate block of memory from pool */
	VCONTEXT_FREE	  free;	   /** return block of previously allocated memory to the pool */
	VCONTEXT_REALLOC  realloc; /** reallocate a memory block */

	/* optional interface */
	VCONTEXT_MEMSTATS  statistics; /** statistics on memory allocator (optional) */
	VCONTEXT_RESET	   reset; /** reset allocator for reuse */

	/* callbacks */
	VCONTEXT_ON_ERROR  error_callback; /** callback: function is called to report error conditions */

} VCONTEXT;


#define V_MALLOC(ctx,size)				(ctx)->malloc( (ctx), (size) )

#define V_REALLOC(ctx, buf, size)		(ctx)->realloc( (ctx), (buf), (size) )

#define V_FREE(ctx, buf)				(ctx)->free( (ctx), (buf) )

#define V_CONTEXT_ERROR(ctx, err, ptr)	do { if (ctx) { (ctx)->error_callback( (ctx), (err), (ptr) ); }  } while(0);

V_INLINE int VCONTEXT_memstats(VCONTEXT *ctx, struct tagVCONTEXT_MEM_STATS *stats)
{
	if (ctx && ctx->statistics) {
		return ctx->statistics( ctx, stats );
	}
	return  -1;
}

V_INLINE int VCONTEXT_reset(VCONTEXT *ctx)
{
	if (ctx && ctx->statistics) {
		return ctx->reset( ctx );
	}
	return  -1;
}

/**
 * @brief get error callback for current memory context.
 */
V_INLINE VCONTEXT_ON_ERROR VCONTEXT_get_error_callback( VCONTEXT *ctx )
{
	return ctx->error_callback;
}

/**
 * @brief set error callback for current memory context.
 */
V_INLINE void VCONTEXT_set_error_callback( VCONTEXT *ctx, VCONTEXT_ON_ERROR on_error )
{
	ctx->error_callback = on_error;
}


/**
 * @brief Default error callback, used by implementations of memory interface.
 */
V_EXPORT void  VCONTEXT_default_memory_error_callback(struct tagVCONTEXT *ctx, int error_msg, void *ptr);


V_EXPORT void VCONTEXT_init_null( VCONTEXT *ctx );

/**
 * @brief get default memory allocator object
 */
V_EXPORT VCONTEXT *VCONTEXT_get_default_ctx();

/**
 * @brief set default memory allocator object.
 * @param ctx (in) this object will now be the default memory allocator.
 */
V_EXPORT void VCONTEXT_set_default_ctx( VCONTEXT *ctx );



/**
 * @brief structure contains report of memory statistics
 * If a field is not computed by the particular implementation then it is set to -1
 */
typedef struct tagVCONTEXT_MEM_STATS {
	size_t			  bigblocks_count;       /** number of big blocks allocated */
	size_t            reserved_count;		 /** number of bytes that are in use by allocator (space of big blocks and internal bookkeeping stuff) */           
	size_t			  user_allocated_count;  /** number of bytes returned to user */
	size_t			  total_allocated_count; /** number of bytes allocated (arena headers + returned to user + internal bookeeping of allocated buffers) */ 
	
} VCONTEXT_MEM_STATS;

V_INLINE void VCONTEXT_MEM_STATS_clear(VCONTEXT_MEM_STATS *stats)
{
	stats->bigblocks_count = (size_t) -1; 
	stats->reserved_count = (size_t) -1;
	stats->user_allocated_count = (size_t) -1;
	stats->total_allocated_count = (size_t) -1;
}

#ifdef  __cplusplus
}
#endif

#endif

