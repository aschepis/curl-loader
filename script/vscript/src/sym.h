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
#ifndef _SYM_H_
#define _SYM_H_

#include <util/vbuckethash.h>
#include <util/varr.h>

struct tagAST_FUNCTION_CALL;
struct tagAST_FUNCTION_DECL; 

typedef unsigned long SYM_LOCATION;

typedef struct tagSTR2AST
{
	VBUCKETHASH_Entry entry;
	const char *name;
	struct tagAST_BASE *ast;
}
	STR2AST;

/* *** symbol hash  *** */

typedef enum {
	FUNC_DECL_WITH_SAME_SIGNATURE,
	FUNC_DECL_WITH_DIFF_SIGNATURE,
	NOT_FUNC_DECL,
	NOTHING_DECL
} 
    SYM_SCOPE_RET;

int SYMBOLS_init(VBUCKETHASH  *hash);

int SYMBOLS_define_function(VBUCKETHASH  *hash, struct tagAST_FUNCTION_DECL *fdecl);

SYM_SCOPE_RET SYMBOLS_find_function_call(VBUCKETHASH  *hash, struct tagAST_FUNCTION_CALL *fcall, struct tagAST_FUNCTION_DECL **found, struct tagAST_BASE **err );

SYM_SCOPE_RET SYMBOLS_find_function_def(VBUCKETHASH  *hash, struct tagAST_FUNCTION_DECL *fdef, struct tagAST_BASE **err);

int SYMBOLS_define(VBUCKETHASH  *hash, const char *key, struct tagAST_BASE *entry);


/* *** symbol table  *** */


typedef struct tagSYM_SCOPE {
  struct tagSYM_SCOPE *parent;
  struct tagAST_FUNCTION_DECL *fdecl;
  int next_storage;
  int can_reuse_count;
  VBUCKETHASH hash;
  VARR resuse_temp_array;
} SYM_SCOPE;


int SYM_SCOPE_push();
int SYM_SCOPE_pop();
int SYM_SCOPE_is_global_scope();


int SYM_SCOPE_undef(SYM_SCOPE *scope, const char *item);
int SYM_SCOPE_define(struct tagAST_BASE *entry);
int SYM_SCOPE_is_defined(struct tagAST_BASE *entry);
int SYM_SCOPE_is_defined_ex(struct tagAST_BASE *entry, struct tagAST_BASE ** find_entry, SYM_SCOPE **find_scope);

struct tagSTR2AST *SYM_SCOPE_find(SYM_SCOPE *scope, const char *item);

struct tagSTR2AST *SYM_SCOPE_find_ex(SYM_SCOPE *scope, const char *item, SYM_SCOPE **find_scope);


SYM_SCOPE_RET SYM_SCOPE_find_function_call( struct tagAST_FUNCTION_CALL *fcall, struct tagAST_FUNCTION_DECL **found, struct tagAST_BASE **err );
SYM_SCOPE_RET SYM_SCOPE_find_function_def(struct tagAST_FUNCTION_DECL *fdef, struct tagAST_BASE **err);


/* *** reuse of variable locations *** */
SYM_LOCATION SYM_SCOPE_make_new_location(SYM_SCOPE *scope);
SYM_LOCATION SYM_SCOPE_make_new_location_reusable(SYM_SCOPE *scope);
int SYM_SCOPE_release_location(SYM_SCOPE *scope, SYM_LOCATION location);
int SYM_SCOPE_is_location_on_stack( SYM_SCOPE *global_scope, SYM_LOCATION location);

V_INLINE size_t SYM_SCOPE_count_vars( SYM_SCOPE *sc) 
{
	return sc->next_storage;
}
#endif
