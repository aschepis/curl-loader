/*
 *     heap.c
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

// must be first include
#include "fdsetsize.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "heap.h"
#include "cl_alloc.h"

#define HEAP_PARENT(X) (X == 0 ? 0 : (((X) - 1) / 2))
#define HEAP_LCHILD(X) (((X)+(X))+1)

/* Increase size of heap array */
static int heap_increase (heap*const h);

/* Fetches a free node-id */
static long heap_get_node_id (heap*const h);

/* Puts node to the heap slot and updates id of the node. */
static void heap_put_node_to_slot (heap*const h, size_t slot, hnode*const node);
	


/* Heapifies to the up direction, so called parent path heapification */
static void filter_up (heap*const h, size_t index);

/* Heapifies to the down direction, so called children path heapification */
static void filter_down (heap*const h, size_t index);


/****************************************************************************************
* Function name - node_reset
*
* Description - Set to zero the fields, that allowed to be zeroed. Use this function 
*               and node memset.
*
* Input -       *node - pointer to an hnode object
*
* Return Code/Output - none
****************************************************************************************/
void node_reset (hnode*const node)
{
	if (!node)
	  return;

	node->node_id = 0;
	node->ctx = 0;
	node->alloc.link.next = 0;
}

/****************************************************************************************
* Function name - heap_init
*
* Description - Performs initialization of an allocated heap.
*
* Input -       *h - pointer to an allocated heap
*               initial_heap_size -  initial size to start
*               increase_step -  size to increase heap, when deciding to do so
*               comparator -  user-function, that compares user-objects, kept by the heap
*               dumper -  user-function, that dumps user-objects, kept by the heap
*               nodes_prealloc -  number of hnodes to be pre-allocated at initialization
*
* Return Code/Output - On success - 0, on error -1
****************************************************************************************/
int heap_init (heap*const h,
               size_t initial_heap_size,
               size_t increase_step,
               heap_cmp_func comparator,
               node_dump_func dumper,
               size_t nodes_prealloc)
{
  size_t i = 0;
	
  if (!h)
    {
      fprintf(stderr, "%s -error: wrong input\n", __func__);
      return -1;
    }
	
  memset ((void*)h, 0, sizeof (*h));
  
  if (! (h->heap = calloc (initial_heap_size, sizeof (hnode*))) )
    {
      fprintf(stderr, "%s - error: alloc heap failed\n", __func__);
      return -1;
    }
	
  /* Alloc array of node-ids */
  if (! (h->ids_arr = calloc (initial_heap_size, sizeof (long))) )
    {
      fprintf(stderr, "%s - error: alloc of nodes-ids array failed\n", __func__);
      return -1;
    }
	
  /* Invalidate node-ids array indexes */
  for (i = 0; i < initial_heap_size; i++)
    {
      h->ids_arr[i] = -1; /* non-valid id is -1 */
    }
	
  h->max_heap_size = initial_heap_size;
  h->heap_increase_step = increase_step;
	
  if (0 == comparator)
    {
      fprintf(stderr, "%s - error: comparator function should be provided.\n", __func__);
      return -1;      
    }
  else
    {
      h->fcomp = comparator;
    }

  h->fndump = dumper; /* If zero, we do not dump nodes. */

  if (!(h->nodes_mpool = cl_calloc (1, sizeof (mpool))))
    {
      fprintf(stderr, "%s - error: mpool allocation failed\n", __func__);
      return -1;
    }
  else
    {
      if (mpool_init (h->nodes_mpool, sizeof (hnode), nodes_prealloc) == -1)
        {
          fprintf(stderr, "%s - error: mpool_init () -  failed\n",  __func__);
          return -1;
        }
    }
  return 0;
}

/****************************************************************************************
* Function name - heap_reset
*
* Description - De-allocates memory and inits to the initial values heap, but does 
*               not deallocate the heap itself.
*
* Input -       *h - pointer to an initialized heap
* Return Code/Output - none
****************************************************************************************/
void heap_reset (heap*const h)
{
  if (h->heap)
    {
      free (h->heap);
    }
  if (h->ids_arr)
    {
      free (h->ids_arr);
    }
  if (h->nodes_mpool)
    {
      mpool_free (h->nodes_mpool);
      free (h->nodes_mpool);
    }
  memset (h, 0, sizeof (*h));
}

