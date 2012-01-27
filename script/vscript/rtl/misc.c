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
#include <vscript.h>
#include "rtlapi.h"
#include "vm.h"

 

int std_strace(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	VSCRIPTVM_stack_trace(ctx);

	V_UNUSED(method_id);

	params->retval = VM_VALUE_LONG_return(ctx, 1);
		
	return 0;
}

int std_trace(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	VSCRIPTVM *vm = (VSCRIPTVM *) ctx;
	
	V_UNUSED(method_id);

	params->retval = VM_VALUE_LONG_return(ctx, vm->trace_flag);
	
	vm->trace_flag = 1;
		
	return 0;
}

int std_exit(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	VSCRIPTVM *vm = (VSCRIPTVM *) ctx;

	long val = VM_SCALAR_to_long(ctx, params->param[0]);

	V_UNUSED(method_id);

	vm->exit_status = val;
	vm->exit_status_set = 1;

	return 0;
}
