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

#ifndef _VBITVECTOR_H_
#define _VBITVECTOR_H_

#ifdef  __cplusplus
extern "C" {
#endif


#include <util/vbasedefs.h>
#include <stdlib.h>
#include <memory.h>

#define UINT_32_NUM_OF_BITS		(32)
#define UINT_32_LOG2			(5)


V_INLINE int VBITS_setbit(unsigned char  *bits, int index)
{
	V_UINT32 *vec;

	vec = (V_UINT32 *) bits + (index >> UINT_32_LOG2);
	*vec |= 1 << (index & (UINT_32_NUM_OF_BITS - 1));
	return 0;
}


V_INLINE int VBITS_clearbit(unsigned char  *bits, int index)
{
	V_UINT32 *vec;

	vec = (V_UINT32 *) bits + ( index >> UINT_32_LOG2);
	*vec &= ~( 1 << ( index & (UINT_32_NUM_OF_BITS - 1)) );
	return 0;
}


V_INLINE int VBITS_testbit(unsigned char *bits, int index)
{
	return ( *( (V_UINT32 *) bits + ( (index) >> UINT_32_LOG2)) & ( 1 << ( (index) & (UINT_32_NUM_OF_BITS - 1)) ) );
}

V_EXPORT size_t VBITS_countbits(const unsigned char *bm, size_t pos);

/**
 * @brief Implements a dynamic array where each element is a bit.
 * Memory allocated is word alligned, this way we can inspect adjacent bit values of this array by leaps of word size.
 */
typedef struct {
  VCONTEXT  *ctx;

  size_t    max_bit_index;
  size_t    size;
  V_UINT32 *buffer;

} VBITVECTOR;


/**
 * @brief returns number of bytes used by bit vector, given the number of bits used.
 */
V_INLINE V_UINT32 VBITVECTOR_getsize(int maxEntries)
{
   /* bits to bytes */
   V_UINT32 bytes = ((maxEntries ) >> 3) + 1;
   
   /* byte number - word aligned */
   return (bytes & ~3) + sizeof(V_UINT32); 
}


/**
 * @brief Object constructor; allocate a new bit vector
 *
 * The bit vector is dynamically allocated from VCONTEXT allocator interface
 * Memory allocated is V_UINT32 alligned, this way we can inspect adjacent bit values of this array by leaps of word size.
 *
 * @param ctx (in) allocator object
 * @param bitmap (out) object that is constructed
 * @param max_index (in) maximum number of bits kept in this container.
 */
V_INLINE int VBITVECTOR_init( VCONTEXT *ctx, VBITVECTOR *bitmap, size_t max_index)
{
	bitmap->max_bit_index = max_index;
	bitmap->size = VBITVECTOR_getsize(max_index);

	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	if (!ctx) {
		return -1;
	}

	bitmap->buffer = (V_UINT32 *) V_MALLOC(ctx, bitmap->size);
	if (!bitmap->buffer) {
		return -1;
	}

	bitmap->ctx = ctx;
	memset( bitmap->buffer, 0, bitmap->size );
	return 0;
}

V_INLINE int VBITVCECTOR_init_from_mem(VBITVECTOR *bitmap, size_t max_index, void *mem, size_t mem_size)
{
	bitmap->ctx = 0;
	bitmap->max_bit_index = max_index,
	bitmap->buffer = (V_UINT32 *) mem;
	bitmap->size = VBITVECTOR_getsize(max_index);

	if (bitmap->size > mem_size) {
		return -1;
	}

	if (!bitmap->buffer) {
		return -1;
	}

	return 0;
}

/** 
 *  @brief change the size of the bitvector
 *  @param bitmap (in out) the object
 *  @param max_index (in) new maximum number of bits
 */
V_INLINE int VBITVECTOR_resize( VBITVECTOR *bitmap, int max_index)
{
	V_UINT8 *ptr;
	V_UINT32 newSize;

	if (!bitmap->ctx) {
		return -1;
	}	
	
	newSize = VBITVECTOR_getsize(max_index);

	ptr = (V_UINT8 *) V_REALLOC( bitmap->ctx, bitmap->buffer, newSize);
	if (!ptr) {
		return -1;
	}	

	if (newSize > bitmap->size) {
		memset( ptr + bitmap->size,  0, newSize - bitmap->size);
	}

	bitmap->buffer = (V_UINT32 *) ptr;
	bitmap->size = newSize;
	bitmap->max_bit_index = max_index;
	return 0;
}


/**
 * @brief Object destructor; destroy bit vector object
 * Memory is released by stored VCONTEXT allocator.
 */
V_INLINE void VBITVECTOR_free( VBITVECTOR *bitmap)
{
	if (bitmap->ctx) {
		if (bitmap->buffer) {
			V_FREE( bitmap->ctx,  bitmap->buffer );
			bitmap->buffer = 0;
		}
	}
}


/**
 * @brief set a bit entry in bit vector (i.e. set bit entry to 1)
 * @param bitmap the bit vector object
 * @param index bit to set
 * If index is out of range then -1 is returned (you can disable this feature by defining VBITVECTOR_NO_RANGECHECK)
 */
V_INLINE int VBITVECTOR_setbit(VBITVECTOR *bitmap, int index)
{
#ifndef VVBITVECTOR_NO_RANGECHECK
	if ((V_UINT32) index >= bitmap->max_bit_index) {
		return -1;
	}
#endif

	VBITS_setbit( (V_UINT8 *) bitmap->buffer, index );
	return 0;
}

/**
 * @brief clear a bit entry in bit vector (i.e. set bit entry to 0)
 * @param bitmap the bit vector object
 * @param index bit to set
 * If index is out of range then -1 is returned (you can disable this feature by defining VBITVECTOR_NO_RANGECHECK)
 */
V_INLINE int VBITVECTOR_clearbit(VBITVECTOR *bitmap, int index)
{
#ifndef VVBITVECTOR_NO_RANGECHECK
	if ((V_UINT32) index >= bitmap->max_bit_index) {
		return -1;
	}
#endif
	VBITS_clearbit( (V_UINT8 *) bitmap->buffer, index );
	return 0;
}

/**
 * @brief test if bit entry in bit vector is set
 * @param bitmap the bit vector object
 * @param index bit to check
 * If index is out of range then -1 is returned (you can disable this feature by defining VBITVECTOR_NO_RANGECHECK)
 */
V_INLINE int VBITVECTOR_testbit(VBITVECTOR *bitmap, int index)
{
#ifndef V_NO_RANGECHECK
	if ((V_UINT32) index >= bitmap->max_bit_index) {
		return -1;
	}
#endif
	return VBITS_testbit( (V_UINT8 *) bitmap->buffer, index );;
}


V_INLINE V_UINT32 VBITVECTOR_get_word( VBITVECTOR *bitmap, int word_index)
{
#ifndef V_NO_RANGECHECK
	if ((V_UINT32) word_index >= (bitmap->size >> 2)) {
		return 0;
	}
#endif
	return (V_UINT32) bitmap->buffer + word_index;
}

V_INLINE void VBITVECTOR_set_word( VBITVECTOR *bitmap, int word_index, V_UINT32 val)
{
#ifndef V_NO_RANGECHECK
	if ((V_UINT32) word_index >= (bitmap->size >> 2)) {
		return;
	}
#endif
	bitmap->buffer[ word_index ] = val;
}


/**
 * @brief Macro, loop over each set bit (value 1) in bit vector
 * @param bitindex_var (type V_UINT32) current bit index for set bit. 
 * @param bitmap (type VBITVECTOR *) the object
 */
#define VBITVECTOR_FOREACH_SET_BIT(bitindex_var, bitmap) \
{ \
	V_UINT32 vbitvector_foreach_word_i##bitindex_var, vbitvector_foreach_word_n##bitindex_var = (bitmap)->size >> 2; \
	for( vbitvector_foreach_word_i##bitindex_var = 0; vbitvector_foreach_word_i##bitindex_var < vbitvector_foreach_word_n##bitindex_var; vbitvector_foreach_word_i##bitindex_var++) {\
		V_UINT32 vbitvector_foreach_word_cur##bitindex_var = (bitmap)->buffer[ vbitvector_foreach_word_i##bitindex_var ];\
		if (vbitvector_foreach_word_cur##bitindex_var) {\
			for((bitindex_var) = vbitvector_foreach_word_i##bitindex_var << 5;vbitvector_foreach_word_cur##bitindex_var;vbitvector_foreach_word_cur##bitindex_var >>= 1,(bitindex_var) += 1) {\
				if (vbitvector_foreach_word_cur##bitindex_var & 1) {


/**	
 * @brief Macro, loop over each cleared bit (value 0) in bit vector.
 * @param bitindex_var (type V_UINT32) current bit index for set bit. 
 * @param bitmap (type VBITVECTOR *) the object
 */
#define VBITVECTOR_FOREACH_ZERO_BIT(bitindex_var, bitmap) \
{ \
	V_UINT32 vbitvector_foreach_word_i##bitindex_var, vbitvector_max_bit_index##i = (bitmap)->max_bit_index , vbitvector_foreach_word_n##bitindex_var = (bitmap)->size >> 2; \
	for( vbitvector_foreach_word_i##bitindex_var = 0; vbitvector_foreach_word_i##bitindex_var < vbitvector_foreach_word_n##bitindex_var; vbitvector_foreach_word_i##bitindex_var++) {\
		V_UINT32 vbitvector_foreach_word_cur##bitindex_var = (bitmap)->buffer[ vbitvector_foreach_word_i##bitindex_var ];\
		if (vbitvector_foreach_word_cur##bitindex_var != 0xFFFFFFFF) {\
			for((bitindex_var) = vbitvector_foreach_word_i##bitindex_var << 5;(bitindex_var) < vbitvector_max_bit_index##i;vbitvector_foreach_word_cur##bitindex_var >>= 1,(bitindex_var) += 1) {\
				if ((vbitvector_foreach_word_cur##bitindex_var & 1) == 0) {


/**
 * @brief loop over each bit in bit vector
 * @param bitindex_var (type V_UINT32) current bit index for set bit. 
 * param  bitvalue (type V_UINT32,V_INT32) value of current bit
 * @param bitmap (type VBITVECTOR *) the object
 */
#define VBITVECTOR_FOREACH_BIT(bitindex_var, bitvalue, bitmap) \
{ \
	V_UINT32 vbitvector_foreach_word_i##bitindex_var, vbitvector_max_bit_index##i = (bitmap)->max_bit_index , vbitvector_foreach_word_n##bitindex_var = (bitmap)->size >> 2; \
	(bitindex_var) = 0; \
	for( vbitvector_foreach_word_i##bitindex_var = 0; vbitvector_foreach_word_i##bitindex_var < vbitvector_foreach_word_n##bitindex_var; vbitvector_foreach_word_i##bitindex_var++) {\
			V_UINT32 vbitvector_foreach_word_cur##bitindex_var = (bitmap)->buffer[ vbitvector_foreach_word_i##bitindex_var ]; { \
			for(;bitindex_var < vbitvector_max_bit_index##i;vbitvector_foreach_word_cur##bitindex_var >>= 1,bitindex_var += 1) {\
				(bitvalue) = (vbitvector_foreach_word_cur##bitindex_var & 1); { \




/**
 * @brief end of iteration over bit array.
 */
#define VBITVECTOR_FOREACH_END\
				}\
			}\
		}\
	}\
}


typedef int  (*VBITVECTOR_VISITOR_V) (VBITVECTOR *bitmap, V_UINT32 index, void *context);
typedef void (*VBITVECTOR_VISITOR)   (VBITVECTOR *bitmap, V_UINT32 index, void *context);

/**
 * @brief iterate over all bits in bit vector, callback is invoked for either each set or cleared bit.
 *
 * @param bitmap (in) the object.
 * @param val (in) if val is 0 then we callback is called for each 0 bit, if val is not zero then callback is called for each set bit.
 * @param visit (in) callback that is called to visit each set (or cleared) bit.
 * @param context (in) pointer that is passed as context on each invocation of callback.
 */
V_INLINE void VBITVECTOR_foreach( VBITVECTOR *bitmap, int val, VBITVECTOR_VISITOR visit, void *context)
{
	V_UINT32 i;

	if (val) {
		VBITVECTOR_FOREACH_SET_BIT(i, bitmap)
			visit(bitmap,i,context);
		VBITVECTOR_FOREACH_END
	} else {
		VBITVECTOR_FOREACH_ZERO_BIT(i, bitmap)
			visit(bitmap,i,context);
		VBITVECTOR_FOREACH_END
	}
}

/**
 * @brief find an entry in bit vector.
 * Invoke a callback on each entry in bit vector, if callback returns not zero for a given index, then the index is returned.
 *
 * @param bitmap (in) the object
 * @param bitval (in) if value is 0 then we callback is called for each 0 bit, if value is not zero then callback is called for each set (1) bit.
 * @param visit (in) callback that is called to visit each set (or cleared) bit.
 * @param context (in) pointer that is passed as context on each invocation of callback.
 * @param pos (out) returns position of bit where callback returned non zero value.
 * @returns first non zero value returned by visit callback
 */
V_INLINE int VBITVECTOR_findin( VBITVECTOR *bitmap, int bitval, VBITVECTOR_VISITOR_V visit, void *context, size_t *pos)
{
	size_t i;
	int ret;

	if (bitval) {
		VBITVECTOR_FOREACH_SET_BIT(i, bitmap)
			ret = visit(bitmap,i,context);
			if (ret) {
				if (pos) {
				  *pos = i;
				}
				return ret;
			}
		VBITVECTOR_FOREACH_END
	} else {
		VBITVECTOR_FOREACH_ZERO_BIT(i, bitmap)
			ret = visit(bitmap,i,context);
			if (ret) {
				if (pos) {
				  *pos = i;
				}
				return ret;
			}
		VBITVECTOR_FOREACH_END
	}
	if (pos) {
		*pos = (size_t) -1;
	}
	return -1;
}


#ifdef  __cplusplus
}
#endif

#endif

