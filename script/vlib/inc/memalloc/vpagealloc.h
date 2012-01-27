/*
*     vpagealloc.h
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

#ifndef _VPAGEALLOC_H_
#define _VPAGEALLOC_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vutil.h>

/*
  Design:
    big blocks contains number of pages(elements) each element is of size 2^n
    
	The big blocks are aligned with page size, we can always get the start of a page
    by takign address & (BLOCKSIZE-1).

    (this is usefull for the nested allocator that takes the page and subdivides it further;
    nested allocator can be a fixed size beast; or one that uses a bitmap;)

    Start of page is the page header, contains pointer to start of big block;
    so that free will update counter of allocated pages;

    Problem:
    Loss of memory due to alignment; (if malloc returnes misaligned memory -
    or too much aligned (we still need a big block header).
    then we lose up to one PAGESIZE per big block;

    TODO: ?????
    don't keep pointers - instead keep offsets; in the future we will be able to put the whole
    thing into shared memory; if we want to; (i don't think we will do that)

    PROBLEM:
    servers;
	lets say that one allocator for one user context uses 64kb (very optimistic)
	then say 10000 users use 640 MB; (???) - possible also very big losses due to alignment.
	wow!
*/    

#include <util/vslist.h>

struct tagVPAGEALLOC_CHUNK;

typedef struct 
{
	VCONTEXT *ctx;
	VSLIST root;       /* linked list of big blocks */
	size_t page_size;  /* must be power of two */     
	size_t page_count; /* number of pages per chunk */ 

	struct tagVPAGEALLOC_CHUNK *current; /*next page allocation we first search this current node */
}
	VPAGEALLOC; 

V_EXPORT int VPAGEALLOC_init(VCONTEXT *ctx, VPAGEALLOC *alloc, size_t page_size, size_t pages_per_bigblock);

V_EXPORT void *VPAGEALLOC_malloc( VPAGEALLOC *alloc );

V_EXPORT void VPAGEALLOC_free(VPAGEALLOC *alloc, void *ptr);

#ifdef  __cplusplus
}
#endif

#endif

