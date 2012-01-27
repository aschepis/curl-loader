/*
*     vcache.h
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


#ifndef _VCACHE_H_
#define _VCACHE_H_

/* 
 * !!! this is not a cache - this is an object pool !!!
   !!! Cache is to retrieve object by id (don't do this here)
 */

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>
#include <util/vdlistunrolled.h>


typedef enum {
  VCACHE_FIRST_IN_FIRST_OUT,
  VCACHE_LAST_IN_FIRST_OUT,
//CACHE_LEAST_FREQUENTLY_USED,
} VCACHE_POLICY;
 
typedef int  (*VCACHE_OBJECT_CTOR_DTOR) (void *obj, size_t size, void *ctx);


typedef struct {
    size_t in_cached_object_count; /** number of objects ready in cache*/
	size_t in_used_object_count;   /** number of objects returned to user */
} VCACHE_STATISTICS;


typedef struct {

	void *data;

} VCACHE_ENTRY_INFO;

/**
 * @brief Interface for object caching, all elements are of the same size.
 *	- interface for not strict cache , i.e. cache does not have an upper bound for number of elements
 *  - interface doe not specify a maximum element lifetime.
 */
typedef struct {

	VCONTEXT	   *data_ctx;			/** memory allocator */
	VCONTEXT	   *general_ctx;

    /* object initialisation/destruction */
    VCACHE_OBJECT_CTOR_DTOR ctor;	/** function pointer:  constructor for cached object */ 
    VCACHE_OBJECT_CTOR_DTOR dtor;	/** function pointer:  destructor for cached object */
    void	       *ctor_dtor_ctx;  /** data context passed to object constructor destructor */
    
	VCONTEXT_ON_ERROR  on_error;	/** error callback */

	/* data */    
	VCACHE_POLICY  policy;
    size_t	       elmsize;			/** size of cached object */		

	size_t		   upper_bound_of_cached_copies;
	size_t		   upper_bound_of_copies;


	/* cache data */
	VDLISTUNROLLED cached_data;
//	VCACHE_STATISTICS	stats;

} VCACHE;

#if 0
V_EXPORT void VCACHE_init( VCONTEXT *data_ctx,
						   VCONTEXT *general_ctx, 

						   VCACHE *cache, 
						   VCACHE_POLICY policy, 

						   size_t elmsize, 
						   , 
						   VCACHE_OBJECT_CTOR_DTOR ctor, 
						   VCACHE_OBJECT_CTOR_DTOR dtor, 
						   void *ctor_dtor_ctx,
			  VCONTEXT_ON_ERROR on_error)
{ 
	if (!data_ctx) {
		data_ctx = VCONTEXT_get_default_ctx();
	}

	if (!general_ctx) {
		general_ctx = VCONTEXT_get_default_ctx();
	}

	cache->data_ctx = data_ctx;
	cache->general_ctx = general_ctx;

    cache->elmsize = elmsize;
	cache->policy = policy;

    cache->ctor = ctor;
    cache->dtor = dtor;
    cache->ctor_dtor_ctx = ctor_dtor_ctx;
    cache->on_error = on_error;

	/* initially unbounded cache */
	cache->upper_bound_of_cached_copies = (size_t) -1;
	cache->upper_bound_of_copies = (size_t) -1;

	if (VDLISTUNROLLED_init( general_ctx, cache->cached_data, sizeof(VCACHE_ENTRY_INFO), 16)) {
		return -1;
	}
	return 0;
}   


/*
 * @brief returns an allocated object, if no entries are available in cache a none is created
 */
V_EXPORT void * VCACHE_object_alloc(VCACHE *cache)
{
	void *ptr;

	ptr = VCACHE_object_cached(cache);
	if (ptr) {
		return ptr;
	}
		
	ptr = V_MALLOC(cache->data_ctx, cache->elmsize );
	if (!ptr) {
		return 0;
	}

	if (cache->ctor( ptr, cache->elmsize, cache->ctor_dtor_ctx )) {
		V_FREE(cache->data_ctx, ptr );
		return 0;
	}

	cache->stats.in_used_object_count++;
	return ptr;
}

/*
 * @brief returns an object from the cache, only if cached instance is available
 */
V_EXPORT int  VCACHE_object_cached(VCACHE *cache)
{
	VCACHE_ENTRY_INFO entry;

	if ( !VDLISTUNROLLED_pop_front(list, &entry, sizeof(entry) )) {
		cache->stats.in_used_object_count++
		return entry.data;
	}
	return 0;
}


/*
 * @brief returns an object to the cache.
 */
int  VCACHE_object_free(VCACHE *cache, void *ptr)
{

	switch(cache->policy) {
	case VCACHE_FIRST_IN_FIRST_OUT:
		VDLISTUNROLLED_push_front(list, &entry, sizeof(entry) )) {
		break;
	case VCACHE_LAST_IN_FIRST_OUT:
		VDLISTUNROLLED_push_back(list, &entry, sizeof(entry) )) {
		break;
	}
	cache->stats.in_used_object_count--;
	cache->stats.in_cached_object_count++;
	return 0;
}

/*
 * @brief destroys cached object and returns memory to allocator
 */
int  VCACHE_object_destroy(VCACHE *cache, void *ptr)
{
	cache->dtor( ptr, cache->elmsize, cache->ctor_dtor_ctx );

	V_FREE(cache->data_ctx, ptr );

	cache->stats.in_used_object_count--;	

	return 0;
}
/**
 * @brief free cache - destroys all cached objects and marks all memory for reuse 
 */
V_EXPORT int VCACHE_reset( VCACHE *cache)
{
}

/**
 * @brief free cache - destroys all cached objects and marks all memory for reuse 
 */
V_EXPORT int VCACHE_destroy( VCACHE *cache)
{
}

/**
 * @brief return cache statistics 
 */
V_EXPORT int VCACHE_stats (VCACHE *cache, VCACHE_STATISTICS *ptr)
{
}


/*
 * @brief return a object to the cache, now it is a cached instance.
 */
int  VCACHE_object_free(VCACHE *cache, void *ptr)
{
}
#endif

#ifdef  __cplusplus
}
#endif

#endif
