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
#include "syntax.h"
#include "asm.h"
#include "exe.h"
#include "sym.h"
#include "vm.h"
#include <limits.h>


/* *** script compiler facade *** */

V_EXPORT VSCRIPT_CL_CTX *VSCRIPT_CL_init(VCONTEXT *ctx)
{
	VSCRIPTCTX * ret = malloc( sizeof(VSCRIPTCTX) );
	if (!ret) {
		return 0;
	}

	memset(ret, 0, sizeof(VSCRIPTCTX) );

	if (VSCRIPTCTX_init( ret, ctx )) {
		free(ret);
		return 0;
	}

	return (VSCRIPT_CL_CTX *) ret;
}


V_EXPORT void VSCRIPT_CL_close(VSCRIPT_CL_CTX *ctx)
{
	VSCRIPTCTX *impl = (VSCRIPTCTX *) ctx;

	if (impl->output_dir) {
		free(impl->output_dir);
	}

	VSCRIPTCTX_free( (VSCRIPTCTX *) ctx );
	
	free(ctx);
}



V_EXPORT void VSCRIPT_CL_set_debug_options(VSCRIPT_CL_CTX *cctx,  int options)
{
	VSCRIPTCTX *ctx = (VSCRIPTCTX *) cctx;
	
		
	ctx->trace_opts  = options;

}


V_EXPORT void VSCRIPT_CL_set_output_dir(VSCRIPT_CL_CTX *cctx, const char *output_directory )
{
	VSCRIPTCTX *ctx = (VSCRIPTCTX *) cctx;

	ctx->output_dir = output_directory ? strdup( output_directory ) : 0;
}


typedef enum {
  TRACE_FILE_AST,
  TRACE_FILE_LISTING,
  TRACE_FILE_LISTING_RESOLVED,
  TRACE_FILE_OUTPUT_FILE,

} TRACE_FILES;


static void make_file_name(char *fname, 
						   const char * source_file, 
						   const char * output_dir,
						   TRACE_FILES kind_of_trace)
{
	char *ext = "";
	const char *base;

	base = strrchr(source_file,'/');
	if (!base)  {
		base = strrchr(source_file,'\\');
		if (!base) {
			base = source_file;
		} else {
			base += 1;
		}
	} else {
		base += 1;
	}

	switch( kind_of_trace ) {
	case TRACE_FILE_AST:
		ext = ".xml";
		break;
	case TRACE_FILE_LISTING:
		ext = ".lst";
		break;
	case TRACE_FILE_LISTING_RESOLVED:
		ext = ".lst2";
		break;
	case TRACE_FILE_OUTPUT_FILE:
		ext = ".vout";
		break;
	}

	sprintf(fname, "%s%s%s%s", 
				output_dir,
#ifdef _WIN32
				"\\"
#else
				"/"
#endif			
				,
				base,
				ext);
}

#ifdef _WIN32
#define PATH_MAX _MAX_PATH
#endif

static FILE *open_trace_file( const char * source_file, TRACE_FILES kind_of_trace)
{
	char trace_file[ PATH_MAX ];

	make_file_name(trace_file, 
				   source_file, 
				   vscript_ctx->output_dir ? vscript_ctx->output_dir : "", 
				   kind_of_trace );

	return fopen( trace_file, kind_of_trace == TRACE_FILE_OUTPUT_FILE ? "wb" : "w" );

}

