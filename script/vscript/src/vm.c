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
#include <util/varr.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <vscript.h>

#include "vmconst.h"


#define WRITE_NEXT_BYTE(var) \
   * cur_byte ++  = (unsigned char) (var)

#define WRITE_NEXT_SHORT(var) \
   * cur_byte ++  = (unsigned char) ( (unsigned int) (var) & 0xFF ); \
   * cur_byte ++  = (unsigned char) ( ( (unsigned int) (var) >> 8) & 0xFF )

#define VM_CONSTANT_LOCATION_TO_SHORT(v) (0xFFFF - (ASM_MAX_CONSTANT - (v))) 
#define VM_GLOBAL_LOCATION_TO_SHORT(v,const_num) (0xFFFF - (const_num) - (ASM_MAX_GLOBAL_VAR - (v))) 


#define WRITE_LOCATION(v, count_constants, count_globals) \
{ \
	if ((v) > ASM_MAX_GLOBAL_VAR ) {  \
		WRITE_NEXT_SHORT( VM_CONSTANT_LOCATION_TO_SHORT(v) ); \
	} else if ( (v) > (ASM_MAX_GLOBAL_VAR  - (count_globals)) ) { \
		WRITE_NEXT_SHORT( VM_GLOBAL_LOCATION_TO_SHORT(v, count_constants) ); \
	} else { \
		WRITE_NEXT_SHORT( (v) ); \
	} \
}

/*
  Function name - ASM_INSTRUCTION_to_bytecode_size
 
  Description - given type of op opcode, return size of the instruction that it represents.
 
  Input - enumeration value on types of opcodes      
 
  Return Code/Output 
 */
int ASM_INSTRUCTION_to_bytecode_size(ASM_TYPE opcode)
{
	//fprintf(stderr,"%x\n",opcode);
	
	switch( opcode )
	{
	case ASM_GOTO:
		return 3;

	case ASM_MOVE:
	case ASM_CPY:
		return 5;

	case ASM_CALL:
	case ASM_XCALL:
		return 4;

	case ASM_VARDEF:
		return 4;

	case ASM_VARUNDEF:
		return 3;

	case ASM_JZ:
	case ASM_JNZ:
		return 5;

	case ASM_RETURN:
		return 3;

	//case ASM_OP_NUM_NEGATE:
	//	break;

	case ASM_ARRAY_LOOKUP:
	case ASM_HASH_LOOKUP:
	case ASM_ARRAY_SET:
	case ASM_HASH_SET:
	case ASM_OP_NUM_EQ:
	case ASM_OP_NUM_NE:
	case ASM_OP_NUM_LT:
	case ASM_OP_NUM_LE:
	case ASM_OP_NUM_ADD:
	case ASM_OP_NUM_SUBST:
	case ASM_OP_NUM_DIV:
	case ASM_OP_NUM_MULT:
	case ASM_OP_NUM_MOD:
	case ASM_OP_STR_EQ:
	case ASM_OP_STR_NE:
	case ASM_OP_STR_LT:
	case ASM_OP_STR_LE:
	case ASM_OP_STR_CAT:
	case ASM_OP_STR_REGEXMATCH:
		return 7;

	case ASM_LABEL:
		return 0;

	default:
		return 1;
	}
}

/*
  Function name - ASM_INSTRUCTION_to_bytecode
 
  Description - given an instruction in memory representaiton (intermedeate code),
				return the bytes that represent this instruction on disk.
 
  Input -       
 
  Return Code/Output 
 */
int ASM_INSTRUCTION_to_bytecode(ASM_INSTRUCTION *instr, void *buf, size_t bufsize, size_t count_constants, size_t count_globals)
{
	unsigned char *cur_byte =  (unsigned char *) buf;
    int opcode;

	V_UNUSED(bufsize);


	WRITE_NEXT_BYTE( instr->type );


	opcode = ASM_INSTRUCTION_get_opcode(instr);


	switch( opcode ) {
	
	case ASM_LABEL:
		return 0;

	case ASM_GOTO:
		// <addr>
		WRITE_NEXT_SHORT( instr->res.arg );
		break;

	case ASM_MOVE:
	case ASM_CPY:
		// <val> <val>
		WRITE_LOCATION(instr->res.arg, count_constants, count_globals);
		WRITE_LOCATION(instr->lhs.arg, count_constants, count_globals);
		break;

	case ASM_CALL:
	case ASM_XCALL:
		WRITE_LOCATION(instr->res.arg, count_constants, count_globals);; // call target
		WRITE_NEXT_BYTE(instr->lhs.arg); // number of parameters
		break;

	case ASM_RETURN:
		WRITE_NEXT_SHORT(instr->res.arg);
		break;

	case ASM_VARDEF:
		// <byte_type> <val>
		WRITE_NEXT_BYTE(instr->res.arg);
		WRITE_LOCATION(instr->lhs.arg, count_constants, count_globals);
		break;

	case ASM_VARUNDEF:
		// <val>
		WRITE_LOCATION(instr->res.arg, count_constants, count_globals);
		break;

	case ASM_JZ:
	case ASM_JNZ:
		// <val> <address>
		WRITE_LOCATION(instr->lhs.arg, count_constants, count_globals);
		WRITE_NEXT_SHORT(instr->res.arg);
		break;

	
	case ASM_OP_NUM_NEGATE:
		break;

	case ASM_ARRAY_LOOKUP:
	case ASM_HASH_LOOKUP:
	case ASM_ARRAY_SET:
	case ASM_HASH_SET:
	case ASM_OP_NUM_EQ:
	case ASM_OP_NUM_NE:
	case ASM_OP_NUM_LT:
	case ASM_OP_NUM_LE:
	case ASM_OP_NUM_ADD:
	case ASM_OP_NUM_SUBST:
	case ASM_OP_NUM_DIV:
	case ASM_OP_NUM_MULT:
	case ASM_OP_NUM_MOD:
	case ASM_OP_STR_EQ:
	case ASM_OP_STR_NE:
	case ASM_OP_STR_LT:
	case ASM_OP_STR_LE:
	case ASM_OP_STR_CAT:
	case ASM_OP_STR_REGEXMATCH:
		// <val> <val> <val>
		WRITE_LOCATION(instr->res.arg, count_constants, count_globals);
		WRITE_LOCATION(instr->lhs.arg, count_constants, count_globals);
		WRITE_LOCATION(instr->rhs.arg, count_constants, count_globals);
		break;

	default:
		return -1;
	}

	return cur_byte - (unsigned char *) buf;
}