/****************************************************************************************
* Function name - heap_dump
*
* Description - Dumps heap fields. Uses user-provided function to dump user object content
*
* Input -       *h - pointer to an initialized heap
* Return Code/Output - none
****************************************************************************************/
void heap_dump (heap*const h)
{
  size_t j = 0;
	
  fprintf (stderr, "\tcurr_heap_size=%Zu\n", h->curr_heap_size);

  for (j = 0; j < h->curr_heap_size; j++)
    {
      fprintf (stderr, "[%Zu: ", j);
      if (h->fndump)
        {
          h->fndump (h->heap[j]);
        }
      else
        {
          fprintf (stderr, 
                   "%s - error: no dump function provided in mpool_init ().\n",
                   __func__);
        }
      fprintf (stderr, " ]\n");
    }	
}


/****************************************************************************************
* Function name - heap_prealloc
*
* Description - Pre-allocates a certain number of hnodes for a heap
*
* Input -       *h - pointer to an initialized heap
*               nodes_prealloc -  number of hnodes to be pre-allocated at initialization
* Return Code/Output - On success - 0, on error -1
****************************************************************************************/
int heap_prealloc (heap*const h, size_t nodes_prealloc)
{
  if (! h || ! nodes_prealloc)
    {
      fprintf(stderr, "%s - error: wrong input\n", __func__);
      return -1;
    }

  if (! h->nodes_mpool || ! h->nodes_mpool->increase_step)
    {
      fprintf(stderr, "%s - error: heap_init () should be called first\n", __func__);
      return -1;
    }
  
  if (mpool_allocate (h->nodes_mpool, nodes_prealloc) == -1)
    {
      fprintf(stderr, "%s - error: mpool_allocate() failed\n", __func__);
      return -1;
    }
	
  return 0;
}

/****************************************************************************************
* Function name - heap_pop
*
* Description - Takes the root node out of the heap and restores heap structure
*
* Input -       *h - pointer to an initialized heap
* Return Code/Output - On success - pointer to hnode, on error - NULL
****************************************************************************************/
hnode* heap_pop (heap*const h, int keep_timer_id)
{
  if (!h || h->curr_heap_size <= 0)
    {
      fprintf(stderr, "%s - error: wrong input\n", __func__);
      return 0;
    }
	
  return heap_remove_node (h, 0, keep_timer_id);
}

/****************************************************************************************
* Function name - heap_top_node
*
* Description -  Provides access to the topest node
*
* Input -        *h - pointer to an initialized heap
* Return Code/Output - On success -  pointer to the topest node, on error - NULL
****************************************************************************************/
hnode*  heap_top_node (heap*const h)
{
  if (!h || h->curr_heap_size <= 0)
    {
      fprintf(stderr, "%s - error: wrong input\n", __func__);
      return 0;
    }
  return h->heap[0];
}

/****************************************************************************************
* Function name - heap_empty
*
* Description -  Tests, whether a heap is empty
*
* Input -        *h - pointer to an initialized heap
* Return Code/Output - Positive number, if empty, zero- if non-empty
****************************************************************************************/
int heap_empty (heap*const h)
{
  return ! h->curr_heap_size;
}

/****************************************************************************************
* Function name - heap_increase
*
* Description - Increases size of the heap adding <heap_increase_step> hnodes
*
* Input -       *h - pointer to an initialized heap
* Return Code/Output - On success - 0, on error -1
****************************************************************************************/
int heap_increase (heap*const h)
{
  hnode** new_heap = 0, **old_heap = 0;
  long* new_ids = 0, *old_ids = 0;
  int new_size = 0, i = 0;

  if (!h || h->heap_increase_step <= 0)
    {
      fprintf(stderr, "%s - error: wrong input\n", __func__);
      return -1;
    }
	
  new_size = h->max_heap_size + h->heap_increase_step;
	
  /* Allocate new arrays for heap and ids */
  if ((new_heap = calloc (new_size, sizeof (hnode*))) == 0)
    {
      fprintf(stderr, "%s - error: alloc of the new heap array failed\n", __func__);
      return -1;
    }
	
  if ((new_ids = calloc (new_size, sizeof (long)) ) == 0)
    {
      fprintf(stderr, "%s - error: alloc of the new nodes-ids array failed\n", __func__);
      return -1;
    }
	
  /* mark all node-ids in the new_ids array as non-valid */
  for (i = 0; i < new_size; i++)
    {
      new_ids[i] = -1;
    }
	
  memcpy (new_heap, h->heap, sizeof (hnode*) * h->max_heap_size);
  memcpy (new_ids, h->ids_arr, sizeof (long) * h->max_heap_size);
	
  /* Keep the old heap and old nodes-ids*/
  old_heap = h->heap;
  old_ids = h->ids_arr;
	
  /* Switch the arrays and correct max_curr_heap_size */
  h->heap = new_heap;
  h->ids_arr = new_ids;
  h->max_heap_size = new_size;
	
  /* Release mem */
  free (old_heap);
  free (old_ids);
  
  return 0;
}

