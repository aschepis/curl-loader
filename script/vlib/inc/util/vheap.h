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

#ifndef _V_HEAP_H_
#define _V_HEAP_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>


typedef int (*VHEAP_compare) (void *lhs, void *rhs, size_t elmsize);

/**
 * @brief Heap data structure; all elements in the heap must be of the same size.
 * A heap data structure is an ordered tree where the root node contains the smallest value.
 * The smallest element in the whole heap is alway the top element.
 * This heap is implemented by an array.
 */
typedef struct  
{
	VCONTEXT	   *ctx;
	VRESIZE_policy	sizepolicy;

	size_t			elmcount;
	size_t			elmmaxcount;
	size_t			elmsize;

	unsigned char	*buffer;

	VHEAP_compare	compare_func;

}  
	VHEAP;


/* 
 * @brief construct a heap object.
 * @param ctx (in) allocator interface. (if null we are using the default allocator)
 * @param heap (in) the object
 * @param elmcount (in) maximum number of elements kept in this heap.
 * @param elmsize (in)  size of one element kept in this heap.
 * @param grow_at (in) 
 * @param compare_func (in) compares two entries in the heap - used to establish heap property.
 */
V_INLINE int VHEAP_init( VCONTEXT *ctx, VHEAP *heap, size_t elmcount, size_t elmsize, VHEAP_compare compare_func)
{
	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	if (!ctx) {
		return -1;
	}

	heap->ctx = ctx;
	heap->sizepolicy = VRESIZE_init_multiply(2);;	
	
	heap->elmsize = elmsize;
	heap->elmcount = 0;
	heap->elmmaxcount = elmcount;
	heap->compare_func = compare_func;

	heap->buffer = (unsigned char *) V_MALLOC( ctx, elmcount * elmsize );

	if (!heap->buffer) {
		return -1;
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
V_INLINE void  VHEAP_set_sizepolicy( VHEAP *heap, VRESIZE_policy sizepolicy)
{
	heap->sizepolicy = sizepolicy;
}


/** 
 * @brief free all memory used by the heap (object destructor).
 * @heap heap (in) the object.
 */
V_INLINE void VHEAP_free( VHEAP *heap ) 
{
	if (heap->buffer) {
		V_FREE(heap->ctx, heap->buffer);
		heap->buffer = 0;
		heap->elmcount = 0;
		heap->elmmaxcount = 0;
		heap->elmsize = 0;
	}
}

V_INLINE size_t VHEAP_size( VHEAP *heap)
{
	return heap->elmcount;
}

V_INLINE size_t VHEAP_maxsize( VHEAP *heap)
{
	return heap->elmmaxcount;
}

V_INLINE size_t VHEAP_elmsize( VHEAP *heap)
{
	return heap->elmsize;
}

/** 
 * @brief returns pointer to the top element of the heap.
 * @param heap		(in) the object
 * @return pointer to the top element of the heap or 0 if heap is empty.
 */
V_INLINE unsigned char *VHEAP_top( VHEAP *heap)
{
	if (heap->elmcount == 0) {
		return 0;
	}
	return heap->buffer;
}


V_INLINE unsigned char *VHEAP_get_at( VHEAP *heap, int index)
{
	if (heap->elmcount == 0) {
		return 0;
	}
	if (index > (int) heap->elmcount || index < 0) {
		return 0;
	}

	return heap->buffer + index * heap->elmsize;
}


/** 
 * @brief remove the top element from the heap
 * @param heap		(in) the object
 */
V_EXPORT int VHEAP_pop( VHEAP *heap);

/** 
 * @brief insert a new element into the heap
 * @param heap		(in) the object
 * @param element	(in) data of object to be inserted into heap
 * @param elmsize	(in) size of data area for element pointer.
 */
V_EXPORT int VHEAP_push( VHEAP *heap, unsigned char *element, size_t elmsize );

/** 
 * @brief check consistency of heap object.
 */
V_EXPORT int VHEAP_check( VHEAP *heap );


typedef void (*VHEAP_VISITOR_V) (int index, void *elm, size_t elmsize, void *context);
typedef int  (*VHEAP_VISITOR)   (int index, void *elm, size_t elmsize, void *context);


V_INLINE void VHEAP_foreach( VHEAP *heap, VHEAP_VISITOR_V eval, void *context)
{
	size_t i;
	size_t sz = heap->elmcount;
	size_t elmsz = heap->elmsize;
	V_UINT8 *pos;

	for(pos = heap->buffer, i = 0; i < sz; i++, pos += elmsz) {
		eval( i, pos, elmsz, context ); 
	}
}

V_INLINE int VHEAP_findif( VHEAP *heap, VHEAP_VISITOR eval, void *context, V_INT32 *retval)
{
	size_t i;
	size_t sz = heap->elmcount;
	size_t elmsz = heap->elmsize;
	V_INT32 ret;
	V_UINT8 *pos;

	if (!retval) {
	  *retval = 0;
	}

	for(i = 0, pos = heap->buffer; i < sz; i++, pos += elmsz) {
		if ( (ret = eval( i, pos, elmsz, context )) != 0) {
			if (retval) {
				*retval = ret;
			}
			return i;
		}
	}
	return -1;
}


#ifdef  __cplusplus
}
#endif

#endif

