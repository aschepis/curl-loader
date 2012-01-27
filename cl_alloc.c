/*
*     cl_alloc.c
*
* 2007 Copyright (c) 
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

// must be the first include
#include "fdsetsize.h"

#include <stdlib.h>

#include "cl_alloc.h"

#define CL_PTR_ALIGN (sizeof(void*))

/*********************************************************************
* Function name - cl_calloc
*
* Description - Allocates aligned memory using calloc
*
* Return Code/Output - On success - pointer to memory, on error - NULL
**********************************************************************/
void* cl_calloc (size_t obj_num, size_t obj_size)
{
  if (!obj_num || !obj_size)
    {
      return NULL;
    }

  const size_t aligned_obj_size = 
    (obj_size + CL_PTR_ALIGN - 1) & (~(CL_PTR_ALIGN - 1));

  return calloc (obj_num, aligned_obj_size);
}
