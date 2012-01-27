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
#ifndef _VM_H_
#define _VM_H_

#include "asm.h"
#include "exe.h"
#include "xlib.h"

#include <vmvalue.h>
#include <wrapper/vwrapper.h>



#define VM_MAX_INSTRUCTION_SIZE 10

#define MAX_XMETHOD_PARAMS  64


typedef enum {
  VSCRIPTVM_STATUS_OK,
  VSCRIPTVM_STATUS_INTERNAL_ERROR,
  VSCRIPTVM_STATUS_NO_INPUT,
  VSCRIPTVM_STATUS_INVALID_EXE_FILE,
  VSCRIPTVM_STATUS_CHECKSUM_ERROR,

} VSCRIPTVM_STATUS;



typedef struct tagVSCRIPT_STACK_FRAME {
  V_UINT32 function_pc;  // start of PC for this function 
  V_UINT32 ret_addr;	 // return address - back to caller

  V_UINT32 stack_pointer_on_call; // when call is made, what is stack pointer
  V_UINT32 stack_top_on_call; // when call is made, what is stack pointer

  V_UINT32 num_args;		  // number of arguments of this call

}
  VSCRIPT_STACK_FRAME;

typedef struct tagVSCRIPTVM_THREAD {

	/* *** function activation records *** */
	VARR  stack_frames;

	/* *** stack variables and return values *** */
	VARR  stack_contents;

	/* *** offset of current stack frame *** */	
	size_t stack_pointer;

	/* *** highest location on stack *** */
	size_t stack_top;

	/* *** xmethod parameters *** */
	int						method_id;
	VSCRIPT_XMETHOD_FRAME	params;
	VM_OBJ_HEADER			*param_data[ MAX_XMETHOD_PARAMS ];


}  
	VSCRIPTVM_THREAD;

typedef struct tagVSCRIPTVM {
	

	/* *** memory allocation go here *** */
	VCONTEXT *ctx;

	/* *** current exe file *** */ 
	VOS_MAPFILE mapfile; // exe file mapped into memory

	/* *** code section *** */
	V_UINT8 *code_base; // start of code section
	V_UINT8 *code_eof;  // last instruction

	/* *** program counter, stored between invocations of vm *** */
	V_UINT32 pc;

	/* *** runtime state *** */
	VSCRIPTVM_THREAD *thread;

	/* *** constant area *** */
	struct tagVM_CONSTANT *cvalue;
	size_t cvalue_length;

	unsigned char *string_area;
	size_t string_area_length;

	/* *** globals *** */
	VARR globals;

	/* *** range of global parameters *** */
	V_UINT16 min_constant;
	V_UINT16 min_global;

	/* ** extension methods *** */
	XMETHODLIB *xlib;

	/* ** exit status ** */
	int exit_status;

	/* ** flags ** */
	int trace_flag : 1;
	int close_map_file : 1;
	int exit_status_set : 1;


} 
	VSCRIPTVM;


VSCRIPTVM *VSCRIPTVM_init(VCONTEXT * mctx, XMETHODLIB *xlib );
VSCRIPTVM_STATUS VSCRIPTVM_open(VSCRIPTVM *ctx, const char *exe_file, int trace);
VSCRIPTVM_STATUS VSCRIPTVM_open_from_image(VSCRIPTVM *ctx, VOS_MAPFILE map_file, int trace);
void VSCRIPTVM_close(VSCRIPTVM *vm);
int VSCRIPTVM_run(VSCRIPTVM *vm);

V_EXPORT void VSCRIPTVM_stack_trace( VSCRIPTVM *vm );

int ASM_INSTRUCTION_to_bytecode(struct tagASM_INSTRUCTION *instr, void *buf, size_t bufsize, 
								size_t min_global, size_t max_globals);
int ASM_INSTRUCTION_to_bytecode_size(ASM_TYPE opcode);


#endif
