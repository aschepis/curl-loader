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
#include "xlib.h"
#include "ast.h"
#include "sym.h"




typedef struct {
	
	VBUCKETHASH_Entry base;
	union {
		AST_FUNCTION_DECL *decl;
		int xmethod_id;
//		XLIB_DELAYTED_METHOD *xmethod;

	} val;

}  UNIQUE_ID_Entry;


static VBUCKETHASH_VALUE hash_id(void * key, size_t key_length)
{
	V_UNUSED(key_length);
	return * ((int *) key);
}

static int hash_compare_id(VBUCKETHASH_Entry *entry, void * key, size_t key_length)
{
	int key_lhs = * ((int *) key);
	int key_rhs =  ((UNIQUE_ID_Entry *) entry)->val.xmethod_id;

	V_UNUSED(key_length);
	return key_lhs == key_rhs;
}

#if 0
static VBUCKETHASH_VALUE hash_fdecls(void * key, size_t key_length)
{
	AST_FUNCTION_DECL *lhs = (AST_FUNCTION_DECL *) key;

	V_UNUSED(key_length);

	return VHASHFUNCTION_Bob_Jenkins_one_at_a_time( lhs->name, VHASH_STRING); 
}

static int hash_compare_fdecls(VBUCKETHASH_Entry *entry, void * key, size_t key_length)
{
	AST_FUNCTION_DECL *lhs = ((UNIQUE_ID_Entry *) entry)->val.decl;
	AST_FUNCTION_DECL *rhs = (AST_FUNCTION_DECL *) key;

	V_UNUSED(key_length);

	return strcmp(lhs->name, rhs->name) == 0;
//		return AST_compare_signatures(lhs, rhs);

	return -1;
}
#endif




int XMETHODLIB_add(XMETHODLIB *xmethods, struct  tagAST_FUNCTION_DECL *decl, XMETHODACTION action)
{
	UNIQUE_ID_Entry *entry;
	
	//check for uniqueness of user supplied method id.
	entry = (UNIQUE_ID_Entry *) malloc(sizeof(UNIQUE_ID_Entry));
	if (!entry) {
		return XMETHODLIB_METHOD_INTERNAL_ERROR;
	}	
	entry->val.xmethod_id = decl->impl.xmethod_id;	

	if ( VBUCKETHASH_insert( &xmethods->unique_id, &entry->base, &decl->impl.xmethod_id, sizeof(int) ) ) {
		return XMETHODLIB_METHOD_ID_REPEATED;
	}

	// entermethod into map - map name to AST declaration.
	entry = (UNIQUE_ID_Entry *) malloc(sizeof(UNIQUE_ID_Entry));
	if (!entry) {
		return XMETHODLIB_METHOD_INTERNAL_ERROR;
	}
	entry->val.decl = decl;	

	// check if we don't have an identical method already
	if (SYMBOLS_find_function_def( &xmethods->hash, decl, 0) == FUNC_DECL_WITH_SAME_SIGNATURE) {
		return XMETHODLIB_METHOD_ALREADY_DEFINED;
	}

	// define function in local symbol table
	if (SYMBOLS_define( &xmethods->hash, decl->name, &decl->super)) {
		return XMETHODLIB_METHOD_INTERNAL_ERROR;
	}

	// add xmethod implementation to dispatch table
	VARR_push_back( &xmethods->map_id_to_action, &action, sizeof(XMETHODACTION) );

	return 0;

}

struct  tagAST_FUNCTION_DECL *XMETHODLIB_find(XMETHODLIB *xmethods, struct tagAST_BASE *fcall)
{
	UNIQUE_ID_Entry *hentry;
	char *fname;

	if (fcall->type == S_FUN_CALL) {
		fname = ((AST_FUNCTION_CALL *) fcall)->name;
	} else {
		fname = ((AST_FUNCTION_DECL *) fcall)->name;
	}

	hentry = (UNIQUE_ID_Entry *) VBUCKETHASH_find(&xmethods->hash, (void *) fname, VHASH_STRING );
	if (hentry) {
		do {
			if (fcall->type == S_FUN_CALL) {
				if (! AST_compare_fcall((AST_FUNCTION_CALL *) fcall, hentry->val.decl)) {
					return hentry->val.decl;
				} 
			} else {
				if (! AST_compare_signatures( (AST_FUNCTION_DECL *) fcall, hentry->val.decl)) {
					return hentry->val.decl;
				} 
			} 

			hentry = (UNIQUE_ID_Entry *) VBUCKETHASH_find_next( &xmethods->hash, &hentry->base, fname, VHASH_STRING );
		} while( hentry );
	}	
	return 0;
}


XMETHODACTION * XMETHODLIB_lookup_action(XMETHODLIB *xmethods, size_t idx )
{
	return (XMETHODACTION *) VARR_at( &xmethods->map_id_to_action, idx );
}

/* *** external interface *** */

V_EXPORT VSCRIPTXLIB *VSCRIPTXLIB_init()
{
	XMETHODLIB *xmethods;

	xmethods = malloc(sizeof(XMETHODLIB));
	if (!xmethods) {
		return 0;
	}

	if (VARR_init(0,&xmethods->map_id_to_action,sizeof(XMETHODACTION), 10 )) {
		goto err;
	}

	
	if (SYMBOLS_init(&xmethods->hash)) {
		goto err;
	}

	if (VBUCKETHASH_init_uniquemap(					
					0,
					&xmethods->unique_id, 
					10, 
					hash_id,
					hash_compare_id)) {
		goto err;
	}
	
	return (VSCRIPTXLIB *) xmethods;

err:
	free(xmethods);
	return 0;
}


