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

#ifndef V_UTIL_UTIL_H_
#define V_UTIL_UTIL_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>
#include <string.h>
#include <ctype.h>

V_INLINE void VUTIL_swapbytes(unsigned char *src, unsigned char *dst, size_t size)
{
	unsigned char tmp;

	while(size>0) {
		tmp = *src;
		*src = *dst;
		*dst = tmp;

		++src;
		++dst;
		--size;
	}
}

/*
 * @brief returns 1 if num is a power of two.
 */
V_INLINE int VUTIL_is_power_of_two( V_UINT32 num )
{
	return (num & (num - 1)) == 0;
	/* num == 2 ^ n -1 | all ones
	 * num == 2 ^ n    | one on next position after the last one.
	 * & will return 0 (no overlap) if num is 2 ^ n
	 */
}	

/* 
 * @brief align integer (alignment must be power of 2)
 */
V_INLINE V_UINT32 VUTIL_align( V_UINT32 num, V_UINT32 align)
{
	/* assert( VUTIL_is_power_of_two( align ) ); */
	return (num  + align - 1 ) & (~(align - 1));
	/*
	 * & (x) (~(align -1 )) set align to the closers alignment (x % align == 0) (round down to closest alignment)
	 * num + align - 1      make sure that the argument crosses alignment border, 
	 */
}

/*
 * @brief align pointer integer (alignment must be power of 2)
 */
V_INLINE void * VUTIL_ptr_align( void *ptr, size_t align)
{
	/* assert( VUTIL_is_power_of_two( align ) ); */
	return (void *) ( (((V_POINTER_SIZE_T) ptr) & ~(align - 1)) + align );
}
/*
 * @brief return pointer to start of "page" ("page" size is power of two).
 */
V_INLINE void * VUTIL_ptr_page_start( void *ptr, size_t page_size)
{
	/* assert( VUTIL_is_power_of_two( align ) ); */
	return (void *) ( ((V_POINTER_SIZE_T) ptr) & ~ ((V_POINTER_SIZE_T) page_size - 1) );
	/*
	 * round argument pointer down to the closes pagesize (assuming pagesize is power of two)
	 */
}

/*
 * @brief checks if pointer is aligned to given power of two
 */
V_INLINE int VUTIL_ptr_is_aligned( void *ptr, size_t page_size)
{
	/* assert( VUTIL_is_power_of_two( align ) ); */
	return ( ((V_POINTER_SIZE_T) ptr) & ((V_POINTER_SIZE_T) page_size - 1) ) == 0;
}

/*
 * @brief check if argument pointer (ptr) is in memory range specified by [from ... to)
 */
V_INLINE int VUTIL_ptr_in_range(void *ptr, void *from , void *to )
{
	return (V_UINT8 *) ptr >= (V_UINT8 *) from && (V_UINT8 *) ptr < (V_UINT8 *) to;
}

V_INLINE char * VUTIL_strdup( VCONTEXT *ctx, const char *str)
{
	int len = strlen(str);
	char *ret;

	ret = (char *) V_MALLOC( ctx, len+1 );
	if (ret) {
		strcpy(ret, str);
	}
	return ret;
}

V_INLINE size_t VUTIL_round_to_power_of_n( size_t num )
{
	size_t n = 1;

	while( n < num ) {
		n <<= 1;
	}
	return n;
}

V_INLINE size_t VUTIL_log_base_2_of_n( size_t num )
{
	size_t ret = 0;

	while( num ) {
		num >>= 1;
		ret ++;
	}
	return ret;
}



V_INLINE char *VUTIL_skip_spaces( const char *p) {
	while( *p != '\0' && isspace(*p)) {
		++p;
	}
	return (char *) p;
}

V_INLINE char *VUTIL_skip_nspace( const char *p) {
	while( *p != '\0' && !isspace(*p)) {
		++p;
	}
	return (char *) p;
}

#ifdef  __cplusplus
}
#endif

#endif

