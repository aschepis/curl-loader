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

#ifndef _VCACHE_H_
#define _VCACHE_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>


struct tagVCACHE;

typedef enum {
  VCACHE_FIRST_IN_FIRST_OUT,
  VCACHE_LAST_IN_FIRST_OUT,
  VCACHE_LEAST_FREQUENTLY_USED,
} VCACHE_POLICY;
 
typedef int  (*VCACHE_OBJECT_CTOR_DTOR) (void *obj, size_t size, void *ctx);
typedef int  (*VCACHE_ALLOC_OBJECT)     (struct tagVCACHE *cache);
typedef int  (*VCACHE_FREE_OBJECT)      (struct tagVCACHE *cache, void *ptr);


typedef struct {
    size_t cached_object_count; /** number of cached objects */
    size_t allocated_count;	/** number of allocated object */
    size_t total_count;		/** currently the cache can hold a maximum of this number of objects */ 

} VCACHE_STATISTICS;

typedef int (*VCACHE_DTOR)  (struct tagVCACHE *cache);

typedef int (*VCACHE_STATS) (struct tagVCACHE *cache, VCACHE_STATISTICS *ptr);


/**
 * @brief Interface for object caching,
 *	- all elements are of the same size
 *	- interface for not strict cache , i.e. cache does not have an upper bound for number of elements
 *  - interface doe not specify a maximum element lifetime.
 */
typedef struct tagVCACHE {

    /* cache interface */
    VCACHE_DTOR		  clean;	/** destroy all cached objects, does not free any memory to the system */
    VCACHE_DTOR		  free;		/** cache destructor: destroy all cached object and free their memory */ 
    VCACHE_STATISTICS stats;	/** get cache statistics */

    /* interface - object access */
    VCACHE_ALLOC_OBJECT	 alloc_obj;	/** function pointer: allocate an cached object */	    	
    VCACHE_ALLOC_OBJECT	 free_obj;	/** function pointer: return object to cache */		
    VCACHE_FREE_OBJECT   destroy_obj;/** function pointer: destroy object and return memory */	

    /* object initialisation/destruction */
    VCACHE_OBJECT_CTOR_DTOR ctor;	/** function pointer:  constructor for cached object */ 
    VCACHE_OBJECT_CTOR_DTOR dtor;	/** function pointer:  destructor for cached object */
    void	       *ctor_dtor_ctx;  /** data context passed to object constructor destructor */
    
	VCONTEXT_ON_ERROR  on_error;	/** error callback */
	
	/* data */    
	VCACHE_POLICY  policy;
    size_t	       elmsize;		/** size of cached object */		
    size_t	       alignment;	/** alignment of cached object */
} VCACHE;

V_INLINE void VCACHE_init( VCACHE *cache, 
						   VCACHE_POLICY policy, 
						   size_t elmsize, 
						   size_t alignment, 
						   VCACHE_OBJECT_CTOR_DTOR ctor, 
						   VCACHE_OBJECT_CTOR_DTOR dtor, 
						   void *ctor_dtor_ctx,
			  VCONTEXT_ON_ERROR on_error)
{ 
    memset(cache, 0, sizeof(VCACHE) );

    cache->elmsize = elmsize;
    cache->alignment = alignment;
	cache->policy = policy;

    cache->ctor = ctor;
    cache->dtor = dtor;
    cache->ctor_dtor_ctx = ctor_dtor_ctx;
    cache->on_error = on_error;
}   

#define VCACHE_ALLOC(cache)	(cache)->alloc_obj( (cache) )

#define VCACHE_FREE(cache,obj)	(cache)->free_obj( (cache), (obj) )

#define VCACHE_DESTROY(cache, obj) (cache)->destroy_obj( (cache), (obj) )

#ifdef  __cplusplus
}
#endif

#endif
