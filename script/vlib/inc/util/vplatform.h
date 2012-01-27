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

#ifndef _VPLATFORM_H_X_
#define _VPLATFORM_H_X_

/* *** platform dependent definitions *** */

/** 
 * @brief Platform specific constants
 *
 *   V_PTRSIZE  size of pointer in current architecture.
 *
 *	 V_DEFAULT_STRUCT_ALIGNMENT default recomended structure member alignment/
 *
 *	 V_PAGE_SIZE size of one memory page on current platform (unit of paging)
 *
 *   V_L2_CACHE_LINE_SIZE size of one processor L2 cache line
 */


/* *** Windows platform with visual C *** */
#ifdef _MSC_VER

/* 
 * surpressed Visual C 6 warnings (error level mode 4)
 * 4154: nonstandard extension used : zero-sized array in struct/union
 * 4200: nonstandard extension used : zero-sized array in struct/union
 * 4127: conditional expression is constant
 */
#pragma warning( disable : 4514 4200 4127 )

#ifdef _WIN32

typedef V_UINT32			V_POINTER_SIZE_T;

#define V_DEFAULT_STRUCT_ALIGNMENT	(sizeof(void *))
#define V_PAGE_SIZE			(4096)
#define V_L2_CACHE_LINE_SIZE		(32)

#else
#error "Undefined platform"
#endif

#elif linux

/*#if i386 */

typedef V_UINT32			V_POINTER_SIZE_T;

#define V_DEFAULT_STRUCT_ALIGNMENT	(sizeof(void *))
#define V_PAGE_SIZE			(4096)
#define V_L2_CACHE_LINE_SIZE		(32)
/*
#else
#error "Undefined platform"
#endif
*/
#else
#error "Undefined platform"
#endif



#endif
