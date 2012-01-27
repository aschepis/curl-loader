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
#ifndef __VMCONST_H__
#define __VMCONST_H__

#include "vmvalue.h"

/* *** constant type definitions *** */

typedef struct tagVM_CONSTANT {
  VM_TYPE type;

  union {
	unsigned long long_value;
	double double_value;
	size_t offset_value;
  } val;

} VM_CONSTANT_VALUE;


#define VM_CONSTANT_long(value)  ((VM_CONSTANT_VALUE *) value)->val.long_value

#define VM_CONSTANT_double(value)  ((VM_CONSTANT_VALUE *) value)->val.double_value

#define VM_CONSTANT_string_offset(value)  ((VM_CONSTANT_VALUE *) value)->val.offset_value

const char * VM_SCALAR_get_const_string(struct tagVSCRIPTVM *vm, VM_CONSTANT_VALUE *val);

size_t VM_SCALAR_get_const_string_length(struct tagVSCRIPTVM *vm, VM_CONSTANT_VALUE *val);

#endif
