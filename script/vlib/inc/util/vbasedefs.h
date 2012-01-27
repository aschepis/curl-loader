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

#ifndef __CDBASE_DEFS_H_
#define __CDBASE_DEFS_H_

#ifdef  __cplusplus
extern "C" {
#endif





/**
 * @mainpage
 *
 * This manual documents the VLIB C API.
 *
 * This library implements a series of commonly used data structures and algorithms.
 *
 * The following design decisions have been taken.
 *  
 * - provide more then one implementation for a datastructure, if there is a significant tradeoff of implementation between 
 *   size/performance. 
 * - give user lot of control over options of an algorithm, especially memory allocation behavior.
 * - iteration: provide both procedural, and functional styles of walking through a datastructures .
 *		procedural style: mostly FOREACH like macros
 *	    functional style: map/deleteif like function.
 * - customizable memory allocation policy. Memory allocation is implemented as an interface (VCONTEXT) and each 
 *   data structure that is allocating memory on its own has a pointer to a VCONTEXT interface.
 * - when a datastructure has to grow in size, like when you insert a new element into a dynamic array and it has to grow, 
 *   then provide different policy for specifing memory growth (addition with constant, multiply by constant, invoke callback), 
 *   policy is set per object.
 * - Don't overuse polymorphism, i.e. each datastructure has a similar interface, but there is no generalized interface for 
 *   say two implementations of a hash table.
 * - library use macros only when you must, use inline functions whenever possible.
 * - every data member of a structure is visible, i.e. no "private" data; 
 *   The main reason for this is to allow inline functions and macros.
 *   assume that the user cares enough and is smart enough not to change the state of those variables other than through the API.
 * - do avoid library namespace clashes (eveything is prefixed by V - hence the name VLIB).
 * - unit tests. provide many many unit tests.
 * 
 */


#include <stdlib.h>

#ifdef _WIN32
	#define V_INLINE __inline
	#define V_EXPORT __declspec(dllexport)
	#define V_STATIC_INLINE static __inline
#else
	#define V_INLINE static inline
	#define V_EXPORT extern
	#define V_STATIC_INLINE static inline
#endif

#define V_UNUSED(x) (void) (x);

#define V_TRUE		(1)
#define	V_FALSE		(0)

#define V_BOOLEAN   int
#define V_UINT8		unsigned char
#define V_INT8		char
#define V_INT16		short
#define V_UINT16	unsigned short
#define V_UINT32	unsigned int	
#define V_INT32		int

#ifdef WIN32
#define V_UINT64    unsigned __int64
#else
#define V_UINT64	unsigned long long
#endif

#define V_INT64		long long


#include <util/vplatform.h>
#include <util/verror.h>
#include <util/vcontext.h>


#define V_MEMBEROF(ptr,offset)			(((V_UINT8 *) ptr) - offset)

#define V_OFFSETOF(s,m)   (size_t)&(((s *)0)->m)


#define V_MEMBEROF_STC(ptr, sname, member)	( (sname *) ( ((V_UINT8 *) ptr) - V_OFFSETOF(sname,member) ) )

#define VPTR_ADD(ptr, offset, type)		( type ) ( ( (V_UINT8 *) (ptr) ) + (offset) )


typedef enum 
{
	VPOLICY_INCREMENT,
	VPOLICY_MULTIPLY_BY_FACTOR,
	VPOLICY_BY_CALLBACK,
}
	VPOLICYTYPE;

/** 
 * type of mechanism for resizing a contingent memory range.
 */
typedef enum {
	VRESIZE_fixed,
	VRESIZE_exact,
	VRESIZE_increment,
	VRESIZE_factor,
	VRESIZE_callback, 
} VRESIZE_type;

typedef size_t (*VRESIZE_POLICY_CALLBACK) (void *object, size_t requested);

/** 
 * @brief Policy for resizing a particular datastructure. This is a general mechanism.
 *
 * Resize policy - different policies for resizing a data structure, when it has to allocate more memory. 
 * Most data structures that own a contingent memory area (strings, dynamic arrays) 
 * are automatically expanded to accommodate new objects if filled beyond its current size.
 * 
 * There are multiple strategies that implement this increase in size; 
 *		VRESIZE_fixed       do not allow change in size (fixed memory area).
 *		VRESIZE_exact       allocate exactly as much as requested.
 *		VRESIZE_increment	Addition by constant, 
 *		VRESIZE_factor		multiplication with factor,
 *		VRESIZE_callback	invoke callback function that decides on this issue.
 */
typedef struct {
	VRESIZE_type  type;
	union {
		size_t int_arg;
		VRESIZE_POLICY_CALLBACK callback;
	} val;
} VRESIZE_policy;


/**
 * @brief returns a resize policy that will increments the requested size by constant (argument)
 */
V_INLINE VRESIZE_policy VRESIZE_init_increment(size_t increment)
{
	VRESIZE_policy ret;
	
	ret.type = VRESIZE_increment;
	ret.val.int_arg = increment;

	return ret;
}

/**
 * @brief returns a resize policy that will multiply the requested by a factor (argument)
 */
V_INLINE VRESIZE_policy VRESIZE_init_multiply(size_t factor)
{
	VRESIZE_policy ret;
	
	ret.type = VRESIZE_factor;
	ret.val.int_arg = factor;

	return ret;
}

/** 
 * @brief returns a resize policy that defer decision of size increas to callback function (argument)
 */
V_INLINE VRESIZE_policy VRESIZE_init_callback(VRESIZE_POLICY_CALLBACK callback)
{
	VRESIZE_policy ret;
	
	ret.type = VRESIZE_factor;
	ret.val.callback = callback;

	return ret;
}

/**
 * @brief returns a resize policy that does not allow change of size (fixed size policy)
 */
V_INLINE VRESIZE_policy VRESIZE_init_fixed(void)
{
	VRESIZE_policy ret;

	ret.type = VRESIZE_fixed;

	return ret;
}


V_INLINE size_t VRESIZE_request(VRESIZE_policy policy, void *object, size_t requested)
{
	size_t ret;

	switch(policy.type) {
		case VRESIZE_fixed:
			return 0;
		case VRESIZE_exact:
			return requested;
		case VRESIZE_increment:
			return requested + policy.val.int_arg;
		case VRESIZE_factor:
			return requested * policy.val.int_arg;
		case VRESIZE_callback:
			ret = policy.val.callback( object, requested );
			if (ret < requested) {
				return 0;
			}
			return ret;
	}
	return 0;
}

#ifdef  __cplusplus
}
#endif

#endif
	
