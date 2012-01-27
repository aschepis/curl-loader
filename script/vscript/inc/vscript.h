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
#ifndef __VSCRIPT_H_Y__
#define __VSCRIPT_H_Y__

#include <util/vbasedefs.h>
#include <wrapper/vwrapper.h>
#include "vmvalue.h"

#ifdef  __cplusplus
extern "C" {
#endif

/* *** common status code *** */
typedef enum {
	VSCRIPT_STATUS_OK,
	VSCRIPT_STATUS_INTERNAL_ERROR,

	/* compiler status */
	VSCRIPT_STATUS_CL_NO_INPUT_ERROR,
	VSCRIPT_STATUS_CL_SYNTAX_ERROR,
	VSCRIPT_STATUS_CL_SEMANTIC_OR_CODEGEN_ERROR,
	VSCRIPT_STATUS_CL_BACKREF_ERROR,
	VSCRIPT_STATUS_CL_WRITE_EXE_ERROR,

	/* xmethod status */
	VSCRIPT_STATUS_XMETHOD_WRONG_PARAMETER_SPEC,
	VSCRIPT_STATUS_XMETHOD_METHOD_ID_REPEATED,
	VSCRIPT_STATUS_XMETHOD_METHOD_ALREADY_DEFINED,

	/* virtual machine status */
	VSCRIPT_STATUS_VM_RUNTIME_ERROR,
	VSCRIPT_STATUS_VM_SUSPEND_FOR_XMETHOD,
}
	VSCRIPT_STATUS;

/* *** EXTENSION METHOD LIBRARY INTERFACE * ***/
typedef  void * VSCRIPTXLIB;

typedef enum {
  VSCRIPTXLIB_CLOPEN = 0x1,
  VSCRIPTXLIB_VMOPEN = 0x2,
} 
  VSCRIPTXLIB_OPENMODE;

/** 
 * callback method for servicing of extension method.
 
 * @param ctx		(in) the script context
 * @param method_id (in) the unique id of the extension method
 * @param params	(in|out) the method parameters of the function
 * @param retval	(in|out) the return value of the function
 */
typedef int (* VSCRIPT_VM_XMETHOD_CALLBACK) (void  *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *frame);

typedef struct tagVSCRIPTXLIB_METHOD_TABLE {
	int method_id;
	char *func_name;
	char *prototype_spec;

} VSCRIPTXLIB_METHOD_TABLE;

typedef struct tagVSCRIPTXLIB_METHOD_CALLBACK_TABLE {
	int method_id;
	char *func_name;
	char *prototype_spec;
	VSCRIPT_VM_XMETHOD_CALLBACK callback_impl;

} VSCRIPTXLIB_METHOD_CALLBACK_TABLE;


V_EXPORT VSCRIPTXLIB *VSCRIPTXLIB_init();

V_EXPORT void VSCRIPTXLIB_free(VSCRIPTXLIB *);



/** 
   function prototypes
   prototype is a string of the following format, defining the prototype of a function, in EBNF notation.

	<prototype> ::= <parameter-spec> ':' <return-value-spec>
 
    <parameter-spec> ::= <parametera-spec> ',' <type-spec>  | <type-spec>
	
    <return-value-spec> ::= <type-spec>
	
	<type-spec> ::=  '%' | '@' | 's'
						
		meaning:  '%'	- hash value
				  '@'	- array value
				  's'	- scalar value

	Example:
	
	 %,s,@:s
	 prototype of a function that returns a scalar, and where the parameter list
	 consists of a hash value, a scalar and an array.
*/

/**
 * @brief register an extension method as suspend-vm/resume-vm protocol.
 
 * @param ctx		the script context
 * @param method_id id of method (must be unique)
 * @param func_name name of function as it appears in the scripting language
 * @param prototype definition of prototype of function as it appears in the scripting language
 * @returns 0 on success

 * method_id parameter is the unique identifier of this extended function.
 
 * An extension method appears as a regular function in the scripting language, but this function is implemented by C code.
 * (in other words a mechanism to call a C code from the scripting language)
 * This mechanism is used to add features to the scripting language that could not be implemented
 * by means of scripting language alone, or to add faster implementation of a given function..

 * you must call this function before compiling (i.e. before calling VSCRIPT_CL_compile_to_bytecode).

 * In this case the C implementation of the function is *not* provided by a callback function,
 * instead when the function is called, the runtime suspends execution , returning special status
 * VSCRIPT_SUSPENDED_ON_FUNCTION_CALL, The caller then retrives both the unique method id and the
 * function call parameters by calling VSCRIPT_VM_axync_xmethod_params
 
 * As the C code finishes servicing of this asynchronous extension method call, the user
 * resumes execution of the runtime and by calling VSCRIPT_VM_resume; here we 
 * are also passing the extension method return values.
 
 * this particular extension mechanism is designed to support asynchronous operations in servers environments 
 * that operate asynchronously.
 
 */
V_EXPORT int VSCRIPTXLIB_add_xmethod_async(VSCRIPTXLIB *ctx, 
									int method_id, 
									const char *func_name,
									const char *prototype
									);


/**
 * Register a set of asynchronous functions from table definition
 */
V_EXPORT int VSCRIPTXLIB_add_xmethod_async_table(VSCRIPTXLIB *ctx, 
									VSCRIPTXLIB_METHOD_TABLE *tbl
									);


/** 
 * Registers method but does not provide implementation.
 * This is used when you want to disable availability of a method in
 * the current environment, but you don't want to break
 * compilation of the script for this purpose,
 * Calling a disabled method will result in a runtime error.
 */
V_EXPORT int VSCRIPTXLIB_add_xmethod_nomethod(VSCRIPTXLIB *ctx, 
									int method_id, 
									const char *func_name,
									const char *prototype
									);


/**
 * Register a set of undefined functions from table definition
 */
V_EXPORT int VSCRIPTXLIB_add_xmethod_nomethod_table(VSCRIPTXLIB *ctx, 
									VSCRIPTXLIB_METHOD_TABLE *tbl
									);


/**
 * @brief register an extension method as callback function
 
 * @param ctx		the script context
 * @param method_id id of method (must be unique)
 * @param func_name name of function as it appears in the scripting language
 * @param prototype definition of prototype of function as it appears in the scripting language
 * @param callbck	implementation of callback function
 * @returns 0 on success

 * method_id parameter is the unique identifier of this extended function, it is later passed to extension function
 
 * An extension method appears as a regular function in the scripting language, but this function is implemented by C code.
 * (in other words a mechanism to call a C code from the scripting language)
 * This mechanism is used to add features to the scripting language that could not be implemented
 * by means of scripting language alone, or to add faster implementation of a given function..

 * you must call this function before compiling (i.e. before calling VSCRIPT_CL_compile_to_bytecode).

 * callback  provides implementation of extension method (extension methods are declared by VSCRIPT_CL_CTX_add_xmethod)  
 * In this case the C implementation of the function is provided by callback function.
 
 */
V_EXPORT int VSCRIPTXLIB_add_xmethod_callback(VSCRIPTXLIB *ctx, 
									 int method_id, 
									 const char *func_name,
									 const char *prototype,
									 VSCRIPT_VM_XMETHOD_CALLBACK callback  
									 );

V_EXPORT int VSCRIPTXLIB_add_xmethod_callback_table(VSCRIPTXLIB *ctx, 
									 VSCRIPTXLIB_METHOD_CALLBACK_TABLE *tbl
									 );


/* *** VSCRIPT BYTECODE COMPILER INTERFACE *** */

typedef enum {
	VSCRIPT_CL_DEBUG_DUMP_AST = 1,
	VSCRIPT_CL_DEBUG_DUMP_LISTING = 2,
	VSCRIPT_CL_DEBUG_DUMP_LISTING_LINKED = 4,
	VSCRIPT_CL_DEBUG_TRACE = 8,
}
	VSCRIPT_CL_DEBUG_OPTS;


typedef void *VSCRIPT_CL_CTX;

/**
 * @brief initialise script compiler context.
 * The scripting compiler context is passed to all compiler function
 * @param 
 */
V_EXPORT VSCRIPT_CL_CTX *VSCRIPT_CL_init(VCONTEXT *ctx);



/**
 * @brief cleanup script context

 * @parma ctx		the script context
 */
V_EXPORT void VSCRIPT_CL_close(VSCRIPT_CL_CTX *ctx);


/**
 * @brief set debug options
 */
V_EXPORT void VSCRIPT_CL_set_debug_options(VSCRIPT_CL_CTX *ctx, int options);


/**
 * @brief set output directory (both trace and output bytcode file)
 */
V_EXPORT void VSCRIPT_CL_set_output_dir( VSCRIPT_CL_CTX *cctx, const char *output_directory );

/**
 * @brief compile a source file to bytecode
 
 * @parma ctx		the script context
 * @param source_file the path of script souce
 
 */
V_EXPORT VSCRIPT_STATUS VSCRIPT_CL_compile_to_bytecode(VSCRIPT_CL_CTX *ctx, const char *source_file,  VSCRIPTXLIB *extlib );



/* *** VSCRIPT VIRTUAL MACHINE INTERFACE *** */

typedef void *VSCRIPT_VM_CTX;


/**
 * @brief initialise virtual machine context.
 * The virtual machine context pointer is passed to all vm function
 * @param 
 */
V_EXPORT VSCRIPT_VM_CTX *VSCRIPT_VM_init(VCONTEXT *ctx, VSCRIPTXLIB *xlib);

/**
 * @brief cleanup virtual machine context

 * @parma ctx		the context
 */
V_EXPORT void VSCRIPT_VM_close(VSCRIPT_VM_CTX *ctx);


/**
 * @brief Invoke and run the virtual machine.

 * @parma ctx		the script context
 * @param fname		if 0 then run 'from main', if non zero then this is the name of a function that we want to run.
 * @param bytecode_file_name bytecode file name
 * @param trace		if not 0 then we are writing a detailed execution trace to standard error stream.
 
 */
V_EXPORT VSCRIPT_STATUS VSCRIPT_VM_run(VSCRIPT_VM_CTX *ctx, const char *source_file, const char *output_directory, int trace );



/**
 * @brief Invoke and run the virtual machine, given a memory mapped file with the executable image.

 * @parma ctx		the script context
 * @param fname		if 0 then run 'from main', if non zero then this is the name of a function that we want to run.
 * @param bytecode_file_name bytecode file name
 * @param trace		if not 0 then we are writing a detailed execution trace to standard error stream.
 
 */
V_EXPORT VSCRIPT_STATUS VSCRIPT_VM_run_with_image(VSCRIPT_VM_CTX *ctx, VOS_MAPFILE mapped_image, int trace );


/**
 * @brief return parameter of 

 * The caller is supposed to service an asynchronous extension method
 * when the VM is suspended by returning VSCRIPT_SUSPENDED_ON_FUNCTION_CALL
 * This function returns parameters and method_id of extension method.
 * see VSCRIPT_VM_add_xmethod_async_method
 */
V_EXPORT int VSCRIPT_VM_async_xmethod_frame(VSCRIPT_VM_CTX *ctx, int *method_id, VSCRIPT_XMETHOD_FRAME **params);


/**
 * @brief pass return value of asynchronous method call
 */
V_EXPORT void VSCRIPT_VM_async_xmethod_return(VSCRIPT_VM_CTX *ctx, void *ret_value );

/** 
 * @brief resume running of VM after completing of servicing a external extension method
 *
 * As the C code finishes servicing of an asynchronous extension method call, the user
 * resumes execution of the runtime and by calling VSCRIPT_VM_resume; here we 
 * are also passing the extension method return values.
 * see VSCRIPT_VM_add_xmethod_async_method
 
 * @parma ctx		the script context
 */
V_EXPORT int VSCRIPT_VM_resume(VSCRIPT_VM_CTX *ctx);

#ifdef  __cplusplus
}
#endif

#endif
