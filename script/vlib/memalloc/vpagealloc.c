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


#include <memalloc/vpagealloc.h>

typedef struct tagVPAGEALLOC_CHUNK
{
	VSLIST_entry  nextnode;    /* next node of bigblocks */
	VPAGEALLOC *alloc;   /* when chunk is freed - update allocator */

	size_t  num_free;
	size_t  page_size;
	size_t  pages_per_bigblock;

	void   *free_list;
	void   *alloc_data;  /* current position of allocated blocks < eof_block */
	void   *eof_block;
}
	VPAGEALLOC_CHUNK; 

typedef struct 
{
	VPAGEALLOC_CHUNK *node;     /* when freeing the page - update chunk header */
}
	VPAGEHEADER;


VPAGEALLOC_CHUNK *VPAGEALLOC_CHUNK_new(VPAGEALLOC *alloc)
{
    size_t bblock_size = alloc->page_size  * (alloc->page_count + 1);  
    void *mem;
    VPAGEALLOC_CHUNK *node;

    mem = V_MALLOC(alloc->ctx, bblock_size );
	if (!mem) {
		return 0;
    }
    node = (VPAGEALLOC_CHUNK *) mem;
  
    node->alloc_data =  VUTIL_ptr_align( mem, alloc->page_size );
    node->eof_block = (V_UINT8 *) node->alloc_data  + bblock_size;
	node->pages_per_bigblock = alloc->page_count;
    node->page_size = alloc->page_size;
  
    node->free_list  = 0;
    node->alloc = alloc;
    
    return 0;
}

void *VPAGEALLOC_CHUNK_next_free_page(VPAGEALLOC_CHUNK *cur)
{
    if (cur->free_list) {
	   /* get page from freelist and update the freelist*/

	  void *ret = cur->free_list;
	  cur->free_list = * ((void **) ret);

	  return ret;
    }

    /* can we get page from this bignode ?*/
    if (cur->alloc_data < cur->eof_block) {
      void *ret = cur->alloc_data;	
      cur->alloc_data = (V_UINT8 *) cur->alloc_data + cur->page_size;
      return ret;
    }

    return 0;
}    


/* ? allocate the first big block right now, or only on demand ?*/
V_EXPORT int VPAGEALLOC_init(VCONTEXT *ctx, VPAGEALLOC *alloc, size_t page_size, size_t pages_per_bigblock)
{
    alloc->ctx = ctx;
    alloc->current = 0;

    alloc->page_size = page_size;
    alloc->page_count = pages_per_bigblock;

    VSLIST_init( &alloc->root );
    
    return 0;

}


V_EXPORT void *VPAGEALLOC_malloc( VPAGEALLOC *alloc )
{
    void *ret;
    VSLIST_entry *elm;
    VPAGEALLOC_CHUNK *cur;

    if (!alloc->current) {
new_node:    
	/* allocate next bigblock - add to list and make it current */
        cur = alloc->current = VPAGEALLOC_CHUNK_new(alloc);
		VSLIST_push_back( & alloc->root, & cur->nextnode );
    } else {
		cur = alloc->current;
    }

    ret = VPAGEALLOC_CHUNK_next_free_page(cur);
    if (!ret) {
		VSLIST_FOREACH(elm, &alloc->root ) {
			if (elm != (VSLIST_entry *) cur) {
				ret = VPAGEALLOC_CHUNK_next_free_page((VPAGEALLOC_CHUNK *) elm);
				if (ret) {
					alloc->current = (VPAGEALLOC_CHUNK *) elm;
					return ret;
				}
			}
		}
	}
	
	if (ret) { 
		VPAGEHEADER *hdr = (VPAGEHEADER *) ret;
		hdr->node = cur;
		return hdr + 1;
	}
	/* no free memory in any chunk - get a new chunk; */
	goto new_node;

}

V_EXPORT void VPAGEALLOC_free(VPAGEALLOC *alloc, void *ptr)
{
    VPAGEHEADER *hdr = VUTIL_ptr_align( ptr, alloc->page_size );  
    VPAGEALLOC_CHUNK *node = hdr->node; 
    VSLIST_entry *elm, *prev;
 
    * ((void **) ptr) = node->free_list;
    node->free_list = ptr;
    
    node->num_free ++;
    if (node->num_free  ==  node->pages_per_bigblock) {
		VPAGEALLOC *alloc = node->alloc;
		
		prev = 0;
		VSLIST_FOREACH(elm, &alloc->root ) {
			if (elm == (VSLIST_entry *) node) {
				VSLIST_unlink_after( &alloc->root, prev );
				V_FREE( alloc->ctx, node );
				return;
			}
			prev = elm;
		}
		return;
	}
	/* error condition - the big block is not part of chain of big blocks */

}

