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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vscript.h>
#include <rtl.h>

extern const char * yytext;

void PrintUsage(char *name)
{

	fprintf(stderr,"%s <in_file> <trace_dir> [<trace_options>]\n"
				   "\n"
				   "This is the test driver for vscript compiler\n"
				   " <in_file>   - source file\n"
				   " <trace_dir> - output directory\n"
				   " <trace_options> - 0 no trace, 1 do trace", name );
	exit(10);
				   
}	


/* *** for test purposes: register an asynchronous method *** */
static VSCRIPTXLIB_METHOD_TABLE aio_methods[] = {

	{ 1000,  "aio_inc",  "s:s"  },
	{ -1, 0, 0  }
};


/* for test purpose: service asynchronous method call for test function */
int do_asynch_methods(VSCRIPT_VM_CTX *vm)
{
	int method_id;
	VSCRIPT_XMETHOD_FRAME *frame;
	void *rvalue;
	long arg_value;


	VSCRIPT_VM_async_xmethod_frame(vm, &method_id, &frame);

	switch( method_id ) {

		case 1000: { // id that this function was registered with 

			fprintf(stderr,"\n*** suspend vm to service asynchronous xmethod %d num parameters %d ***\n", method_id, frame->param_count);

			arg_value = VM_SCALAR_to_long( (void *) vm, frame->param[0] );

			rvalue = VM_VALUE_LONG_return( vm, arg_value + 1 );

			VSCRIPT_VM_async_xmethod_return(vm, rvalue);

			fprintf(stderr,"*** resume vm  ***\n");

			return 0;
		}

		default:
			return -1;
	}
}

/* ********************************************** *
	Test main. 
	this program is  Invoked by regression tests
 * ********************************************** */
int main(int argc,char *argv[] )
{
	int ret = 0;
	VSCRIPT_CL_CTX *ctx;
	VSCRIPTXLIB *lib = 0;
	int trace_vm = 0;

	if (argc < 3 ) {
		PrintUsage(argv[0]);
	}

	/* *** init library of extension functions *** */
	lib = VSCRIPTXLIB_init( 0 );

	/* *** register function of runtime library (built in functions) *** */
	if (RTL_register(lib)) {
		fprintf(stderr,"Error: can't init runtime library\n");
		return -1;
	}

	if (VSCRIPTXLIB_add_xmethod_async_table(lib, aio_methods)) {
		fprintf(stderr,"Error: can't init asynchronous methods\n");
		return -1;
	}


	/* *** compile to bytecode *** */
	ctx = VSCRIPT_CL_init( 0 );

	VSCRIPT_CL_set_debug_options(ctx, 
			VSCRIPT_CL_DEBUG_DUMP_AST | 
			VSCRIPT_CL_DEBUG_DUMP_LISTING | 
			VSCRIPT_CL_DEBUG_TRACE | 
			VSCRIPT_CL_DEBUG_DUMP_LISTING_LINKED);
	VSCRIPT_CL_set_output_dir(ctx, argv[2]);

	ret = VSCRIPT_CL_compile_to_bytecode(ctx, argv[1], lib);

 	VSCRIPT_CL_close(ctx);


	/* *** if compilation succeded - run virtual machine *** */
	if (!ret) {

		VSCRIPT_VM_CTX *vm;

		if (argc >= 4 && atoi( argv[3] ) ) {
			trace_vm = 1;
		}
		
		vm = VSCRIPT_VM_init( 0, lib );

		if( (ret = VSCRIPT_VM_run(vm, argv[1], argv[2], trace_vm)) != VSCRIPT_STATUS_OK) {		
 			do {
				if (ret == VSCRIPT_STATUS_VM_SUSPEND_FOR_XMETHOD) {

					if (do_asynch_methods( vm )) {
						fprintf(stderr,"Error while executing asynchronous function\n");
						break;
					}
				
				} else {
					// some kind of error
					break;
				}
			} while( (ret =	VSCRIPT_VM_resume(vm)) != 0);
		}
	
		VSCRIPT_VM_close( vm );
	}

	exit(ret);
}	
