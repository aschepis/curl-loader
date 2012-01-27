/*
*     heap.h
*
* 2006-2007 Copyright (C) 
* Robert Iakobashvili, <coroberti@gmail.com>
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

#ifndef HEAP_H
#define HEAP_H

#include "mpool.h"


/*
  hnode - is the housekeeping node of heap, its building block.
*/
typedef struct hnode
{
  /* Base for the "allocatable" property. */
  allocatable alloc;	
  
  /*
     The unique id of the node. When used in timer-queue, 
     we place here timer-id.
  */
  long node_id;

  /* 
     Pointer to the user-data context to keep with the node. 
     When used in timer-queue, we place here pointer to
     timer-node.
  */
  void* ctx;

} hnode;

/* 	
   Prototype of the function to be used to compare heap-kept objects
   for the sake of sorting in a heap. Sorting is necessary to implement 
   sorted queue container.
*/
typedef int (*heap_cmp_func) (hnode* const, hnode* const);

/* 	
   Prototype of the function to dump the nodes. Each type of nodes
   has its own context to dump.
*/
typedef void (*node_dump_func) (hnode* const);


typedef struct heap
{
  /* Array of pointers to the nodes. */
  hnode** heap;
	
  /* Maximum size of the heap. */
  size_t max_heap_size;
	
  /* Current size of the heap. */
  size_t curr_heap_size;
	
  /* 
     Really heap increase step.
  */
  size_t heap_increase_step;
	
  /* 
     Array of node ids to serve for mapping between node-id and place in heap,
     containing node with the id.
		
     When ids_arr[i] is -(-1), it is free and may be used for a newcoming node. 
     But, when ids_arr[i] >=0, it does contain some valid node-id corresponding
     to a valid node in the <heap> array.	
  */
  long* ids_arr;
	
  /* The last provided slot from <ids_arr> (node-id) */
  size_t ids_last;
	
  /* The lowest freed node-id */
  size_t ids_min_free;
	
  /* 
     Keeping <ids_min_free> in order to restore the value for a
     node to be re-scheduled.
  */
  size_t ids_min_free_restore;

  /* Comparator function for nodes. */
  heap_cmp_func fcomp;

  /* Dumper function for nodes. */
  node_dump_func fndump;

  /* Memory pool of hnodes */
  struct mpool* nodes_mpool;
	
} heap;


/****************************************************************************************
* Function name - node_reset
*
* Description - Zeros the fields of hnode, that are allowed to be zeroed. 
*               Attention!!! Use this function and not memset.
*
* Input -       *node - pointer to an hnode object
* Return Code/Output - none
****************************************************************************************/
void node_reset (hnode*const node);

/****************************************************************************************
* Function name - heap_init
*
* Description - Performs initialization of an allocated heap.
*
* Input -       *h - pointer to an allocated heap
*               initial_heap_size -  initial size to start
*               increase_step -  number of hnodes used to increase heap, when doing it
*               comparator -  user-function, that compares user-objects, kept by the heap
*               dumper -  user-function, that dumps user-objects, kept by the heap
*               nodes_prealloc -  number of hnodes to be pre-allocated at initialization
* Return Code/Output - On success - 0, on error -1
****************************************************************************************/
int heap_init (heap*const h,
               size_t initial_heap_size,
               size_t increase_step,
               heap_cmp_func comparator,
               node_dump_func dumper,
               size_t nodes_prealloc);


/****************************************************************************************
* Function name - heap_reset
*
* Description - De-allocates memory and inits to the initial values heap, 
*               but does not deallocate the heap itself.
*
* Input -       *h - pointer to an initialized heap
* Return Code/Output - none
****************************************************************************************/
void heap_reset (heap*const h);


/****************************************************************************************
* Function name - heap_dump
*
* Description -  Dumps heap fields. Uses user-provided function to dump user objects
*
* Input -        *h - pointer to an initialized heap
* Return Code/Output - none
****************************************************************************************/
void heap_dump (heap*const h);

/****************************************************************************************
* Function name - heap_prealloc
*
* Description - Pre-allocates a certain number of hnodes for a heap
*
* Input -       *h - pointer to an initialized heap
*                     nodes_prealloc -  number of hnodes to be pre-allocated at initialization
* Return Code/Output - On success - 0, on error -1
****************************************************************************************/
int heap_prealloc (heap*const h, size_t nodes_prealloc);

/****************************************************************************************
* Function name - heap_pop
*
* Description - Takes the root node out of the heap and restores heap structure
*
* Input -       *h - pointer to an initialized heap
* Return Code/Output - On success - pointer to hnode, on error - NULL
****************************************************************************************/
hnode* heap_pop (heap*const h, int keep_timer_id );


/****************************************************************************************
* Function name - heap_push
*
* Description -  Pushes a new hnode to the heap
*
* Input -       *h - pointer to an initialized heap
*               *node - pointer to an initialized heap
*               keep_node_id - flag, whether to respect the <node-id> from the node
*                              (support for periodical timer)
* Return Code/Output - On success - 0 or positive node-id, on error - (-1)
****************************************************************************************/
long heap_push (heap* const h, hnode* const node, int keep_node_id);

/****************************************************************************************
* Function name - heap_top_node
*
* Description -  Returns the topest node
*
* Input -       *h - pointer to an initialized heap
* Return Code/Output - On success -  pointer to the topest node, on error - NULL
****************************************************************************************/
hnode* heap_top_node (heap*const h);

/****************************************************************************************
* Function name - heap_remove_node
*
* Description -  Removes node from a certain slot and restores heap structure
*
* Input -       *h - pointer to an initialized heap
*               slot - index of the heap-array (slot), where to remove hnode
*               reserve_slot - true -means to reserve the slot
* Return Code/Output - On success -  pointer to the removed node, on error - NULL
****************************************************************************************/
hnode* heap_remove_node (heap*const h, const size_t slot, int reserve_slot);

/****************************************************************************************
* Function name - heap_empty
*
* Description -  Tests, whether a heap is empty
*
* Input -        *h - pointer to an initialized heap
* Return Code/Output - Positive number, if empty, zero- if non-empty
****************************************************************************************/
int heap_empty (heap*const h);

/****************************************************************************************
* Function name - heap_size
*
* Description -  Returns current size of a heap
*
* Input -        *h - pointer to an initialized heap
* Return Code/Output - On Success - zero or positive number, on error - (-1)
****************************************************************************************/
int heap_size (heap*const h);

void release_node_id (heap*const h, const size_t node_id);

#endif /* HEAP_H */
