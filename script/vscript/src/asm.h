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
#ifndef _ASM_H_
#define _ASM_H_

#include <stdio.h>
#include <util/vslist.h>
#include <util/vbuckethash.h>


struct tagVSCRIPTCTX; 

/* 
	layout of memory map in intermedeate representation
	location

	0 ... <top_of_stack_on_scope>	stack variables

	<top_of_stack_on_scope+1>  ,,, ASM_MAX_GLOBAL_VAR	global variables

	ASM_MAX_GLOBAL_VAR+1 ... ASM_MAX_CONSTANT       constants

 */

typedef unsigned long ASM_STORAGE_LOCATION;

typedef unsigned long ASM_PROGRAM_COUNTER;


#define ASM_MAX_CONSTANT  0xFFFFFFFF

#define ASM_MAX_GLOBAL_VAR 0x1FFFFFFF

#define ASM_PC(x) (ASM_PROGRAM_COUNTER) (x)

#define ASM_IS_CONSTANT(x) \
	(ASM_PC(x) > ASM_MAX_GLOBAL_VAR)

#define ASM_IS_STACK(x, max_global) \
	(ASM_PC(x) < (ASM_MAX_GLOBAL_VAR - ASM_PC(max_global)))

/* *** pool of constant during compilation/code generatoin **** */

typedef struct tagASM_CONSTANT_ENTRY {
  VBUCKETHASH_Entry entry;
  struct tagAST_EXPRESSION *value; // key
  ASM_STORAGE_LOCATION location;   // value

  VSLIST_entry next; // keeps entries sorted by storage location.

} ASM_CONSTANT_ENTRY;


int	ASM_CONSTANT_POOL_init(struct tagVSCRIPTCTX *ctx);
void ASM_CONSTANT_POOL_free(struct tagVSCRIPTCTX *ctx);

ASM_STORAGE_LOCATION ASM_CONSTANT_POOL_location(struct tagVSCRIPTCTX *ctx, struct tagAST_EXPRESSION * expr);

ASM_STORAGE_LOCATION ASM_CONSTANT_POOL_first_id(struct tagVSCRIPTCTX *ctx);

size_t	ASM_CONSTANT_POOL_size( struct tagVSCRIPTCTX *ctx );		


/* *** enumeration of all opcodes *** */
typedef enum {
		/* label definition */
	ASM_LABEL, 
	
	/* goto */
	ASM_GOTO,

	/* move variable or constant from source location to target location (addref) */
	ASM_MOVE,

	/* copy variables or constant from source to target location  */
	ASM_CPY,

	/* call method in local code segment, i.e. bytecode segment for this source module */
	ASM_CALL,
	
	/* call method in external (i.e. non local code segment) library */
	ASM_XCALL,

	/* return from current function */
	ASM_RETURN,

	/* variable definition */
	ASM_VARDEF,

	/* variable undefine */
	ASM_VARUNDEF,

	/* jump if location is zero */
	ASM_JZ,

	/* jump if location is not zero */
	ASM_JNZ,

	/* lookup in dynamic array */
	ASM_ARRAY_LOOKUP,

	/* lookup in hash */
	ASM_HASH_LOOKUP,

	/* modify location in dynamic array */
	ASM_ARRAY_SET,

	/* modify location in dynamic array */
	ASM_HASH_SET,

	/* unary operators */
	ASM_OP_NUM_NEGATE,

	/* binary operators */
	ASM_OP_NUM_EQ,
	ASM_OP_NUM_NE,
	ASM_OP_NUM_LT,
	ASM_OP_NUM_LE,
	
	ASM_OP_NUM_ADD,
	ASM_OP_NUM_SUBST,
	ASM_OP_NUM_DIV,
	ASM_OP_NUM_MULT,
	ASM_OP_NUM_MOD,

	ASM_OP_STR_EQ,
	ASM_OP_STR_NE,
	ASM_OP_STR_LT,
	ASM_OP_STR_LE,
	ASM_OP_STR_CAT,
	ASM_OP_STR_REGEXMATCH,

	ASM_FORWARD_DECL_AST_RES = 0x01000000,
	ASM_FORWARD_DECL_ASM_RES = 0x02000000,
}
	ASM_TYPE;

const char *ASM_TYPE_get_sym_name( ASM_TYPE instr );

#define ASM_FORWARD_DECL_MASK	(ASM_FORWARD_DECL_AST_RES | ASM_FORWARD_DECL_ASM_RES)

