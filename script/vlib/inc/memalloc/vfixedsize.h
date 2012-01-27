/*
*     vfixedsize.h
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

#ifndef _VFIXEDSIZE_H_
#define _VFIXEDSIZE_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vutil.h>
#include <util/varr.h>
#include <memalloc/vcache.h>

typedef struct {
	V_UINT8  *alloc_data;  /* current position of allocated blocks < eof_block */
	V_UINT8  *start_buffer;
	V_UINT8  *eof_buffer;
	size_t   num_free;
} 
	VFIXEDHEAP_CHUNK_INFO;


/** 
 *	@brief allocator for blocks of fixed size and alingment;
 */
typedef struct  {

	VCONTEXT	base_interface;				/** exported allocator interface */

	VCONTEXT	*ctx;						/** this allocator interface gets memory for big blocks from it */
	size_t		elmsize;					/** size of one element */
	size_t		alignment;					/** alignment per element */
	size_t		elmmaxcount_per_chunk;		/** maximum elements per big memory block */
	size_t		chunk_size;					/** size of one chunk */

	void		*freelist;					/** list of freed object */
	
	VFIXEDHEAP_CHUNK_INFO *current;			/** current big block that did the last allocation */

	VARR		info_sizes;					/** bookeeping: array of pointers to big blocks */
	
	size_t		num_free;					/** number of free elements */
}
	VFIXEDHEAP;

/** 
 * @brief iniitialise a fixed size memory allocator 
 */
V_EXPORT int VFIXEDHEAP_init(VCONTEXT *ctx,VFIXEDHEAP *alloc, size_t elmsize, size_t alignment, size_t elmperchunk);


/**
 * @brief return pointer to memory allocation interface
 */
V_INLINE VCONTEXT *VFIXEDHEAP_get_ctx(VFIXEDHEAP *alloc) 
{
	return &alloc->base_interface;
}

/**
 * @brief destroy the allocator and free all memory
 */
V_EXPORT void VFIXEDHEAP_free( VFIXEDHEAP *alloc );

/**
 * @brief check consistency of allocator
 */
V_EXPORT int VFIXEDHEAP_check( VFIXEDHEAP *alloc );



#ifdef  __cplusplus
}
#endif


#endif

