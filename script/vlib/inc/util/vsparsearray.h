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

#ifndef _VSPARSEARRAY_H_
#define _VSPARSEARRAY_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>
#include <util/vbitvector.h>
#include <string.h>

typedef struct {	

  size_t elmcount;
  size_t elmmaxcount;
#if 0
  size_t maxindex;
  size_t minindex;
#endif

  V_UINT8 *data;
  V_UINT8 bitmask[];
	
} VSPARSEARRAYNODE;

/*
 @brief sparse table  
 
 The following explanation is taken from here: http://code.google.com/p/google-sparsehash/

 The idea is that a table with (logically) t buckets is divided
 into t/M *groups* of M buckets each.  (M is a constant set in
 GROUP_SIZE for efficiency.)  Each group is stored sparsely.
 Thus, inserting into the table causes some array to grow, which is
 slow but still constant time.  Lookup involves doing a
 logical-position-to-sparse-position lookup, which is also slow but
 constant time.  The larger M is, the slower these operations are
 but the less overhead (slightly).

 To store the sparse array, we store a bitmap B, where B[i] = 1 iff
 bucket i is non-empty.  Then to look up bucket i we really look up
 array[# of 1s before i in B].  This is constant time for fixed M.

 Terminology: the position of an item in the overall table (from
 1 .. t) is called its "location."  The logical position in a group
 (from 1 .. M ) is called its "position."  The actual location in
 the array (from 1 .. # of non-empty buckets in the group) is
 called its "offset."
*/
typedef struct {

	VCONTEXT *ctx;				/** allocation interface */
	VRESIZE_policy sizepolicy;  /** resize policy of one node */
	
	size_t elmsize;				/** size of one element */
	size_t elmcount;			/** current number of elements */
	size_t elmmaxcount;			/** maximum number of elements */ 
	size_t elmmaxcount_node;	/** maximum number of elements per node  (less than elmmaxcount)*/
	size_t node_count;			/** number of nodes (elmmmaxcount / elmmaxcount_node) */
#ifndef VSPARSEARRAY_NOT_POWER_OF_TWO_SIZED_NODES
	size_t elmmaxcount_node_log2;
#endif

	VSPARSEARRAYNODE **node;	/** array of nodes (groups) */

	VBITVECTOR			imp;

} VSPARSEARRAY;

typedef int  (*VSPARSEARRAY_VISITOR_V) (VSPARSEARRAY *arr, V_UINT32 index, void *entry, void *context);
typedef void (*VSPARSEARRAY_VISITOR)   (VSPARSEARRAY *arr, V_UINT32 index, void *entry, void *context);


V_EXPORT int  VSPARSEARRAY_init(VCONTEXT *ctx, VSPARSEARRAY *arr, size_t elmsize, size_t elmmaxcount, size_t elmpernode);

V_EXPORT void VSPARSEARRAY_free( VSPARSEARRAY *arr );

V_EXPORT int  VSPARSEARRAY_isset( VSPARSEARRAY *arr, size_t idx );

V_EXPORT int  VSPARSEARRAY_set(VSPARSEARRAY *arr, size_t idx, void *data, size_t elmsize);

V_EXPORT int  VSPARSEARRAY_get(VSPARSEARRAY *arr, size_t idx, void **data, size_t *elmsize);

V_EXPORT int  VSPARSEARRAY_unset(VSPARSEARRAY *arr, size_t idx);

V_EXPORT int  VSPARSEARRAY_check( VSPARSEARRAY *arr);

/**
 * @brief Macro, loop over each element in sparse array
 * @param index (type V_UINT32) current index 
 * @param entryptr (type V_UINT8 *) pointer to data of current entry
 * @param bitmap (type VSPARSEARRAY *) the object
 */
#define VSPARSEARRAY_FOREACH( index, entryptr, arr ) \
{ \
	size_t vsparsearray_i##index; \
	size_t vsparsearray_offset_i##index, vsparsearray_bitset_i##index; \
	size_t vsparsearray_datapos##index; \
	\
	for( vsparsearray_offset_i##index = vsparsearray_i##index = 0; \
		 vsparsearray_i##index < (arr)->node_count; \
		 vsparsearray_i##index ++ ) { \
	\
		VSPARSEARRAYNODE *vsparsearray_node##index = (arr)->node[ vsparsearray_i##index ]; \
		if (vsparsearray_node##index) { \
			 vsparsearray_datapos##index = 0; \
			 (arr)->imp.buffer = (V_UINT32 *) vsparsearray_node##index -> bitmask; \
			 vsparsearray_offset_i##index = (arr)->elmmaxcount_node * vsparsearray_i##index; \
			 VBITVECTOR_FOREACH_SET_BIT( vsparsearray_bitset_i##index, & (arr)->imp ) \
				 (entryptr) =  VPTR_ADD( vsparsearray_node##index -> data , (vsparsearray_datapos##index ++) * (arr)->elmsize, V_UINT8 *); \
				 (index) = vsparsearray_offset_i##index + vsparsearray_bitset_i##index;

#define VSPARSEARRAY_FOREACH_END \
			VBITVECTOR_FOREACH_END \
		} \
	} \
}

/**
 * @brief iterate over all entries of sparse table, callback is invoked for each element.
 *
 * @param arr (in) the object.
 * @param visit (in) callback that is called to visit each set (or cleared) bit.
 * @param context (in) pointer that is passed as context on each invocation of callback.
 */
V_INLINE void VSPARSEARRAY_foreach( VSPARSEARRAY *arr, VSPARSEARRAY_VISITOR visit, void *context)
{
	V_UINT32 i;
	V_UINT8 *entry;
	
	VSPARSEARRAY_FOREACH(i, entry, arr)
		visit(arr, i, entry, context);
	VSPARSEARRAY_FOREACH_END
}

#ifdef  __cplusplus
}
#endif

#endif
