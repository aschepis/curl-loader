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

#ifndef __VARR_H__
#define __VARR_H__

#ifdef  __cplusplus
extern "C" {
#endif


#include <util/vbasedefs.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Dynamic array, all elements contained in this structure are of the same size.
 *
 * A dynamic array, growable array, resizable array, dynamic table,
 * or array list is a data structure, an array which is automatically expanded to 
 * accommodate new objects if filled beyond its current size. 
 * In most ways the dynamic array has performance similar to an array, with the addition 
 * of new operations to add and remove elements from the end:
 *
 *   - Getting or setting the value at a particular index (constant time)
 *   - Iterating over the elements in order (linear time, good cache performance)
 *   - Add an element to the end (constant amortized time)
 *
 * From http://en.wikipedia.org/wiki/Dynamic_array

 *
 *

 */
typedef struct  {

	VCONTEXT *ctx;
	VRESIZE_policy sizepolicy;

	size_t   elmmaxcount;
	size_t   elmsize;
	size_t   elmcount;

	V_UINT8  *buffer;

} VARR;


V_INLINE int VARR_init( VCONTEXT *ctx, VARR *arr, size_t elmsize, size_t numofelem)
{
	/*
	if (numofelem < 0) {
		return -1;
	}
	*/
	
	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	arr->ctx		= ctx;
	arr->sizepolicy	= VRESIZE_init_multiply(2);

	arr->elmsize	= elmsize;

	arr->elmcount	= 0;
	arr->elmmaxcount= numofelem;
	arr->buffer		= 0;


	if (numofelem) {
		if ((arr->buffer = (V_UINT8 *) V_MALLOC(ctx, elmsize * numofelem)) == 0) {
			return -1;
		}
	}
	return 0;
}

/**
 * @brief set resize policy
 
 * The resize policy determines how much space is allocated in advance, given
 * that the container has to be resized.

 * @brief arr the object
 * @brief sizepolicy amount 
 */
V_INLINE void  VARR_set_sizepolicy( VARR *arr, VRESIZE_policy sizepolicy)
{
	arr->sizepolicy = sizepolicy;
}

/**
 * @brief Allocates a dynamic array from already allocated memory area; 
 *
 * The memory is not owned by this object, meaning that this dynamic array cannot be resized 
 * (does not grow as maximum size is exceed); data are is not freed in object destructor.
 * 
 * @param arr		(out) the object
 * @param elmsize	(in)  size of one element
 * @param ptr		(in)  memory area that will contain
 * @param busize	(in)  size of memory area pointed to by ptr (size of Data area passed has to be greater or equal to the size of one element elmsize).
 * @return 0 if ok -1 on failure.
 */
V_INLINE  int VARR_init_fixed( VARR * arr, size_t elmsize, void *ptr, size_t bufsize)
{
	if (bufsize < elmsize) {
		return -1;
	}

	arr->ctx = 0;
	arr->elmsize = elmsize;

	arr->elmcount = 0;
	arr->elmmaxcount = bufsize / elmsize;
	arr->buffer = ptr;
	arr->sizepolicy.type = VRESIZE_fixed;

	return 0;
}

/** 
 * @brief Macro: Allocate dynamic array on stack
 *
 * Warning! this macro uses alloca standard library function - this is not always portable, 
 * and it bombs out when it causes stack overflow.
 *
 * @param arr		(out) (type VARR *) the object.
 * @param elmsize	(in)  (type size_t) size of one element
 * @param numofelem (in)  (type size_t) maximum number of elements.
 */
#define VARR_init_stack( arr, elmsize, numofelem ) \
do { \
	(arr)->ctx = 0; \
	(arr)->elmcount = 0; \
	(arr)->elmsize = (elmsize); \
	(arr)->elmmaxcount = (elmsize) * (numofelem); \
	(arr)->buffer = (char *) alloca( (str)->elmmaxcount ); \
	(arr)->sizepolicy.type = VRESIZE_fixed; \
} while(0);


/** 
 * @brief free all memory held by dynamic array (destructor).
 * @param buf (in out) the object
 */
V_INLINE void VARR_free( VARR *arr) 
{
	if (arr->ctx) {
		if (arr->buffer) {
			V_FREE( arr->ctx, arr->buffer );
			arr->buffer = 0;
		}
	}
}

V_INLINE  void VARR_reset( VARR * arr)
{
	arr->elmcount = 0;
}

/**
 * @brief returns number of objects that are currently held by this collection.
 * @param  arr (in) the object
 * @return 
 */
V_INLINE size_t VARR_size( VARR *arr) 
{
	return arr->elmcount;
}

/** 
 * @brief returns maximum number of elements that can currently be held by this collection.
 * @param  arr (in) the object
 * @return 
 */
V_INLINE size_t VARR_maxsize( VARR *arr )
{
	return arr->elmmaxcount;
}

/**
 * @brief change size of dynamic array (i.e. change size of memory allocated for array)
 * If requested size is smaller then the current size, then all skipping elements are discarded.
 *
 * @param arr (in out) the object
 * @param num_elem (in) change maximum number of element to this number. 
 */
V_INLINE int VARR_resize( VARR *arr, size_t num_elem) 
{
	V_UINT8 * ptr;

	if (num_elem > arr->elmmaxcount) {
		if (!arr->ctx) {
			return -1;
		}
		ptr = V_REALLOC( arr->ctx, arr->buffer, num_elem * arr->elmsize);
		if (!ptr) {
			return -1;
		}
		arr->buffer = ptr;
		arr->elmmaxcount = num_elem;
		if (num_elem < arr->elmcount) {
			arr->elmcount = num_elem;
		}
	} else {
		arr->elmcount = num_elem;
	}
	return 0;
}

V_INLINE V_UINT8 * VARR_at( VARR * arr, size_t index) 
{
	if (index >= arr->elmcount) {
		return 0;
	}

	return arr->buffer + (index * arr->elmsize);
}

V_INLINE int VARR_copy_at( VARR * arr, size_t index, void *elm, size_t elmsize) 
{
	if (index >= arr->elmcount) {
		return -1;
	}

	memcpy( elm, arr->buffer + (index * arr->elmsize), elmsize );
	return 0;
}

V_EXPORT int VARR_insert_at( VARR *arr, size_t index, void *elm, size_t elmsize);

V_EXPORT int VARR_set_at( VARR *arr, size_t index, void *elm, size_t elmsize);


V_INLINE int VARR_delete_at( VARR *arr, size_t index)
{
	if (index >= arr->elmcount) {
		return -1;
	}
	
	if (index < (arr->elmcount - 1)) {
		size_t elmsize = arr->elmsize;

		memmove( arr->buffer + index * elmsize,
				 arr->buffer + (index + 1) * elmsize, 
				 (arr->elmcount - index - 1) * elmsize);
		
	} 
	--arr->elmcount;
	return 0;
}


V_INLINE int VARR_push_back( VARR *arr, void *elm, size_t elmsize)
{	
	return VARR_insert_at(arr,arr->elmcount,elm, elmsize);		
}


V_INLINE int VARR_pop_back( VARR *arr, void *ptr, size_t elmsize )
{
	if (arr->elmcount <=0) {
		return -1;
	}

	if (ptr) {
		if (elmsize != arr->elmsize) {
			return -1;
		}

		memcpy( ptr, arr->buffer + (--arr->elmcount) * arr->elmsize, elmsize);
	}
	return 0;
}

V_INLINE int VARR_stack_top( VARR *arr, void *ptr, size_t elmsize )
{
	if (arr->elmcount <=0) {
		return -1;
	}

	if (ptr) {
		if (elmsize != arr->elmsize) {
			return -1;
		}

		memcpy( ptr, arr->buffer + arr->elmcount * arr->elmsize, elmsize);
	}
	return 0;
}

#if 0

#define VARR_FOREACH( loopvarname, array )
  
#define VARR_FOREACH_RANGE( loopvarname, from_idx, to_idx, array )

#define VARR_FOREACH_REVERSE( loopvarname, array )

#define VARR_FOREACH_REVERSE_RANGE( loopvarname, from_idx, to_idx, array )

#endif

/* Ups: can do this only with gcc typeof, very bad, not portable

#define VARR_FOREACH( loopvarname, array )	{\
	char *VARR_FOREACH_##loopvarname_eof = (array)->buffer + ((array)->elmcount * (array)->elmsize); \
	char *VARR_FOREACH_##loopvarname_ptr = (array)->buffer; \
	for(;VARR_FOREACH_##loopvarname_ptr < VARR_FOREACH_##loopvarname_eof; VARR_FOREACH_##loopvarname_ptr += (array)->elmsize) { \
		loopvarname = (cast-to-type-of-loopvarname) VARR_FOREACH_##loopvarname_ptr
*/


typedef void (*VARR_VISITOR_V) (int index, void *elm, size_t elmsize, void *context);
typedef int  (*VARR_VISITOR)   (int index, void *elm, size_t elmsize, void *context);

V_INLINE void VARR_foreach( VARR *arr, VARR_VISITOR_V eval, void *context)
{
	size_t i;
	size_t sz = arr->elmcount;
	size_t elmsz = arr->elmsize;
	V_UINT8 *pos;

	for(pos = arr->buffer, i = 0; i < sz; i++, pos += elmsz) {
		eval( i, pos, elmsz, context ); 
	}
}

V_INLINE int VARR_foreach_range( VARR *arr, VARR_VISITOR_V eval, void *context, int from_idx, int to_idx)
{
	size_t i;
	size_t sz = arr->elmcount;
	size_t elmsz = arr->elmsize;
	V_UINT8 *pos;

	if (!(from_idx > 0 && from_idx < to_idx && (size_t) to_idx <  sz)) {
		return -1;
	}

	for(pos = arr->buffer + (from_idx * elmsz), i = from_idx; i < (size_t) to_idx; i++, pos += elmsz) {
		eval( i, pos, elmsz, context ); 
	}

	return 0;
}



V_INLINE void VARR_foreach_reverse( VARR *arr, VARR_VISITOR_V eval, void *context)
{
	size_t i;
	size_t sz = arr->elmcount;
	size_t elmsz = arr->elmsize;
	V_UINT8 *pos;

	for(pos = arr->buffer + (elmsz * (sz-1)), i = sz-1; ; i--, pos -= elmsz) {
		eval( i, pos, elmsz, context ); 
		if (i == 0) {
			break;
		}
	}
}


V_INLINE int VARR_foreach_reverse_range( VARR *arr, VARR_VISITOR_V eval, void *context, int from_idx, int to_idx)
{
	int i;
	size_t sz = arr->elmcount;
	size_t elmsz = arr->elmsize;
	V_UINT8 *pos;

	if (!(from_idx > 0 && from_idx < to_idx && (size_t) to_idx < sz)) {
		return -1;
	}

	for(i = (to_idx - 1), pos = arr->buffer + (elmsz * i); ; i--, pos -= elmsz) {
		eval( i, pos, elmsz, context ); 
		if (i == from_idx) {
			break;
		}
	}

	return 0;
}


V_INLINE int VARR_findif( VARR *arr, VARR_VISITOR eval, void *context, V_INT32 *retval)
{
	size_t i;
	size_t sz = arr->elmcount;
	size_t elmsz = arr->elmsize;
	V_INT32 ret;
	V_UINT8 *pos;

	if (!retval) {
	  *retval = 0;
	}

	for(i = 0, pos = arr->buffer; i < sz; i++, pos += elmsz) {
		if ( (ret = eval( i, pos, elmsz, context )) != 0) {
			if (retval) {
				*retval = ret;
			}
			return i;
		}
	}
	return -1;
}

V_INLINE int VARR_findif_range( VARR *arr, VARR_VISITOR eval, void *context,  
							    int from_idx, int to_idx, V_INT32 *retval)
{
	int  i;
	size_t sz = arr->elmcount;
	size_t elmsz = arr->elmsize;
	V_INT32 ret;
	V_UINT8 *pos;

	if (!retval) {
	  *retval = 0;
	}

	if (!(from_idx > 0 && from_idx < to_idx && (size_t) to_idx < sz)) {
		return -1;
	}

	for(i = from_idx, pos = arr->buffer + (from_idx * elmsz); i < to_idx; i++, pos += elmsz) {
		if ((ret = eval( i, pos, elmsz, context )) != 0) {
			if (retval) {
				*retval = ret;
			}			
			return i;
		}
	}

	return -1;
}



V_INLINE int VARR_findif_reverse( VARR *arr, VARR_VISITOR eval, void *context, V_INT32 *retval)
{
	size_t i;
	size_t sz = arr->elmcount;
	size_t elmsz = arr->elmsize;
	V_INT32 ret;
	V_UINT8 *pos;

	if (!retval) {
	  *retval = 0;
	}

	for(i = sz-1, pos = arr->buffer + (elmsz * (sz-1)); ; i--, pos -= elmsz) {
		if ((ret = eval( i, pos, elmsz, context )) != 0) {
			if (retval) {
				*retval = ret;
			}			
			return i;
		}
		if (i == 0) {
			break;
		}
	}
	return -1;
}


V_INLINE int VARR_findif_reverse_range( VARR *arr, VARR_VISITOR eval, void *context, 
									  int from_idx, int to_idx, V_INT32 *retval)
{
	int i;
	size_t sz = arr->elmcount;
	size_t elmsz = arr->elmsize;
	V_INT32 ret;
	V_UINT8 *pos;

	if (!retval) {
	  *retval = 0;
	}

	if (!(from_idx > 0 && from_idx < to_idx && (size_t) to_idx < sz)) {
		return -1;
	}

	for(i = to_idx - 1, pos = arr->buffer + (elmsz * i); ; i--, pos -= elmsz) {
		if ((ret = eval( i, pos, elmsz, context )) != 0) {
			if (retval) {
				*retval = ret;
			}			
			return i;
		}
		if (i == from_idx) {
			break;
		}
	}

	return -1;
}


/** 
 * @brief iterate over all entries of the array and delete entries that match predicate from the array.
 * @param list (in) the object.
 * @param check_if (in) predicate function; the function returns 1 then inspected argument element will be deleted; if argument pointer is NULL then all entries will be deleted. 
 * @param context (in) data pointer passed to every invocation of check_if
 */
#if 0
V_INLINE void VARR_deleteif( VARR *list, VDLIST_VISITOR check_if, void *context)
{
    VDLIST_entry *cur,*next,*del;

    VDLIST_FOREACH_SAVE(cur,next,list) {
		if (!check_if || check_if( list, cur, context))  {
			del = VDLIST_unlink( list, cur );
			if (free_ctx) {
				free_ctx->free( V_MEMBEROF(del,offset_of_link) );
			}
		}
	}
}
#endif

/**
 * @}
 */

#ifdef  __cplusplus
}
#endif

#endif
