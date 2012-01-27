/*
*     mpool.c
*
* 2006 Copyright (c) 
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
#include <unistd.h>
#include <errno.h>

#define CL_PAGE_SIZE_MIN 1024
#define CL_PAGE_SIZE_DEF 4096
#define MPOOL_PTR_ALIGN (sizeof(void*))


#include "mpool.h"

static int os_free_list_chunk_size = -1;

void allocatable_set_next (allocatable* item, allocatable* next_item)
{
  item->link.next = (linkable *)next_item;
}

allocatable* allocatable_get_next (allocatable* item)
{
  return (allocatable *) item->link.next;
}

int mpool_add (mpool* mpool, allocatable* new_item)
{
  /* Put to free_list_head */
  
  if (mpool->free_list_head)
    {
      allocatable_set_next (new_item, mpool->free_list_head);
   }
	
  mpool->free_list_head = new_item;
  mpool->free_list_size++;
  
  return 0;
}

allocatable* mpool_remove (mpool* mpool)
{
  allocatable *temp = mpool->free_list_head;

   if (temp)
   {
     mpool->free_list_head =  allocatable_get_next (mpool->free_list_head);
     mpool->free_list_size--;

     /* null the next pointer */
     allocatable_set_next (temp, 0);
   }
   return temp;
}

/****************************************************************************************
* Function name - mpool_take_obj
*
* Description - Takes an object from a memory pool
*
* Input -       *mpool - pointer to an initialized mpool
* Return Code/Output - On success - pointer to object, on error - NULL
****************************************************************************************/
allocatable* mpool_take_obj (mpool* mpool)
{
  if (! mpool)
    {
      fprintf (stderr, "%s - wrong input\n", __func__);
      return 0;
    }

  allocatable* obj = 0;
	
  if (! (obj = mpool_remove (mpool)))
    {
      /*
        Mpool is empty. Allocating from the OS. 
       */
      if (mpool_allocate (mpool, mpool->increase_step) == -1)
        {
          fprintf (stderr, "%s - mpool_allocate () - failed\n", __func__);
          return 0;
        }
      else
        {
          if (! (obj = mpool_remove (mpool)))
            {
              /* Rare scenario. */
              fprintf (stderr, "%s - mpool_remove () - failed, still no objects\n", __func__);
              return 0;
            }
        }
    }
  
  return obj;
}

/****************************************************************************************
* Function name - mpool_take_obj
*
* Description - Returns an object to a memory pool
*
* Input -       *mpool - pointer to an initialized mpool
*               *item - pointer to an object
* Return Code/Output - On success - 0, on error - (-1)
****************************************************************************************/
int mpool_return_obj (mpool* mpool, allocatable* item)
{
  if (! mpool || ! item)
    {
      fprintf (stderr, "%s - wrong input\n", __func__);
      return -1;
    }
	
  return mpool_add (mpool, item);
}

/****************************************************************************************
* Function name - mpool_init
*
* Description - Performs initialization of an allocated memory pool.
*
* Input -       *mpool - pointer to an allocated mpool
*               object_size -  required size of object
*               num_obj -  number of objects to be allocated for the memory pool
* Return Code/Output - On success - 0, on error - (-1)
****************************************************************************************/
int mpool_init (mpool* mpool, size_t object_size, int num_obj)
{
  if (! mpool || ! object_size || num_obj < 0)
   {
     fprintf (stderr, "%s - wrong input\n", __func__);
     return -1;
   }

  /* Figure out the page size */
  if (os_free_list_chunk_size < 0)
    {
      int pagesize = -1;
      if ((pagesize = getpagesize()) == -1)
        {
          fprintf (stderr, "%s - warning: getpagesize() failed with errno %d\n", 
                   __func__, errno);
        }
      
      if (pagesize < CL_PAGE_SIZE_MIN)
        {
          pagesize = CL_PAGE_SIZE_DEF;
        }

      os_free_list_chunk_size = (pagesize * 9)/10;
    }

  /* Alignment as proposed by Michael Moser */
  object_size = (object_size + MPOOL_PTR_ALIGN -1) & (~(MPOOL_PTR_ALIGN - 1));

  if (object_size > (size_t) os_free_list_chunk_size)
    {
      fprintf (stderr, "%s - error: too large object size\n", __func__);
      return -1;
    }

  mpool->increase_step = mpool->free_list_size = 0;
  mpool->obj_size = object_size;
  
  /* Preventing fragmentation */
  mpool->increase_step = os_free_list_chunk_size / mpool->obj_size;
	
  if (mpool_allocate (mpool, num_obj) == -1)
    {
      fprintf (stderr, "%s - mpool_allocate () failed\n", __func__);
      return -1;
    }

  return 0;
}