/****************************************************************************************
* Function name - heap_push
*
* Description - Pushes a new hnode to the heap
*
* Input -       *h - pointer to an initialized heap
*               *nd - pointer to node
*               keep_node_id - flag, whether to respect the <node-id> from the node
*                              (support for periodical timer)
* Return Code/Output - On success - 0 or positive node-id, on error - (-1)
****************************************************************************************/
long heap_push (heap* const h, hnode* const nd, int keep_node_id)
{
  long new_node_id = -1;
	
  if (!h || !nd)
    {
      fprintf(stderr, "%s - error: wrong input\n", __func__);
      return -1;	
    }
	
  if (h->curr_heap_size >= h->max_heap_size)
    {
      if (heap_increase (h) == -1)
        {
          fprintf(stderr, "%s - error: heap_increase() failed\n", __func__);
          return -1;
        }
    }
	
  if (keep_node_id)
    {
      /* Re-scheduled timers */
      new_node_id = nd->node_id;
    }
  else
    {
      /* Get free node-id */
      new_node_id = heap_get_node_id (h);
	
      /* 
         Set node-id to the hnode, it will be further passed from 
         the node to the relevant slot in <ids> array by 
         heap_put_node_to_slot ().
      */
      nd->node_id = new_node_id;
    }
	
  /* Place the node to the end of heap */
  heap_put_node_to_slot (h, h->curr_heap_size, nd);
  
  /* Restore the heap structure */
  filter_up (h, h->curr_heap_size);
  
  /* Increase current number of nodes in heap */ 
  h->curr_heap_size++;
  
  return new_node_id;
}

/****************************************************************************************
* Function name - heap_get_node_id
*
* Description -  Provides a "free" id
*
* Input -        *h - pointer to an initialized heap
* Return Code/Output - node-id
****************************************************************************************/
long heap_get_node_id (heap*const h)
{
  while (++h->ids_last < h->max_heap_size && h->ids_arr[h->ids_last] >= 0)
    ;

  if (h->ids_last == h->max_heap_size && h->ids_min_free < h->max_heap_size)
    {
      h->ids_last = h->ids_min_free;
      h->ids_min_free = h->max_heap_size;
    }
  
  return (long) h->ids_last;
}

/****************************************************************************************
* Function name - heap_put_node_to_slot
*
* Description -  Puts hnode to a certain slot position and updates the node id
*
* Input -        *h - pointer to an initialized heap
*                slot - index of the heap-array (slot) to be used for hnode
*                *nd - pointer to hnode
* Return Code/Output - none
****************************************************************************************/
void heap_put_node_to_slot (heap*const h, size_t slot, hnode*const nd)
{
  /* Insert node into the specified slot of the heap */
  h->heap[slot] = nd;
  
  /* Update slot in the parallel ids-array */
  h->ids_arr[nd->node_id] = slot;
}

