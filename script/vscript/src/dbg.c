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
#include "vm.h"

V_EXPORT void VSCRIPTVM_stack_trace( VSCRIPTVM *vm )
{
	size_t i,size; 
	VSCRIPT_STACK_FRAME *record;
	VSCRIPTVM_THREAD *thread = vm->thread;

	fprintf(stderr,"\n*** stack trace ***\n");
	
	size = VARR_size( &thread->stack_frames );

	if (size) {
		
		for(i = size - 1; ; i--) {
			record = (VSCRIPT_STACK_FRAME *) VARR_at( &thread->stack_frames, i );

			fprintf(stderr,"function_pc=%x ret_addr=%x stack_pointer=%x num_args=%d\n", 
						record->function_pc, 
						record->ret_addr,
						record->stack_pointer_on_call,
						record->num_args);
			if ( i == 0) {
				break;
			}
		};
	}
}


void dump_stack( VSCRIPTVM *vm) 
{
	size_t i;

	fprintf(stderr,"\n\t[stack] sp=%d st=%d\n",vm->thread->stack_pointer, vm->thread->stack_top);
	for(i = 0;i <= vm->thread->stack_top;i++) {
		void **elm = (void **) VARR_at( &vm->thread->stack_contents, i );

		if (*elm) {
			char tmp[100];
			const char *sval;
			int tmp_size;
			VM_BASE_TYPE  var_type;
			
			var_type = VM_TYPE_get_base_type(*elm);
			sval = VM_SCALAR_to_string(vm, *elm, tmp, &tmp_size);

			fprintf(stderr, "\t\t%x [%x]|%s\n", i, var_type, sval);
		}
	}

	fflush(stderr);
	
}

