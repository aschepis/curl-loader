/*
*     mpool.h
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

#ifndef MPOOL_H
#define MPOOL_H

/*
   Object linkable supplies "linkable" property, when
   inherited.
*/
typedef struct linkable
{
  struct linkable* next;
} linkable;

/*
  Object allocatable supplies "allocatable" property, when
  inherited.
*/
typedef struct allocatable
{
  struct linkable link;

  /* 
   * Don't null the structure as a whole. The field to be managed by
     an allocator and is important for its internal housekeeping 
  */
  int mem_block_start;
} allocatable;

/*
  Memory pool. 
  Attention: Non-thread safe, but you may add it.
*/
typedef struct mpool
{
  /* Freelist free_list_head */
  allocatable* free_list_head;
	
  /* Freelist size. */
  int free_list_size;

  /* Number of objects for each allocation */
  int increase_step;

  /* Number of allocated memory blocks */
  int blocks_alloc_num;
	
  /* Object size */
  int obj_size;
	
  /* Number of allocated objects */
  int obj_alloc_num;
} mpool;

/****************************************************************************************
* Function name - mpool_init
*
* Description - Performs initialization of an allocated memory pool.
*
* Input -       *mpool - pointer to an allocated mpool
*               object_size -  required size of object
*               num_obj -  number of objects to be allocated for the memory pool
*
* Return Code/Output - On success - 0, on error - (-1)
****************************************************************************************/
int mpool_init (mpool* mpool, size_t object_size, int num_obj);

/****************************************************************************************
* Function name - mpool_free
*
* Description - Releases all allocated memory from a pool. The pool itself remains allocated.
*
* Input -       *mpool - pointer to an allocated mpool
*
* Return Code/Output - none
****************************************************************************************/
void mpool_free (mpool* mpool);

/****************************************************************************************
* Function name - mpool_size
*
* Description -  Returns the number of the allocated objects
*
* Input -       *mpool - pointer to an allocated mpool
* Return Code/Output - On success - number of objects, on error - (-1)
****************************************************************************************/
int mpool_size (mpool* mpool);

/****************************************************************************************
* Function name - mpool_allocate
*
* Description - Allocates for an initialized pool some additional number of objects.
*               The objects will be of the same size as passed at the pool initialization.
*
* Input -       *mpool - pointer to an initialized mpool
*               num_obj -  number of objects to be added to a memory pool
* Return Code/Output - On success - 0, on error - (-1)
****************************************************************************************/
int mpool_allocate (mpool* mpool, size_t num_obj);

/****************************************************************************************
* Function name - mpool_mem_release
*
* Description - Releases from mpool to OS a specified number of objects. 
*
* Input -       *mpool - pointer to an initialized mpool
*               num_obj -  number of objects to be released from a memory pool
* Return Code/Output - On success - 0, on error - (-1)
****************************************************************************************/
int mpool_mem_release (mpool* mpool, size_t num_obj);

/****************************************************************************************
* Function name - mpool_take_obj
*
* Description - Takes an object from a memory pool
*
* Input -       *mpool - pointer to an initialized mpool
* Return Code/Output - On success - pointer to object, on error - NULL
****************************************************************************************/
struct allocatable* mpool_take_obj (mpool* mpool);

/****************************************************************************************
* Function name - mpool_take_obj
*
* Description - Returns an object to a memory pool
*
* Input -       *mpool - pointer to an initialized mpool
*               *item - pointer to an object
* Return Code/Output - On success - 0, on error - (-1)
****************************************************************************************/
int mpool_return_obj (mpool* mpool, allocatable* new_item);



#endif /* MPOOL_H */