int VSCRIPTVM_THREAD_init(VCONTEXT * ctx, VSCRIPTVM_THREAD *thread)
{	
	if (VARR_init(ctx, &thread->stack_frames, sizeof(VSCRIPT_STACK_FRAME), 10 )) {
		return 1;
	}

	if (VARR_init(ctx, &thread->stack_contents, sizeof(void *), 10)) {
		return 1;
	}
	thread->stack_pointer = 0;
	thread->stack_top = 0;
	return 0;
}

VSCRIPTVM_THREAD * VSCRIPTVM_THREAD_new(VCONTEXT * ctx)
{	
	VSCRIPTVM_THREAD *thread;

	thread = (VSCRIPTVM_THREAD *) V_MALLOC( ctx, sizeof(VSCRIPTVM_THREAD) );
	if (!thread) {
		return 0;
	}

	if (VSCRIPTVM_THREAD_init(ctx, thread)) {
		V_FREE( ctx, thread);
		return 0;
	}
	thread->params.retval = 0;
	return thread;
}

void VSCRIPTVM_THREAD_free(VSCRIPTVM_THREAD *thread)
{	
	VARR_free( &thread->stack_frames );
	VARR_free( &thread->stack_contents );
}



V_INLINE int VSCRIPTVM_is_constant( VSCRIPTVM *vm, unsigned int arg_res )
{
	return arg_res > vm->min_constant;
}

	
VSCRIPTVM *VSCRIPTVM_init(VCONTEXT * mctx, XMETHODLIB *xlib)
{
	VSCRIPTVM *ctx = V_MALLOC(mctx, sizeof(VSCRIPTVM) );


	if (!ctx) {
		return 0;
	}

	ctx->ctx = mctx;
	ctx->xlib = xlib;

	return ctx;
}

VSCRIPTVM_STATUS VSCRIPTVM_open_from_image(VSCRIPTVM *ctx, VOS_MAPFILE map_file, int trace)
{
	void *hdr;
	EXE_CODE_SECTION_HDR range;

	// map exe file into memory
	ctx->mapfile = map_file;

	hdr = VOS_MAPFILE_get_ptr( ctx->mapfile );

	ctx->close_map_file = 1;
	
	// check header
	if (EXE_is_header_ok(hdr)) {
		return VSCRIPTVM_STATUS_INVALID_EXE_FILE;
	}

	//VOS_MAPFILE_get_ptr( ctx->mapfile );
	//(size_t) VOS_MAPFILE_get_length(ctx->mapfile);

	range = * ((EXE_CODE_SECTION_HDR *) 
						EXE_get_section_ptr( hdr, EXE_HDR_CODE_SECTION ));
	
	ctx->min_constant = 0xFFFF - range.const_count;
	ctx->min_global = ctx->min_constant - range.global_count;

	// get code section
	ctx->code_base = VPTR_ADD( 
						EXE_get_section_ptr( hdr, EXE_HDR_CODE_SECTION ),
						sizeof(EXE_CODE_SECTION_HDR),
						V_UINT8 *);

	ctx->code_eof = VPTR_ADD( 
						EXE_get_section_ptr( hdr, EXE_HDR_CODE_SECTION ), 
						EXE_get_section_size(hdr, EXE_HDR_CODE_SECTION ),  
						V_UINT8 *);

	// get constant section
	ctx->cvalue = EXE_get_section_ptr( hdr, EXE_HDR_CONSTANT_SECTION );
	ctx->cvalue_length = EXE_get_section_size( hdr, EXE_HDR_CONSTANT_SECTION ) / sizeof(VM_CONSTANT_VALUE); 

	// get string values of constant section
	ctx->string_area = EXE_get_section_ptr( hdr, EXE_HDR_CONSTANT_STRING );
	ctx->string_area_length = EXE_get_section_size( hdr, EXE_HDR_CONSTANT_STRING );
	

	// init runtime stuff.
	ctx->pc = 0;

	ctx->trace_flag = trace ? 1 : 0;

	ctx->exit_status = 0;
	ctx->exit_status_set = 0;

	if (VARR_init( ctx->ctx, &ctx->globals, sizeof(void *), 10)) {
		return VSCRIPTVM_STATUS_INTERNAL_ERROR;
	}
	if ((ctx->thread = VSCRIPTVM_THREAD_new(ctx->ctx)) == 0) {
		return VSCRIPTVM_STATUS_INTERNAL_ERROR;
	}

	return VSCRIPT_STATUS_OK;
}