V_EXPORT void VSCRIPTXLIB_free(VSCRIPTXLIB *arg)
{
	XMETHODLIB *xmethods = (XMETHODLIB *) arg;

	VBUCKETHASH_free( &xmethods->hash );
	VBUCKETHASH_free( &xmethods->unique_id );

	free(xmethods);
}


static int VSCRIPTXLIB_add_xmethod_action(XMETHODLIB *ctx, int method_id, const char *func_name, const char *prototype, XMETHODACTION action)
{
	AST_FUNCTION_DECL *fdecl;

	V_UNUSED(method_id); //TODO: ???

	// parse function declaration	
	fdecl = AST_make_xmethod_decl( func_name, prototype);
	if (!fdecl) {
		return VSCRIPT_STATUS_XMETHOD_WRONG_PARAMETER_SPEC;
	}

	fdecl->impl.xmethod_id = VARR_size( &ctx->map_id_to_action );

	// add xmethod to symbol table(s)
	switch(XMETHODLIB_add( ctx, fdecl, action )) {
	case XMETHODLIB_METHOD_INTERNAL_ERROR:
		return VSCRIPT_STATUS_INTERNAL_ERROR;

	case XMETHODLIB_METHOD_ID_REPEATED:
		return 	VSCRIPT_STATUS_XMETHOD_METHOD_ID_REPEATED;

	case XMETHODLIB_METHOD_ALREADY_DEFINED:
		return VSCRIPT_STATUS_XMETHOD_METHOD_ALREADY_DEFINED;

	default:
		return 0;
	}
}

V_EXPORT int VSCRIPTXLIB_add_xmethod_callback(VSCRIPTXLIB *ctx, int method_id, const char *func_name, const char *prototype, VSCRIPT_VM_XMETHOD_CALLBACK callback )
{
	XMETHODACTION action;

	action.action_type = XMETHOD_CALLBACK;
	action.method_id = method_id;
	action.callback_ptr = callback;

	return VSCRIPTXLIB_add_xmethod_action( (XMETHODLIB *) ctx, method_id, func_name, prototype, action);
}


V_EXPORT int VSCRIPTXLIB_add_xmethod_async(VSCRIPTXLIB *ctx, 
									int method_id, 
									const char *func_name,
									const char *prototype
									)
{
	XMETHODACTION action;

	action.action_type = XMETHOD_ASYNCH;
	action.method_id = method_id;
	action.callback_ptr = 0;

	return VSCRIPTXLIB_add_xmethod_action( (XMETHODLIB *) ctx, method_id, func_name, prototype, action);
}

V_EXPORT int VSCRIPTXLIB_add_xmethod_nomethod(VSCRIPTXLIB *ctx, 
									int method_id, 
									const char *func_name,
									const char *prototype
									)
{
	XMETHODACTION action;

	action.action_type = XMETHOD_NOTIMPLEMENTED;
	
	return VSCRIPTXLIB_add_xmethod_action( (XMETHODLIB *) ctx, method_id, func_name, prototype, action);
}

#define VALID_VSCRIPTXLIB_METHOD_TABLE_ENTRY(tbl) \
	((tbl)->method_id != -1 && \
	 (tbl)->method_id && \
	 (tbl)->prototype_spec)

V_EXPORT int VSCRIPTXLIB_add_xmethod_async_table(VSCRIPTXLIB *ctx, 
									VSCRIPTXLIB_METHOD_TABLE *tbl
									)
{
	for( ; VALID_VSCRIPTXLIB_METHOD_TABLE_ENTRY(tbl); tbl++) {
		if (VSCRIPTXLIB_add_xmethod_async(ctx, 
			tbl->method_id,
			tbl->func_name,
			tbl->prototype_spec)) {
			return -1;
		}
	
	}
	return 0;
}

V_EXPORT int VSCRIPTXLIB_add_xmethod_nomethod_table(VSCRIPTXLIB *ctx, 
									VSCRIPTXLIB_METHOD_TABLE *tbl
									)
{
	for( ; VALID_VSCRIPTXLIB_METHOD_TABLE_ENTRY(tbl); tbl++) {
		if (VSCRIPTXLIB_add_xmethod_nomethod(ctx, 
			tbl->method_id,
			tbl->func_name,
			tbl->prototype_spec)) {
			return -1;
		}
	
	}
	return 0;
}


#define VALID_VSCRIPTXLIB_METHOD_CALLBACK_TABLE_ENTRY(tbl) \
	((tbl)->method_id != -1 && \
	 (tbl)->method_id && \
	 (tbl)->prototype_spec)

V_EXPORT int VSCRIPTXLIB_add_xmethod_callback_table(VSCRIPTXLIB *ctx, 
									 VSCRIPTXLIB_METHOD_CALLBACK_TABLE *tbl
									 )
{
	for(;VALID_VSCRIPTXLIB_METHOD_CALLBACK_TABLE_ENTRY(tbl);tbl++) {
		
		if (VSCRIPTXLIB_add_xmethod_callback(ctx, 
			tbl->method_id,
			tbl->func_name,
			tbl->prototype_spec,
			tbl->callback_impl)) {
			return -1;
		}
	}
	return 0;
}