/****************************************************************************************
* Function name - heap_remove_node
*
* Description -  Removes hnode from a certain slot position, reheapefies the heap 
* 		 and marks the slot in the ids array as available (-1)
*
* Input -       *h - pointer to an initialized heap
*               slot - index of the heap-array (slot), where to remove hnode
*               reserve_slot - true -means to reserve the slot
* Return Code/Output - On success - a valid hnode, on error - 0
****************************************************************************************/
hnode* heap_remove_node (heap*const h, const size_t slot, int reserve_slot)
{
  hnode* mved_end_node = 0;
  size_t parent_slot = 0;
  hnode* removed_node = h->heap[slot];
  size_t removed_node_id;

  if (!removed_node)
    {
      fprintf(stderr, "%s - error: null removed node.\n", __func__);
      return 0;
    }
	
  removed_node_id = removed_node->node_id;

  /* Decrement the heap size */
  h->curr_heap_size--;

  /* Reheapify only, if we're not deleting the last entry. */
  if (slot < h->curr_heap_size)
    {
      mved_end_node = h->heap[h->curr_heap_size];
      
      /* 
         Move the end node to the location being removed.  Update
         slot in the parallel <ids> array.
      */
      heap_put_node_to_slot (h, slot, mved_end_node);

      /* 	
         If the mved_end_node "node-value" < than the value its 
         parent, we move it up the heap.
      */
      parent_slot = HEAP_PARENT (slot);
		
      if ((*h->fcomp) (mved_end_node, h->heap[parent_slot])) // <
        {
          filter_up (h, slot);
        }
      else
        {
          filter_down (h, slot);
        }
    }
	
  /* Mark the node-id entry as free. */
  if (! reserve_slot)
    {
      release_node_id (h, removed_node_id);
    }

  return removed_node;
}


void release_node_id (heap*const h, const size_t node_id)
{
  h->ids_arr [node_id] = -1;
  
  if (node_id < h->ids_min_free  &&  node_id <= h->ids_last)
    {
      h->ids_min_free_restore = h->ids_min_free;
      h->ids_min_free = node_id;
    }
}

/****************************************************************************************
* Function name -  filter_up
*
* Description -  Heap structure restoration to the up direction
*
* Input -       *h - pointer to an initialized heap
*               index - position, from which to start
* Return Code/Output - none
****************************************************************************************/
void filter_up (heap*const h, size_t index)
{
  int curr_pos = index;
  int parent_pos = HEAP_PARENT(index);
  hnode* target = h->heap [index];

  /* Traverse path of parents up to the root */
  while (curr_pos > 0)
    {
      /* Compare target and parent value */
      if ((*h->fcomp) (h->heap[parent_pos], target)) /* less   < */
        {
          break;
        }
      else
        {
          /* Move data from parent position to current position.*/
          heap_put_node_to_slot (h, curr_pos, h->heap[parent_pos]);
			
          /* Update current position pointing to parent */
          curr_pos = parent_pos;
			
          /* Next parent */
          parent_pos = (curr_pos - 1)/2;
        }
    }
	
  heap_put_node_to_slot (h, curr_pos, target);
}

/****************************************************************************************
* Function name -  filter_down
*
* Description -  Heap structure restoration to the down direction
*
* Input -       *h - pointer to an initialized heap
*               index - position, from which to start
* Return Code/Output - none
****************************************************************************************/
void filter_down (heap*const h, size_t index)
{
  void* target = h->heap [index];
  size_t curr_pos = index;

  /*
    Compute the left child index and go down along path of children,
     stopping at the end of list, or when find a place for target 
  */
  size_t child_pos = HEAP_LCHILD(index);

  while (child_pos < h->curr_heap_size)
    {
      /* 
         Index of the right child is child_pos+1. Compare the two childs and 
         change child_pos, if comparison is true.
      */
      if ((child_pos+1 < h->curr_heap_size) &&
          (*h->fcomp) (h->heap[child_pos+1], h->heap[child_pos])) /* less  < */
        {
          child_pos = child_pos + 1;
        }

      /* Compare selected child to target */
      if ((*h->fcomp) (target, h->heap[child_pos])) /* less < */
        {
          /* Target belongs at curr_pos */
          break;
        }
      else
        {
          /* 
             Move selected child to the parent position. 
             The position of the selected child starts to be 
             empty and available. 
          */
          heap_put_node_to_slot (h, curr_pos, h->heap[child_pos]);
          
          /* Update index and continue */
          curr_pos = child_pos;
          child_pos = HEAP_LCHILD(curr_pos);
        }
    }
	
  /* Place target to the available vacated position */
  heap_put_node_to_slot (h, curr_pos, target);
}

/****************************************************************************************
* Function name - heap_size
*
* Description -  Returns current size of heap
*
* Input -        *h - pointer to an initialized heap
* Return Code/Output - On Success - zero or positive number, on error - (-1)
****************************************************************************************/
int heap_size (heap*const h)
{
  if (!h)
    return -1;
	
  return h->curr_heap_size;
}