VSCRIPTVM_STATUS VSCRIPTVM_open(VSCRIPTVM *ctx, const char *exe_file, int trace)
{
	VSCRIPTVM_STATUS  ret;

	ctx->mapfile = VOS_MAPFILE_open(exe_file, VOS_MAPPING_READ, 0);
	if (!ctx->mapfile) {
		return VSCRIPTVM_STATUS_NO_INPUT;
	}

	ret = VSCRIPTVM_open_from_image(ctx, ctx->mapfile, trace);
	ctx->close_map_file = 0;
	return ret;
}

void VSCRIPTVM_close(VSCRIPTVM *vm)
{
	if (vm->close_map_file) {
		VOS_MAPFILE_close(vm->mapfile);
	}
	free(vm);
}

void VSCRIPTVM_release_range( VARR *container, VCONTEXT *ctx, int start, int end)
{
	int i;
	// TODO: forall local variables on current stack frame - decreas refcount
	for(i = start; i < end; i++ ) {
		void **ptr = (void **) VARR_at( container, i );

		if (*ptr) {
			VM_BASE_TYPE  var_type;

			var_type = VM_TYPE_get_base_type( *ptr );
			if (!VM_IS_CONSTANT(var_type)) {

				VM_OBJ_HEADER_release( ctx, *ptr );
			}
		}
	}	
}


#define ERROR_MSG_LEN 512

static void VSCRIPTVM_runtime_error(const char *format, ... )
{
  char msg[ERROR_MSG_LEN];
  int len;
  va_list ap;

  va_start(ap, format);
  len = 
  
#ifdef _WIN32
    _vsnprintf
#else
    vsnprintf
#endif
    (msg, sizeof(msg) - 1, format, ap);
    
  va_end(ap);

  msg[len] = '\0';

  fprintf(stderr,"\nRuntimeError: %s\n", msg);

}


#define GET_NEXT_BYTE(var) \
  (var) = * cur_pc ++ ;

#define GET_NEXT_SHORT(var) \
  (var) = *(cur_pc++); \
  (var) |= *(cur_pc++) << 8;

#define SET_PC(arg) \
  cur_pc = start_pc + (arg);

#define GET_PC \
  ( cur_pc - start_pc )

void *SCRIPTVM_get_value(VSCRIPTVM *vm, unsigned int idx)
{
	void *ret = 0;
	unsigned int pos;
	void **ptr;

	if (VSCRIPTVM_is_constant(vm,idx)) {

		pos = idx - vm->min_constant - 1;

		if (pos < vm->cvalue_length) {
			ret = vm->cvalue + pos;		
		} 

	} else if ( idx > vm->min_global) {
		// get from global pool
		VARR *heap;
	
		heap = &vm->globals;

		pos = idx - vm->min_global - 1;

		ptr = (void **) VARR_at( heap, pos );
		if (ptr) {
			ret = *ptr;
		}

	} else {
		// get from stack
		VARR *stack;
		size_t sidx;
				
		stack = &vm->thread->stack_contents;

		if (idx ) {

			sidx = vm->thread->stack_pointer + idx;

			// all relative the current stack pointer.
			ptr = (void **) VARR_at( stack, sidx );

		} else {

			ptr = (void **) VARR_at( stack, vm->thread->stack_top  );
		
		}

		if (ptr) {
			ret = *ptr;
		}

 
	}

	if (vm->trace_flag) {
		char tmp[100];
		int tmp_size;
			
		fprintf(stderr, "get=%s(%x) ", 
			VM_SCALAR_to_string(vm, ret, tmp, &tmp_size), idx );
	}
	return ret;	
}


int SCRIPTVM_set_value(VSCRIPTVM *vm, unsigned int idx, void *value)
{
	unsigned int pos;

	if (VSCRIPTVM_is_constant(vm,idx)) {
		return -1;
	} else if ( idx >= vm->min_global) {
		// get from global pool
		VARR *heap;
	
		heap = &vm->globals;

		pos = idx - vm->min_global - 1;

		if (VARR_set_at( heap, pos, &value, sizeof( value ))) {
			return -1;
		}
		
	} else {
		// set to stack
		VARR *stack;
		size_t sidx;
				
		stack = &vm->thread->stack_contents;
		
		sidx = vm->thread->stack_pointer + idx;

		if (sidx > vm->thread->stack_top) {
			vm->thread->stack_top = sidx;
		}

		if (VARR_set_at( stack, sidx, &value, sizeof(value))) {
			return -1;
		}

		// if location is on stack and idx == (stack_pointer + 1) -> advance stack pointer

	}
	if (vm->trace_flag) {
		char tmp[100];
		int tmp_size;
			
		fprintf(stderr, "set=%s(%x) ", 
			VM_SCALAR_to_string(vm, value, tmp, &tmp_size), idx );
	}	
	return 0;
}