V_EXPORT VSCRIPT_STATUS VSCRIPT_CL_compile_to_bytecode(VSCRIPT_CL_CTX *ctx, const char *source_file, VSCRIPTXLIB *extlib)
{
	VSCRIPT_STATUS ret;
	FILE *fp;
	int rt;
	AST_BASE *ast;

	vscript_ctx = (VSCRIPTCTX *) ctx;
	vscript_ctx->extension_library = (struct tagXMETHODLIB *) extlib;

	if ( MY_YY_open_file( source_file ) ) {
		fprintf( stderr, "Can't open file %s\n", source_file );
		return VSCRIPT_STATUS_CL_NO_INPUT_ERROR;
	}

	/* ********************************************** *
				parsing
	 * ********************************************** */
	ret = MY_YY_parse(&ast);

	switch(ret) {
		case MY_YY_PARSE_STATUS_OK: 
			{
			if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
				fprintf(stdout,"*** syntax ok ***\n");
			}

			if (! VTREE_check_tree( &vscript_ctx->my_ast_root->node )) {
			  if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
					fprintf(stdout,"*** ERROR: parse tree is not ok ***\n");
			  }
			  return VSCRIPT_STATUS_INTERNAL_ERROR;
			}

			if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_DUMP_AST) {

				FILE *fp = open_trace_file(source_file, TRACE_FILE_AST);
				if (fp) {			
					MY_YY_dump_as_xml( fp, vscript_ctx->my_ast_root );
					fclose(fp);
				}
			}
			}
			break;
		
		case MY_YY_PARSE_STATUS_SYNTAX_ERROR:
			if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
				fprintf(stderr,"*** ERROR: some kind of syntax error ***\n");
			}
			return VSCRIPT_STATUS_CL_SYNTAX_ERROR;

		default:
			if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
				fprintf(stderr,"*** internal parser error (%d) ***\n",ret);
			}
			return VSCRIPT_STATUS_INTERNAL_ERROR;
	}

	/* ********************************************** *
				semantics check / code gen
	 * ********************************************** */
	if (!MY_YY_code_gen( vscript_ctx->my_ast_root )) {		
		if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
			fprintf(stdout,"*** semantics and code generation ok ***\n");
		}
	} else {		
		if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
			fprintf(stderr,"*** ERROR semantics or code generation ***\n");
		}
		return VSCRIPT_STATUS_CL_SEMANTIC_OR_CODEGEN_ERROR;
	}

	if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_DUMP_LISTING) {

		FILE *fp = open_trace_file( source_file, TRACE_FILE_LISTING);
		if (fp) {			
			ASM_CONSTANT_POOL_dump(fp, vscript_ctx);
			ASM_write_listing(fp, vscript_ctx->my_code );
			fclose(fp);
		}
	}

	/* ********************************************** *
			link back references in opcodes
	 * ********************************************** */
	//resolution of back references (now that all locations are set) 
	if (!ASM_resolve_backrefs( vscript_ctx->my_code )) {
		if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
			fprintf(stderr,"*** resolving back references ok ***\n");
		}

	} else {
		if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
			fprintf(stderr,"*** ERROR resolving back references ***\n");
		}
		return VSCRIPT_STATUS_CL_BACKREF_ERROR;
	}


	if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_DUMP_LISTING_LINKED) {

		FILE *fp = open_trace_file(source_file, TRACE_FILE_LISTING_RESOLVED);
		if (fp) {			
			ASM_CONSTANT_POOL_dump(fp, vscript_ctx);
			ASM_write_listing(fp, vscript_ctx->my_code );
			fclose(fp);
		}
	}

	/* ********************************************** *
			write executable file
	 * ********************************************** */

	fp = open_trace_file( source_file, TRACE_FILE_OUTPUT_FILE);
	if (!fp) {
		return VSCRIPT_STATUS_CL_WRITE_EXE_ERROR;
	}
	rt = EXE_write_exe_file( fp, vscript_ctx );
	fclose(fp);
	if (!rt) {
		if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
			fprintf(stderr,"*** write exe file ok ***\n");
		}
	} else {
		if (vscript_ctx->trace_opts & VSCRIPT_CL_DEBUG_TRACE) {
			fprintf(stderr,"*** ERROR write exe file ***\n");
		}
		return VSCRIPT_STATUS_CL_WRITE_EXE_ERROR;
	}

	return VSCRIPT_STATUS_OK;
}


/* *** virtual machine stuff *** */


V_EXPORT VSCRIPT_VM_CTX *VSCRIPT_VM_init(VCONTEXT *ctx, VSCRIPTXLIB *xlib)
{	
	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	return (VSCRIPT_VM_CTX *) VSCRIPTVM_init(ctx, (XMETHODLIB *) xlib );

}

V_EXPORT void VSCRIPT_VM_close(VSCRIPT_VM_CTX *ctx)
{
	VSCRIPTVM_close( (VSCRIPTVM *) ctx);
}

V_EXPORT VSCRIPT_STATUS VSCRIPT_VM_run(VSCRIPT_VM_CTX *ctx, const char *source_file, const char *output_directory, int trace )
{
	VSCRIPTVM *vm = (VSCRIPTVM *) ctx;
	VSCRIPTVM_STATUS ret;
	char bytecode_file[ PATH_MAX ];

	make_file_name(bytecode_file, source_file, output_directory, TRACE_FILE_OUTPUT_FILE);

	ret = VSCRIPTVM_open( vm, bytecode_file, trace);
	if (ret != VSCRIPT_STATUS_OK) {
		return ret;
	}

	return VSCRIPTVM_run(vm);
}

V_EXPORT VSCRIPT_STATUS  VSCRIPT_VM_run_with_image(VSCRIPT_VM_CTX *ctx, VOS_MAPFILE mapped_image, int trace )
{
	VSCRIPTVM *vm = (VSCRIPTVM *) ctx;
	VSCRIPTVM_STATUS ret;

	ret = VSCRIPTVM_open_from_image(vm, mapped_image, trace);
	if (ret != VSCRIPT_STATUS_OK) {
		return ret;
	}

	return VSCRIPTVM_run(vm);
}


V_EXPORT int VSCRIPT_VM_async_xmethod_frame(VSCRIPT_VM_CTX *ctx, int *method_id, VSCRIPT_XMETHOD_FRAME **params)
{
	VSCRIPTVM *vm = (VSCRIPTVM *) ctx;

	*method_id = vm->thread->method_id;
	*params = &vm->thread->params;
	return 0;
}

V_EXPORT void VSCRIPT_VM_async_xmethod_return(VSCRIPT_VM_CTX *ctx, void *ret_value)
{
	VSCRIPTVM *vm = (VSCRIPTVM *) ctx;

	vm->thread->params.retval = ret_value;
}

V_EXPORT int VSCRIPT_VM_resume(VSCRIPT_VM_CTX *ctx)
{
	VSCRIPTVM *vm = (VSCRIPTVM *) ctx;

	return VSCRIPTVM_run(vm);
}