typedef union {
	ASM_STORAGE_LOCATION arg;
	struct tagAST_BASE *forward_decl_ast;
	struct tagASM_INSTRUCTION *forward_decl_asm;
} ASM_PARAM;


/* *** an assembly instruction - represented in intermedeate code *** */
typedef struct tagASM_INSTRUCTION {
	VSLIST_entry entry;

	ASM_TYPE type;
	ASM_PROGRAM_COUNTER pc;

	ASM_PARAM res;
	ASM_PARAM lhs;
	ASM_PARAM rhs;

#ifdef TRACK_LISTING
	struct tagAST_BASE *ast_entry;
#endif

}   ASM_INSTRUCTION;

V_INLINE int ASM_INSTRUCTION_get_opcode(ASM_INSTRUCTION *instr)
{
	return instr->type & 0xFF;
}



ASM_INSTRUCTION *ASM_INSTRUCTION_OP3(
						ASM_TYPE arg,
						ASM_STORAGE_LOCATION res,
						ASM_STORAGE_LOCATION lhs, 
						ASM_STORAGE_LOCATION rhs);

ASM_INSTRUCTION *ASM_INSTRUCTION_PC(ASM_TYPE arg,ASM_PROGRAM_COUNTER pc);

ASM_INSTRUCTION *ASM_INSTRUCTION_FORWARD_DECL_ASM(ASM_TYPE arg,ASM_INSTRUCTION *fdecl);

ASM_INSTRUCTION *ASM_INSTRUCTION_FORWARD_DECL_AST(ASM_TYPE arg,struct tagAST_BASE *fdecl, ASM_PROGRAM_COUNTER count);

ASM_INSTRUCTION *ASM_INSTRUCTION_COND_JUMP_ASM(ASM_TYPE arg, ASM_STORAGE_LOCATION cond, ASM_INSTRUCTION *fdecl);

V_INLINE void ASM_INSTRUCTION_set_forwarded(ASM_INSTRUCTION *instr,ASM_PROGRAM_COUNTER  pc)
{
	instr->res.arg = pc;
	instr->type = instr->type & 0xFF;
}

void ASM_CONSTANT_POOL_dump(FILE *fp, struct tagVSCRIPTCTX *ctx);

void ASM_write_listing(FILE *fp, VSLIST *code);

/* *** attribute for code generation; each AST node  has this structure prepended *** */
typedef struct tagAST_ASM_ATTRIBUTE {
	VSLIST code;
	ASM_STORAGE_LOCATION storage;
	//ASM_INSTRUCTION *label;

}  AST_ASM_ATTRIBUTE;

V_INLINE void AST_ASM_ATTRIBUTE_init( AST_ASM_ATTRIBUTE *attr )
{
	VSLIST_init(&attr->code);
	attr->storage = 0;
	//attr->label = 0;
}


V_INLINE AST_ASM_ATTRIBUTE * get_code( const void *ptr )
{
	return (AST_ASM_ATTRIBUTE *) ((unsigned char *) ptr - sizeof(AST_ASM_ATTRIBUTE));
}

V_INLINE int has_code( const void *ptr )
{
	AST_ASM_ATTRIBUTE *attr = get_code( ptr );
	return ! VSLIST_isempty( &attr->code );
}

V_INLINE ASM_INSTRUCTION  *get_first_instruction(const void *ptr)
{
	return (ASM_INSTRUCTION  *) VSLIST_get_first( &get_code(ptr)->code );
}

V_INLINE ASM_INSTRUCTION  *get_last_instruction(const void *ptr)
{
	return (ASM_INSTRUCTION  *) VSLIST_get_last( &get_code(ptr)->code );
}

V_INLINE void ASM_MERGE_CODE(void *lhs, void *rhs )
{
	AST_ASM_ATTRIBUTE *llhs =  get_code( lhs );
	AST_ASM_ATTRIBUTE *rrhs =  get_code( rhs );

	VSLIST_append( &llhs->code, &rrhs->code );
}

V_INLINE void ASM_ADD( void *lhs, ASM_INSTRUCTION *inst)
{
	VSLIST_push_back( &get_code(lhs)->code, &inst->entry );
#ifdef TRACK_LISTING
	inst->ast_entry = (struct tagAST_BASE *) lhs;
#endif
}

int get_asm_op(struct tagAST_EXPRESSION *expr);


#endif