int VSCRIPTVM_run(VSCRIPTVM *vm)
{
	register unsigned char *cur_pc, *start_pc;
	V_UINT8  op;
	V_UINT16 arg_res,arg_lhs,arg_rhs;
	V_UINT8  var_type;
	VSCRIPTVM_THREAD *cthread = vm->thread; // current thread


	start_pc = vm->code_base;
	cur_pc = start_pc  + vm->pc;

	if (cthread->params.retval) {
		goto ret_val;
	}

	while(1) {

		if (cur_pc > vm->code_eof) {
			VSCRIPTVM_runtime_error("program counter past end of bytecode range pc=%x",GET_PC);
			goto err;
		}

		GET_NEXT_BYTE(op);

		if (vm->trace_flag) {
			fprintf(stderr, "\n0x%05X %03d-%03d|\t%s\t", 
						cur_pc - start_pc - 1, 
						vm->thread->stack_pointer,
						vm->thread->stack_top,
						ASM_TYPE_get_sym_name( op ) );
		}

		// i hope the compiler generates a jump table for this one ... todo: check!
		switch(op) {

		case ASM_VARDEF:{
			VM_OBJ_HEADER *value;

			GET_NEXT_BYTE(var_type); // variable type to define
			GET_NEXT_SHORT(arg_res); // storage location

			// if slot is a constant - error
			if (VSCRIPTVM_is_constant( vm, (size_t) arg_res )) {
				VSCRIPTVM_runtime_error("Trying to define variable in constant range pc=%x index=%d",GET_PC, arg_res);
				goto err;
			}

			// create variable definition (refcount=1).
			value = VM_OBJ_HEADER_new_type(vm->ctx, var_type);
			if (!value) {
				VSCRIPTVM_runtime_error("Can't define value of desired type pc=%x index=%d type=%d",GET_PC, arg_res, var_type);
				goto err;
			}
 
			// set slot value - check that slot is not defined.
			if (SCRIPTVM_set_value(vm, (size_t) arg_res, value)) {
				goto set_err;
			}
		
		}
		break;

		case ASM_VARUNDEF: {
			void *value;
			
			GET_NEXT_SHORT(arg_res); // storage location


			// check that location is currently taken.
			value = SCRIPTVM_get_value(vm, arg_res);
			if (!value) {
				VSCRIPTVM_runtime_error("Undef var that is not defined  pc=%x location=%d",GET_PC, arg_res);
				goto err;

			}

		
			// decreast refcount on desired slot and set slot status to undefined
			VM_OBJ_HEADER_release(vm->ctx,  value);
			
			// ups: what is happeneing to the stack counter ?
		}
		break;

		case ASM_CPY: {	
			void *value;

			GET_NEXT_SHORT(arg_res);    // target of move
			GET_NEXT_SHORT(arg_rhs);	// source of move		


			value = SCRIPTVM_get_value(vm, arg_rhs);
			if (!value) {
				VSCRIPTVM_runtime_error("Can't get right hand side pc=%x code=%d method_id=%d",GET_PC, op, arg_res);
				goto err;
			}

			
			if (VM_IS_SCALAR( VM_TYPE_get_base_type(value) ) ) {
				// create copy of value
				value = VM_OBJ_HEADER_copy_scalar( vm, value );
				if (!value) {
					VSCRIPTVM_runtime_error("Can't create copy of right hand side pc=%x code=%d method_id=%d",GET_PC, op, arg_res);
					goto err;
					
				}
			} else {
				VM_OBJ_HEADER_add_ref( value );	
			}

			// well arg_res can't be a constant too - if yes then issue error
			if (SCRIPTVM_set_value(vm, arg_res, value)){
				goto set_err;
			}
		}
		break;

		case ASM_MOVE: {
			void *value;

			GET_NEXT_SHORT(arg_res);    // target of move
			GET_NEXT_SHORT(arg_rhs);	// source of move		


			value = SCRIPTVM_get_value(vm, arg_rhs);
		
			// if argument source is constant - no add ref / else addref			
			//if () {
			if (!VSCRIPTVM_is_constant(vm, arg_rhs) &&
				!VM_IS_SCALAR( VM_TYPE_get_base_type(value) ) ) {
				VM_OBJ_HEADER_add_ref( value );	
			}

			// well arg_res can't be a constant too - if yes then issue error
			if (SCRIPTVM_set_value(vm, arg_res, value)) {
				goto set_err;
			}
			
		}
		break;

		case ASM_GOTO: {
			GET_NEXT_SHORT(arg_res);
			SET_PC( arg_res );
		}
		break;

		// conditional jump
		case ASM_JZ:
		case ASM_JNZ: {
			void *index_value;
			unsigned long index_btype;
			long val;

			GET_NEXT_SHORT(arg_rhs); // conditional location
 			GET_NEXT_SHORT(arg_res); // jump target

			index_value = SCRIPTVM_get_value(vm, arg_rhs);
			if (!index_value) {
				VSCRIPTVM_runtime_error("Index expression not defined pc=%x code=%d",GET_PC, op);
				goto err;
			}

			// type check (supposed to be scalar)
			index_btype = VM_TYPE_get_base_type(index_value);
			if (! VM_IS_SCALAR( index_btype ) || index_btype != VM_LONG ) {
				VSCRIPTVM_runtime_error("Index expression is supposed to be of type long pc=%x code=%d",GET_PC, op);
				goto err;
			}

			val = VM_SCALAR_to_long(vm, index_value);
			if (op == ASM_JNZ) {
				if (val) {
				  SET_PC( arg_res );
				}
			} else {
				if (!val) {
				   SET_PC( arg_res );
				}
			}
		}
		break;

		case ASM_XCALL:
		case ASM_CALL: {
			V_UINT8 num_params;
			VSCRIPT_STACK_FRAME rec;
			size_t i;

			GET_NEXT_SHORT(arg_res); // call target
			GET_NEXT_BYTE(num_params); // number of parameters

			// create activation record and push it to activation record stack
			rec.function_pc = arg_res;
			rec.ret_addr = GET_PC;
			rec.stack_pointer_on_call = cthread->stack_pointer;
			rec.stack_top_on_call = cthread->stack_top;
			rec.num_args = num_params;

			// push to activation record stack
			VARR_push_back( &cthread->stack_frames, &rec, sizeof(VSCRIPT_STACK_FRAME));

			if (VARR_size(&cthread->stack_frames) > 1) {
			   cthread->stack_pointer = cthread->stack_top - num_params ;
			}
			
			//dump_stack(vm);

			if (op == ASM_CALL) {

				// goto stack
				SET_PC( arg_res );

			} else  { // xtension method call 
				XMETHODACTION *action;
				
				// lookup of method action in extension library definition
				action = XMETHODLIB_lookup_action( vm->xlib, arg_res );


				if (!action) {
					VSCRIPTVM_runtime_error("Extension library method is not defined pc=%x code=%d method_id=%d",GET_PC, op, arg_res);
					goto err;
				}

				// pack call into structure, so that they can be retrieved later.
				if (num_params > MAX_XMETHOD_PARAMS) {
					VSCRIPTVM_runtime_error("Extension library gets to many parameters  pc=%x code=%d method_id=%d param_count=%d",GET_PC, op, arg_res, num_params);
					goto err;
				}

				

				for(i = 0; i < num_params; i++ ) {
					cthread->param_data[ i ] = (VM_OBJ_HEADER *) 
						SCRIPTVM_get_value( vm, i + 1 );
				}

				cthread->params.retval = 0;
				cthread->params.param_count = num_params;
				cthread->params.param = cthread->param_data;


				switch(action->action_type) {
				case XMETHOD_ASYNCH:
					// suspend running of this vm.

					vm->pc = cur_pc - start_pc;
					cthread->method_id = action->method_id;//(int) arg_res;
					return VSCRIPT_STATUS_VM_SUSPEND_FOR_XMETHOD;

				case XMETHOD_CALLBACK: {// call C implementation of the extension method
					typedef int (VM_CB) (VSCRIPTVM *ctx, int method_id,  VSCRIPT_XMETHOD_FRAME *frame);
					int rt;
					
					// call method
					rt = ((VM_CB *) (action->callback_ptr)) ( vm, action->method_id,  &cthread->params);

					if (vm->exit_status_set) {
						return VSCRIPT_STATUS_OK;
					}

					if (rt || !cthread->params.retval) {
						VSCRIPTVM_runtime_error("Extension library method failed pc=%x code=%d method_id=%d",GET_PC, op, arg_res);
						goto err;
					}


					// put return value to stack
					/*
					VARR_copy_at( 
						&cthread->stack_contents, 
						rec.stack_pos_on_call, 
						cthread->params.retval, 
						sizeof(void *) );
					*/


ret_val:
					SCRIPTVM_set_value( vm, 0, cthread->params.retval);
					
					{
					VSCRIPT_STACK_FRAME record;

					// pop function activation stack
					VARR_pop_back(&cthread->stack_frames,&record, sizeof(VSCRIPT_STACK_FRAME));
				  //cthread->stack_pointer = rec.stack_pointer_on_call;
					cthread->stack_pointer = record.stack_pointer_on_call;
				  //cthread->stack_top = rec.stack_top_on_call - record.num_args;
					cthread->stack_top = record.stack_top_on_call - record.num_args;
					}

					}
					break;
					
				case XMETHOD_NOTIMPLEMENTED:
					VSCRIPTVM_runtime_error("Extension library method has no implementation pc=%x code=%d method_id=%d",GET_PC, op, arg_res);				goto err;
					goto err;
				}
			}

		}
		break;



		case ASM_RETURN: {
			void *value;
			int next_stack_top;

			GET_NEXT_SHORT(arg_res); // return value 
		
			if ( VARR_size(&cthread->stack_frames)) {
				VSCRIPT_STACK_FRAME record;

				// put return value arg_res into right slot.
				value = SCRIPTVM_get_value(vm, arg_res);
			
				// if argument source is constant - no add ref / else addref			
				//if (!VSCRIPTVM_is_constant(vm, arg_rhs)) {
				if (! VM_IS_SCALAR( VM_TYPE_get_base_type(value) ) ) {
					VM_OBJ_HEADER_add_ref( value );	
				}

				// well arg_res can't be a constant too - if yes then issue error
				if (SCRIPTVM_set_value(vm, 0, value)) {
					goto set_err;
				}
			
				//dump_stack(vm);
				
				// pop entry from invocation stack of current thread
				VARR_pop_back(&cthread->stack_frames,&record, sizeof(VSCRIPT_STACK_FRAME));


				next_stack_top = record.stack_top_on_call - record.num_args;


				//VSCRIPTVM_release_range( &vm->thread->stack_contents, vm->ctx, next_stack_top + 1, cthread->stack_top);


				// set back current stack pointer			
				cthread->stack_pointer = record.stack_pointer_on_call;
				cthread->stack_top = record.stack_top_on_call - record.num_args;

				// set PC to return value
				SET_PC( record.ret_addr ); 

			} else {

				//VSCRIPTVM_release_range( &vm->thread->stack_contents, vm->ctx, 0, cthread->stack_top + 1);
				//VSCRIPTVM_release_range( &vm->globals, vm->ctx, 0, VARR_size(&vm->globals) );

				return VSCRIPT_STATUS_OK;
			}
		}
		break;
		


		/*
		case ASM_HASH_LOOKUP: {
			GET_NEXT_SHORT(arg_res); // store result of lookup here
			GET_NEXT_SHORT(arg_lhs); // storage location of hash
			GET_NEXT_SHORT(arg_rhs); // storage location of index expression
		}
		break;
		
		case ASM_HASH_SET: {
			GET_NEXT_SHORT(arg_res); // value to be stored in array
			GET_NEXT_SHORT(arg_lhs); // storage location of array
			GET_NEXT_SHORT(arg_rhs); // storage location of index expression
		}
		break;
		*/

		case ASM_HASH_LOOKUP:
		case ASM_HASH_SET:
		case ASM_ARRAY_LOOKUP: 
		case ASM_ARRAY_SET: {
			void *index_value, *collection_value, *value;
			unsigned long index_btype;

			GET_NEXT_SHORT(arg_res); // value to be stored in array
			GET_NEXT_SHORT(arg_lhs); // storage location of array
			GET_NEXT_SHORT(arg_rhs); // storage location of index expression
			
			
			// get index expression
			index_value = SCRIPTVM_get_value(vm, arg_rhs);
				if (!index_value) {
				VSCRIPTVM_runtime_error("Index expression not defined pc=%x code=%d",GET_PC, op);
				goto err;
			}

			// get collection obect
			collection_value = SCRIPTVM_get_value(vm, arg_lhs);

			if (op == ASM_ARRAY_SET || op == ASM_ARRAY_LOOKUP) {

				// index type (supposed to be scalar)
				index_btype = VM_TYPE_get_base_type(index_value);
				if (VM_IS_SCALAR( index_btype ) != VM_LONG ) {
					VSCRIPTVM_runtime_error( "Index expression is supposed to be of type long pc=%x code=%d",GET_PC, op);
					goto err;
				}
				
				if ( !collection_value || VM_TYPE_get_base_type( collection_value ) != VM_ARRAY) {
					VSCRIPTVM_runtime_error("Array argument is supposed to be an array. pc=%x code=%d",GET_PC, op);
					goto err;
				}

			} else {
				if ( !collection_value || VM_TYPE_get_base_type( collection_value ) != VM_HASH) {
					VSCRIPTVM_runtime_error("Hash argument is supposed to be a hash. pc=%x code=%d",GET_PC, op);
					goto err;
				}
			}


			if (op == ASM_ARRAY_SET || op == ASM_HASH_SET) {
				// get result value
				value = SCRIPTVM_get_value(vm, arg_res);
				if (!value) {
					VSCRIPTVM_runtime_error("Result value not defined. pc=%x code=%d",GET_PC, op);
					goto err;
				}

				value = VM_OBJ_HEADER_const2scalar( vm, value);
				if (!value) {
					VSCRIPTVM_runtime_error("Can't make var from const. pc=%x code=%d",GET_PC, op);
					goto err;
				}
			}


			switch(op) {
			case ASM_HASH_SET:

				if (VM_VALUE_HASH_set( vm,  
						(VM_VALUE_HASH *) collection_value, 
						(VM_OBJ_HEADER *) index_value, 
						value
						)) {
					goto set_err;
				}
				break;
			case ASM_ARRAY_SET: 

				if (VM_VALUE_ARRAY_set( vm->ctx,  
						(VM_VALUE_ARRAY *) collection_value, 
						VM_SCALAR_to_long(vm,index_value), 
						value
						)) {
					goto set_err;
				}
				break;

			case ASM_HASH_LOOKUP: {
				VM_OBJ_HEADER *rval;
				
				
				rval = VM_VALUE_HASH_get( vm,
						(VM_VALUE_HASH *) collection_value, 
						(VM_OBJ_HEADER *) index_value); 

				if (!rval) {
					rval = (VM_OBJ_HEADER *) VM_VALUE_STRING_init(vm->ctx, 0);					

				}		
				if (SCRIPTVM_set_value( vm, arg_res, rval)) {
					goto set_err;
				}
				}
				break;

			case ASM_ARRAY_LOOKUP: {
				VM_OBJ_HEADER *rval;
				
				
				rval = VM_VALUE_ARRAY_get(
						(VM_VALUE_ARRAY *) collection_value, 
						VM_SCALAR_to_long(vm,index_value)); 

				if (!rval) {
					rval = (VM_OBJ_HEADER *) VM_VALUE_STRING_init(vm->ctx, 0);					

				}		
				if (SCRIPTVM_set_value( vm, arg_res, rval)) {
					goto set_err;
				}
				}
				break;
			}

		}
		break;


		case ASM_OP_NUM_NEGATE: {
		}
		break;

		case ASM_OP_NUM_EQ: 
		case ASM_OP_NUM_NE: 
		case ASM_OP_NUM_LT: 
		case ASM_OP_NUM_LE: 

		case ASM_OP_NUM_ADD: 
		case ASM_OP_NUM_SUBST: 
		case ASM_OP_NUM_DIV: 
		case ASM_OP_NUM_MULT: 
		case ASM_OP_NUM_MOD: {
			void *lhs,*rhs;
			VM_BASE_TYPE  blhs, brhs, common;
		

			GET_NEXT_SHORT(arg_res); // value to be stored in array
			GET_NEXT_SHORT(arg_lhs); // storage location of array
			GET_NEXT_SHORT(arg_rhs); // storage location of index expression


			//if (op == ASM_OP_NUM_ADD) {
			//	dump_stack(vm);
			//}

			// get value objects
			lhs = SCRIPTVM_get_value(vm, arg_lhs);
			if	 (!lhs) {
				VSCRIPTVM_runtime_error("Left hand side arg not define pc=%x code=%d",GET_PC, op);
				goto err;
			}

			// type check (supposed to be scalar)
			blhs = VM_TYPE_get_base_type(lhs) & ~VM_CONSTANT;
			if (!  VM_IS_SCALAR( blhs)  ) {
				VSCRIPTVM_runtime_error("Left hand side arg supposed to be scalar pc=%x code=%d",GET_PC, op);
				goto err;
			}
			
			rhs = SCRIPTVM_get_value(vm, arg_rhs);
			if (!rhs) {
				VSCRIPTVM_runtime_error("Right hand side arg not define pc=%x code=%d",GET_PC, op);
				goto err;
			}

			brhs = VM_TYPE_get_base_type(lhs) & ~VM_CONSTANT;
			if (! VM_IS_SCALAR( brhs ) ) {
				VSCRIPTVM_runtime_error("Right hand side arg supposed to be scalar pc=%x code=%d",GET_PC, op);
				goto err;
			}

			if (op >= ASM_OP_NUM_EQ && op <= ASM_OP_NUM_LE) {
				long res;
				VM_VALUE_LONG * lval;

				common = blhs < brhs ? blhs : brhs;



				// perform op
				if (common == VM_LONG){
					long vlhs = VM_SCALAR_to_long(vm, lhs);
					long vrhs = VM_SCALAR_to_long(vm, rhs);
									
					switch(op) {
						case ASM_OP_NUM_EQ: { 
							res = vlhs == vrhs;
						}
						break;

						case ASM_OP_NUM_NE: {
							res = vlhs != vrhs;
						}
						break;

						case ASM_OP_NUM_LT: {
							res = vlhs < vrhs;
						}
						break;

						case ASM_OP_NUM_LE: 
						{
							res = vlhs <= vrhs;
						}
						break;
					}
				} else if (common == VM_DOUBLE) {

					double vlhs = VM_SCALAR_to_double(vm, lhs);
					double vrhs = VM_SCALAR_to_double(vm, rhs);
				
					switch(op) {
						case ASM_OP_NUM_EQ: { 
							res = vlhs == vrhs;
						}
						break;

						case ASM_OP_NUM_NE: {
							res = vlhs != vrhs;
						}
						break;

						case ASM_OP_NUM_LT: {
							res = vlhs < vrhs;
						}
						break;

						case ASM_OP_NUM_LE: 
						{
							res = vlhs <= vrhs;
						}
						break;
					}
				}
				
				lval = VM_VALUE_LONG_init(vm->ctx, res );
				if (SCRIPTVM_set_value(vm, arg_res, lval)) {
					goto set_err;
				}

	
			} else {
	
				// what is the lowest common integer type 
				// string, long, => long
				// string, long, double => double
				common = blhs < brhs ? blhs : brhs;

				// perform op
				if (common == VM_LONG){
					long vlhs = VM_SCALAR_to_long(vm, lhs);
					long vrhs = VM_SCALAR_to_long(vm, rhs);
					long res;
					VM_VALUE_LONG * lval;

					switch(op) {

						case ASM_OP_NUM_ADD: {
							res = vlhs + vrhs;
						}
						break;
						case ASM_OP_NUM_SUBST:  {
							res = vlhs - vrhs;
						}
						break;
						case ASM_OP_NUM_DIV:  {
							if (vrhs == 0) {
								VSCRIPTVM_runtime_error("Divide by zero error, pc=%x code=%d",GET_PC, op);
								goto err;
							}
							res = vlhs /  vrhs;
						}
						break;
						case ASM_OP_NUM_MULT: {
							res = vlhs *  vrhs;
						}
						break; 
						case ASM_OP_NUM_MOD:  {
							if (vrhs == 0) {
								VSCRIPTVM_runtime_error( "Divide by zero error, pc=%x code=%d",GET_PC, op);
								return -1;
							}
							res = vlhs %  vrhs;
						}
						break;
					}
					
					lval = VM_VALUE_LONG_init(vm->ctx, res );
					if (SCRIPTVM_set_value(vm, arg_res, lval)) {
						goto set_err;
					}
					
				} else if (common == VM_DOUBLE) {

					double vlhs = VM_SCALAR_to_double(vm, lhs);
					double vrhs = VM_SCALAR_to_double(vm, rhs);
					double res;
					VM_VALUE_DOUBLE *lval;

					switch(op) {
						case ASM_OP_NUM_ADD: {
							res = vlhs +  vrhs;
						}
						break;
						case ASM_OP_NUM_SUBST:  {
							res = vlhs - vrhs;
						}
						break;
						case ASM_OP_NUM_DIV:  {
							if (vrhs == 0) {
								VSCRIPTVM_runtime_error("Divide by zero error, pc=%x code=%d",GET_PC, op);
								goto err;
							}
							res = vlhs / vrhs;
						}
						break;
						case ASM_OP_NUM_MULT: {
							res = vlhs * vrhs;
						}
						break; 

						// what if lhs is integer ? then mod is feasible.
						case ASM_OP_NUM_MOD:  {
							VSCRIPTVM_runtime_error("Modulus operation with double arguments, pc=%x code=%d",GET_PC, op);
							goto err;
						}
						break;
					}
					
					lval = VM_VALUE_DOUBLE_init(vm->ctx, res );
					if (SCRIPTVM_set_value(vm, arg_res, lval)) {
						goto set_err;
					}

				}
			}
		}
		break;

		case ASM_OP_STR_EQ:
		case ASM_OP_STR_NE:
		case ASM_OP_STR_LT:
		case ASM_OP_STR_LE:
		case ASM_OP_STR_CAT:
		case ASM_OP_STR_REGEXMATCH: {
			void *lval,*lhs,*rhs;
			const char *slhs, *srhs;
			char tmp_lhs[100], tmp_rhs[100]; //TODO: what is maximum size of double as string ?
			int lhs_size, rhs_size;

			GET_NEXT_SHORT(arg_res); // value to be stored in array
			GET_NEXT_SHORT(arg_lhs); // storage location of array
			GET_NEXT_SHORT(arg_rhs); // storage location of index expression

			// get argument value objects.
			lhs = SCRIPTVM_get_value(vm, arg_lhs);

			if (!lhs) {
				VSCRIPTVM_runtime_error("Left hand side arg not define pc=%x code=%d",GET_PC, op);
				goto err;

			}

			rhs = SCRIPTVM_get_value(vm, arg_rhs);
			
			if (!rhs) {
				VSCRIPTVM_runtime_error("Right hand side arg not define pc=%x code=%d",GET_PC, op);
				goto err;
			}

			// argument type check (assumed to be scalar) 
			if (! VM_IS_SCALAR( VM_TYPE_get_base_type(lhs) ) ) {
				VSCRIPTVM_runtime_error("string operator: Left hand side arg supposed to be scalar pc=%x code=%d",GET_PC, op);
				goto err;
			}

			if (! VM_IS_SCALAR( VM_TYPE_get_base_type(rhs) ) ) {
				VSCRIPTVM_runtime_error("string operator: Right hand side arg supposed to be scalar pc=%x code=%d",GET_PC, op);
				goto err;
			}

			// convert arguments to string representation
			slhs = VM_SCALAR_to_string(vm, lhs, tmp_lhs, &lhs_size);
			srhs = VM_SCALAR_to_string(vm, rhs, tmp_rhs, &rhs_size);

			// dispatch on operator type
			switch( op ) {
				case ASM_OP_STR_EQ:
					lval = VM_VALUE_LONG_init( vm->ctx, 
								strcmp( slhs, srhs ) == 0 );
					break;
				case ASM_OP_STR_NE:
					lval = VM_VALUE_LONG_init( vm->ctx, 
								strcmp( slhs, srhs ) != 0 );
					break;
				case ASM_OP_STR_LT:
					lval = VM_VALUE_LONG_init( vm->ctx, 
								strcmp( slhs, srhs ) < 0 );
					break;
				case ASM_OP_STR_LE:
					lval = VM_VALUE_LONG_init( vm->ctx, 
								strcmp( slhs, srhs ) <= 0 );
					break;
				case ASM_OP_STR_CAT:
					lval = VM_VALUE_STRING_init_add( vm->ctx, slhs, lhs_size, srhs, rhs_size);

					break;
				case ASM_OP_STR_REGEXMATCH: 
					break;

			}
			if (SCRIPTVM_set_value(vm, arg_res, lval)) {
				goto set_err;
			}
			

			
		}
		break;

		default:	
		VSCRIPTVM_runtime_error( "Illegal instruction at pc=%x code=%d",GET_PC, op );
		goto err;
		}

	}
	return 0;

set_err:
	VSCRIPTVM_runtime_error("Can't modify variable %x. pc=%x opcode=%d", 
		arg_res, GET_PC, op);
err:
    VSCRIPTVM_stack_trace( vm );
	return -1;

}