/****************************************************************************************
* Function name - mpool_free
*
* Description - Releases all allocated memory from a pool. The pool itself remains allocated.
*
* Input -       *mpool - pointer to an allocated mpool
* Return Code/Output - none
****************************************************************************************/
void mpool_free (mpool* mpool)
{
  if (mpool->obj_alloc_num != mpool->free_list_size)
    {
      fprintf (stderr, "%s - all objects must be returned\n", __func__);
      return;
    }

  if (! mpool->blocks_alloc_num)
    {
      fprintf (stderr, "%s - there are no allocated memory blocks\n", __func__);
      return;
    }

  allocatable** memblock_array = 0;

  if (! (memblock_array = 
         calloc (mpool->blocks_alloc_num, sizeof (allocatable*))))
    {
      fprintf (stderr, "%s - calloc () of blocks_array failed\n", __func__);
      return;
    }
	
  int memblock_array_index = 0;

  /*
    Iterate through all objects to fetch starts of memblocks and 
    write them to the memblock_array
  */
  allocatable* item = 0;
  for (item = mpool->free_list_head; item ; item = allocatable_get_next (item))
    {
      if (item->mem_block_start)
        {
          memblock_array[memblock_array_index++] = item;
        }
    }

  // now free () all the blocks
  while (--memblock_array_index >= 0)
    {
      free (memblock_array[memblock_array_index]);
    }

  memset (mpool, 0, sizeof (*mpool));
}

/****************************************************************************************
* Function name - mpool_size
*
* Description - Returns the number of the allocated objects
*
* Input -       *mpool - pointer to an allocated mpool
* Return Code/Output - On success - number of objects, on error - (-1)
****************************************************************************************/
int mpool_size (mpool* mpool)
{
  return mpool->free_list_size;
}

/****************************************************************************************
* Function name - mpool_allocate
*
* Description - Allocates for an initialized pool some additional number of objects 
*
* Input -       *mpool - pointer to an initialized mpool
*               num_obj -  number of objects to be added for a memory pool
* Return Code/Output - On success - 0, on error - (-1)
****************************************************************************************/
int mpool_allocate (mpool* mpool, size_t num_obj)
{
  if (! mpool || ! num_obj)
    {
      fprintf (stderr, "%s - wrong input.\n", __func__);
      return -1;
    }

  if (mpool->increase_step <= 0)
    {
      fprintf (stderr, "%s - mpool not initialized\n", __func__);
      return -1;
    }

  // number of allocations, each of about a PAGE_SIZE
  int num_alloc_step = num_obj / mpool->increase_step;
  
  // minimum 1 allocation step should be done
  num_alloc_step = num_alloc_step ? num_alloc_step : 1;
  
  // number of allocated by this function call objects
  int obj_allocated = 0;
  
  // u_char is to enable ANSI-C pointer arithmetics
  unsigned char* chunk;
  
  while (num_alloc_step--)
    {
      chunk = 0;

      /* mpool->obj_size is aligned at pool init stage */
      if (! (chunk =  calloc (mpool->increase_step, mpool->obj_size)))
        {
          fprintf (stderr, "%s - calloc () failed\n", __func__);
          break;
        }
      else
        {
          mpool->blocks_alloc_num++;
          
          // add to mpool successfully allocated mpool->increase_step number of objects
          //
          int i;
          
          for ( i = 0; i < mpool->increase_step; i++)
            {
              allocatable* item = (allocatable*)(chunk + i*mpool->obj_size);
              
              // The first block (i == 0) we mark as 1, which mean allowed to call free (), others -0
              //
              item->mem_block_start =  i ? 0 : 1;
              mpool_add (mpool, item);
            }
          
          obj_allocated += mpool->increase_step;
        }
    }
	
  if (!obj_allocated)
    {
      fprintf (stderr, "%s - failed to allocate objects\n", __func__);
      return -1;
    }
  else
    {
      mpool->obj_alloc_num += obj_allocated;
    }

  return 0;
}

/****************************************************************************************
* Function name - mpool_mem_release
*
* Description - Releases from mpool to OS a specified number of objects.
*
* Input -       *mpool - pointer to an initialized mpool
*               num_obj -  number of objects to be released from a memory pool
* Return Code/Output - On success - 0, on error - (-1)
****************************************************************************************/
int mpool_mem_release (mpool* mpool, size_t num_obj)
{
  if (! mpool || num_obj <= 0)
    {
      fprintf (stderr, "%s - wrong input\n", __func__);
      return -1;
    }

  for (; mpool->free_list_head && num_obj > 0; num_obj--)
    {
      allocatable* item_to_free = mpool_remove (mpool);
      free (item_to_free);
    }

  return 0;
}
