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
#include "asm.h"
#include "syntax.h"
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "scr.tab.h"




typedef struct {
	unsigned long low;
	unsigned long high;
} LOW_HIGH;

/*
  Function name - hash_value
 
  Description - callback function, computes hash value of constant expression
				the key of constant table is the value of the content
				constant expression can be of several types, so we have to handle cases...
 
  Input -       
 
  Return Code/Output 
 */
static VBUCKETHASH_VALUE hash_value (void * key, size_t key_length)
{
	AST_EXPRESSION *lhs = (AST_EXPRESSION *) key;

	V_UNUSED(key_length);

	if (lhs->exp_type == S_EXPR_INT_CONST) {
		return lhs->val.int_value;
	} else if (lhs->exp_type == S_EXPR_DOUBLE_CONST) {
		LOW_HIGH *ptr = (LOW_HIGH *) &lhs->val.double_value;
		return ptr->low | ptr->high;
	} else if (lhs->exp_type == S_EXPR_STRING_CONST) {
		return VHASHFUNCTION_Bob_Jenkins( lhs->val.string_value, VHASH_STRING ); 
	}
	return 0;
}


/*
  Function name - hash_compare
 
  Description - compare two keys for entries in constant table,
				the key of constant table is the value of the content
 
  Input -       
 
  Return Code/Output 
 */
static int hash_compare(VBUCKETHASH_Entry *entry, void * key, size_t key_length)
{
	AST_EXPRESSION *lhs = ((ASM_CONSTANT_ENTRY *) entry)->value;
	AST_EXPRESSION *rhs = (AST_EXPRESSION *) key;

	V_UNUSED(key_length);

	if (lhs->exp_type != rhs->exp_type) {
		return -1;
	}

	if (lhs->exp_type == S_EXPR_INT_CONST) {
		return lhs->val.int_value != rhs->val.int_value;
	} else if (lhs->exp_type == S_EXPR_DOUBLE_CONST) {
		return lhs->val.double_value != rhs->val.double_value;
	} else if (lhs->exp_type == S_EXPR_STRING_CONST) {
		return strcmp( lhs->val.string_value, rhs->val.string_value ) != 0; 
	}

	return -1;
}

/* *** constant pool *** */

/*
  Function name - ASM_CONSTANT_POOL_init
 
  Description - we don' want to duplicate constants, so keep them all in a pool 
				where the key is the value of the constant, and vlaue is the storage location
				in initialised data segment, where this constant is stored.

				this function initalises the constant pool
 
  Input -       
 
  Return Code/Output 
 */
int	ASM_CONSTANT_POOL_init(struct tagVSCRIPTCTX *ctx)
{
	ctx->next_constant = 0;

	if (VBUCKETHASH_init_uniquemap(					
					0,
					&ctx->constant_pool, 
					10, 
					hash_value ,
					hash_compare)) {
		return -1;
	}
	
	VSLIST_init( &ctx->constants );	

	return 0;
}

/*
  Function name - ASM_CONSTANT_POOL_free
 
  Description - free constant pool
 
  Input -       
 
  Return Code/Output 
 */
void	ASM_CONSTANT_POOL_free(struct tagVSCRIPTCTX *ctx)
{
	VBUCKETHASH_free(&ctx->constant_pool);
}

/*
  Function name - ASM_CONSTANT_POOL_init
 
  Description - we don' want to duplicate constants, so keep them all in a pool 
				where the key is the value of the constant, and vlaue is the storage location
				in initialised data segment, where this constant is stored.

				this function returns the address for a constant expression,
				it first looks it up in constant (hash) table
				if it is not found, a new entry is created and an address is assigned to the constant,
				
				later when executable file is created, we put in the constant to its partiular address.
 
  Input -       
 
  Return Code/Output 
 */

ASM_STORAGE_LOCATION ASM_CONSTANT_POOL_location(struct tagVSCRIPTCTX *ctx, struct tagAST_EXPRESSION * expr)
{
	ASM_CONSTANT_ENTRY *hentry;
	ASM_CONSTANT_ENTRY *new_entry;
	
	hentry = (ASM_CONSTANT_ENTRY  *) VBUCKETHASH_find(&ctx->constant_pool, (void *) expr, VHASH_STRING );
	if (hentry) {
		return ASM_MAX_CONSTANT - hentry->location;
	}

	new_entry = (ASM_CONSTANT_ENTRY *) VSCRIPTCTX_malloc(sizeof(ASM_CONSTANT_ENTRY));

	new_entry->value = expr;
	new_entry->location = ctx->next_constant ++;
	
	if (VBUCKETHASH_insert( &ctx->constant_pool, &new_entry->entry, expr, VHASH_STRING )) {
		return -1;
	}

	VSLIST_push_front( &ctx->constants,  &new_entry->next );

	return ASM_MAX_CONSTANT - new_entry->location;
}

size_t	ASM_CONSTANT_POOL_size( struct tagVSCRIPTCTX *ctx )
{		
	return VBUCKETHASH_size( &ctx->constant_pool );
}

ASM_STORAGE_LOCATION ASM_CONSTANT_POOL_first_id(struct tagVSCRIPTCTX *ctx)
{
	return ASM_MAX_CONSTANT - VBUCKETHASH_size( &ctx->constant_pool );
}



/*
  Function name - ASM_CONSTANT_POOL_dump
 
  Description - dumps contents of constant pool to argument file (argument fp)
				is used as part of creting assembly listing
  Input -       
 
  Return Code/Output 
 */
void ASM_CONSTANT_POOL_dump(FILE *fp, struct tagVSCRIPTCTX *ctx)
{
	VBUCKETHASH_Entry *cur;
 
	VBUCKETHASH_FOREACH(cur, &(ctx->constant_pool) )

		ASM_CONSTANT_ENTRY *entry = (ASM_CONSTANT_ENTRY *) cur;

		fprintf( fp, "constant %lX : ", ASM_MAX_CONSTANT - entry->location);
		AST_print_expression( &entry->value->super, fp );
		fprintf( fp, "\n");

	VBUCKETHASH_FOREACH_END

	fprintf( fp, "\n***\n\n" );
}


/* *** dump of listing file from sequence of instructions *** */

typedef void (*ASM_PRINT_INSTRUCTION) (FILE *fp, ASM_INSTRUCTION *node);


typedef struct {
  const char *symname;
  int  val;
  ASM_PRINT_INSTRUCTION print;

} ASM_INSTRUCTION_INFO;


static void print_param(FILE *fp,ASM_PARAM *param,unsigned int flags, int param_num )
{
	unsigned int rflags = flags >> (24 + 2 * (param_num - 1));

	if (rflags & 1) {
		ASM_INSTRUCTION *inst;
		
		inst = get_first_instruction( param->forward_decl_ast );
		
		fprintf(fp,"{ast: ");
		AST_BASE_show_node(fp, param->forward_decl_ast, 1 );
		fprintf(fp,"}");
		
		if (inst) {
			fprintf(fp,"{asm: pc=0x%lX obj=%p}", inst->pc,inst);
		}
		
	} else  if (rflags & 2) {
		fprintf(fp,"{asm: pc=0x%lX obj=%p}", param->forward_decl_asm->pc,param->forward_decl_asm);

	
	} else {
		fprintf(fp, "0x%lX", param->arg);
	}
}

/* *** the following function each prints arguments of some specific assembly instruction *** */

static void param_res(FILE *fp, ASM_INSTRUCTION *node)
{
	print_param(fp, &node->res, node->type, 1);
}

static void param_res_lhs(FILE *fp, ASM_INSTRUCTION *node)
{
	print_param(fp, &node->res, node->type, 1);
	
	fprintf(fp, " , ");

	print_param(fp, &node->lhs, node->type, 2);
}

static void print_vardef(FILE *fp, ASM_INSTRUCTION *node)
{
	fprintf(fp, "type=%s", AST_BASE_get_var_type_name( node->res.arg ) );

	fprintf(fp, " , location=");

	print_param(fp, &node->lhs, node->type, 2);
}

static void print_indexed_get(FILE *fp, ASM_INSTRUCTION *node)
{
	fprintf(fp, "ret=" );

	print_param(fp, &node->res, node->type, 1);

	fprintf(fp, " := , collection=" );

	print_param(fp, &node->lhs, node->type, 2);

	fprintf(fp, " , [ index_expr=");

	print_param(fp, &node->rhs, node->type, 3);

	fprintf(fp, " ] ");
}

static void print_indexed_set(FILE *fp, ASM_INSTRUCTION *node)
{

	fprintf(fp, "collection=" );

	print_param(fp, &node->lhs, node->type, 2);

	fprintf(fp, " , [ index_expr=");

	print_param(fp, &node->rhs, node->type, 3);


	fprintf(fp, " ] , := rhs=" );

	print_param(fp, &node->res, node->type, 1);

}

static void print_label(FILE *fp, ASM_INSTRUCTION *node)
{
	fprintf(fp, "#%p",node);
}

static void param_res_lhs_rhs(FILE *fp, ASM_INSTRUCTION *node)
{
	print_param(fp, &node->res, node->type, 1);
	
	fprintf(fp, " , ");

	print_param(fp, &node->lhs, node->type, 2);

	fprintf(fp, " , ");

	print_param(fp, &node->rhs, node->type, 3);
}


static ASM_INSTRUCTION_INFO info[] = {
	/* label definition */
	{ "label",	ASM_LABEL, print_label },
	
	/* goto */
	{ "jmp",	ASM_GOTO, param_res },

	/* move variable or constant from source location to target location (addref) */
	{ "mov",	ASM_MOVE, param_res_lhs },

	/* copy variable or constant from source location to target location (new instance) */
	{ "cpy",	ASM_CPY,  param_res_lhs },

	/* call method in local code segment, i.e. bytecode segment for this source module */
	{ "call",	ASM_CALL, param_res_lhs },
	
	/* call method in external (i.e. non local code segment) library */
	{ "xcall",	ASM_XCALL, param_res_lhs },

	/* return from current function */
	{ "ret",	ASM_RETURN, param_res_lhs },

	/* variable definition */
	{ "vardef",	ASM_VARDEF, print_vardef},

	/* variable undefine */
	{ "varundef", ASM_VARUNDEF, param_res },

	/* jump if location is zero */
	{ "jz",		ASM_JZ, param_res_lhs },

	/* jump if location is not zero */
	{ "jnz",	ASM_JNZ, param_res_lhs },

	/* lookup in dynamic array */
	{ "arrget", ASM_ARRAY_LOOKUP, print_indexed_get },

	/* lookup in hash */
	{ "hashget", ASM_HASH_LOOKUP, print_indexed_get },

	/* modify location in dynamic array */
	{ "arrset", ASM_ARRAY_SET, print_indexed_set },

	/* modify location in dynamic array */
	{ "hashset",	ASM_HASH_SET, print_indexed_set },

	/* unary operators */
	{ "uminus",		ASM_OP_NUM_NEGATE, 0 },

	/* binary operators */
	{ "eqnum",	ASM_OP_NUM_EQ, param_res_lhs_rhs },
	{ "nenum",	ASM_OP_NUM_NE, param_res_lhs_rhs },
	{ "ltnum",	ASM_OP_NUM_LT, param_res_lhs_rhs },
	{ "lenum",	ASM_OP_NUM_LE, param_res_lhs_rhs },
	
	{ "addnum",	ASM_OP_NUM_ADD, param_res_lhs_rhs },
	{ "subnum",	ASM_OP_NUM_SUBST, param_res_lhs_rhs },
	{ "divnum",	ASM_OP_NUM_DIV, param_res_lhs_rhs },
	{ "mulnum",	ASM_OP_NUM_MULT, param_res_lhs_rhs },
	{ "modnum",	ASM_OP_NUM_MOD, param_res_lhs_rhs },

	{ "eqstr",	ASM_OP_STR_EQ, param_res_lhs_rhs },
	{ "nestr",	ASM_OP_STR_NE, param_res_lhs_rhs },
	{ "ltstr",	ASM_OP_STR_LT, param_res_lhs_rhs },
	{ "lestr",	ASM_OP_STR_LE, param_res_lhs_rhs },
	{ "catstr",	ASM_OP_STR_CAT, param_res_lhs_rhs },
	{ "match",	ASM_OP_STR_REGEXMATCH, param_res_lhs_rhs },
};

/*
  Function name - ASM_INSTRUCTION_get_sym_name
 
  Description - returns mnemonic name for instruction code.
 
  Input -   instruction opcode 
 
  Return Code/Output 
 */
const char *ASM_TYPE_get_sym_name( ASM_TYPE instr )
{
	return info[ instr ].symname;
}

/*
  Function name - ASM_write_listing
 
  Description - write assembly listing to file
 
  Input -  fp - file 
		   code - list of instructions.
 
  Return Code/Output - none 
 */
void ASM_write_listing(FILE *fp, VSLIST *code)
{
	VSLIST_entry *entry;
	long	pc = 0;

	if (!code) {
		return;
	}

	VSLIST_FOREACH(entry,code) {
		ASM_INSTRUCTION * instr = (ASM_INSTRUCTION *) entry;
		int index = ASM_INSTRUCTION_get_opcode( instr );

#ifdef TRACK_LISTING
		fprintf(fp,";");
		AST_BASE_show_node(fp, instr->ast_entry, 1);
		fprintf(fp,"\n");
#endif

		if (instr->pc) {
			fprintf(fp, "0x%05lX|\t%s\t", 
						instr->pc, 
						ASM_TYPE_get_sym_name( index )  );
		} else {
			fprintf(fp, "0x%05lX(%p)|\t%s\t", 
						pc,
						instr,
						ASM_TYPE_get_sym_name( index ) );

		}

		if (info[ index ].print) {
			info[ index ].print(fp, instr );
		}
		fprintf(fp, "\n");
		pc++;
	}

}



/* *** instruction constructors  *** */

/*
  Function name - ASM_INSTRUCTION_OP3
 
  Description - constructs three address instruction, 
				where all arguments are values (not forward references)
 
  Input -  arg - opcode of instruction
		   res - result of instruction
		   lhs - left hand operator
		   rhs - righ hand operator
 
  Return Code/Output 
 */

ASM_INSTRUCTION *ASM_INSTRUCTION_OP3(
						ASM_TYPE arg,
						ASM_STORAGE_LOCATION res,
						ASM_STORAGE_LOCATION lhs, 
						ASM_STORAGE_LOCATION rhs)
{
	ASM_INSTRUCTION * ret;
	
	ret= (ASM_INSTRUCTION *) VSCRIPTCTX_malloc(sizeof(ASM_INSTRUCTION));
	if (!ret) { 
		return 0;
	}

	ret->type = arg;
	ret->pc = 0;
	ret->res.arg = res;
	ret->lhs.arg = lhs;
	ret->rhs.arg = rhs;

	return ret;
}



/*
  Function name - ASM_INTRUCTION_COND_JUMP_ASM
 
  Description - construct three address instruction, where one argument
				is forward reference to assembly instruction
				this is typical of conditional jump instructions
 
  Input -  arg - opcode of instruction
		   cond - location of jump condition
		   ref  - forward reference to assembly instruction
 
  Return Code/Output 
 */
ASM_INSTRUCTION *ASM_INSTRUCTION_COND_JUMP_ASM(ASM_TYPE arg, ASM_STORAGE_LOCATION cond, ASM_INSTRUCTION *ref)
{
	ASM_INSTRUCTION * ret;
	
	ret = (ASM_INSTRUCTION *) VSCRIPTCTX_malloc(sizeof(ASM_INSTRUCTION));
	if (!ret) { 
		return 0;
	}

	ret->type = arg | ASM_FORWARD_DECL_ASM_RES;
	ret->pc = 0;
	ret->res.forward_decl_asm = ref;
	ret->lhs.arg = cond;
	
	return ret;
}

/*
  Function name - ASM_INSTRUCTION_PC
 
  Description - construct instruction with program counter target 
				(i.e. jump instruction)
 
  Input -       
 
  Return Code/Output 
 */
ASM_INSTRUCTION *ASM_INSTRUCTION_PC(ASM_TYPE arg,ASM_PROGRAM_COUNTER pc)
{
	ASM_INSTRUCTION * ret;
	
	ret = (ASM_INSTRUCTION *) VSCRIPTCTX_malloc(sizeof(ASM_INSTRUCTION));
	if (!ret) { 
		return 0;
	}

	ret->type = arg;
	ret->res.arg = pc;

	return ret;
}

/*
  Function name - ASM_INSTRUCTION_FORWARD_DECL_AST
 
  Description - construct two address instruction where one argument is reference to ast node.
				during linking ref to ast node is resolved.
 
  Input -       arg - op code
				fdecl - forward declaration to ast node 
				count - second argument.
 
  Return Code/Output 
 */
ASM_INSTRUCTION *ASM_INSTRUCTION_FORWARD_DECL_AST(ASM_TYPE arg,struct tagAST_BASE *fdecl, ASM_PROGRAM_COUNTER count)
{
	ASM_INSTRUCTION * ret;
	
	ret = (ASM_INSTRUCTION *) VSCRIPTCTX_malloc(sizeof(ASM_INSTRUCTION));
	if (!ret) { 
		return 0;
	}

	ret->type = arg | ASM_FORWARD_DECL_AST_RES;
	ret->pc = 0;
	ret->res.forward_decl_ast = fdecl;
	ret->lhs.arg = count;

	return ret;
}

/*
  Function name - ASM_INSTRUCTION_FORWARD_DECL_ASM
 
  Description -
 
  Input -       
 
  Return Code/Output 
 */
ASM_INSTRUCTION *ASM_INSTRUCTION_FORWARD_DECL_ASM(ASM_TYPE arg,ASM_INSTRUCTION *fdecl)
{
	ASM_INSTRUCTION * ret;
	
	ret = (ASM_INSTRUCTION *) VSCRIPTCTX_malloc(sizeof(ASM_INSTRUCTION));
	if (!ret) { 
		return 0;
	}

	ret->type = arg | ASM_FORWARD_DECL_ASM_RES;
	ret->pc = 0;
	ret->res.forward_decl_asm = fdecl;


	return ret;
}


/* *** map token to assembly instructions *** */


static struct MapOpToAsmOp {
	int swap_args;
	int token;
	int asm_op;
} map[] = {
	{ 0, TK_OP_NUM_EQ, ASM_OP_NUM_EQ },	
	{ 0, TK_OP_NUM_NE, ASM_OP_NUM_NE },	
	{ 0, TK_OP_NUM_LT, ASM_OP_NUM_LT },	
	{ 1, TK_OP_NUM_GT, ASM_OP_NUM_LT },	
	{ 0, TK_OP_NUM_LE, ASM_OP_NUM_LE },	
	{ 1, TK_OP_NUM_GE, ASM_OP_NUM_LE },	
	{ 0, TK_OP_NUM_ADD, ASM_OP_NUM_ADD },	
	{ 0, TK_OP_NUM_SUBST, ASM_OP_NUM_SUBST },	
	{ 0, TK_OP_NUM_DIV, ASM_OP_NUM_DIV },	
	{ 0, TK_OP_NUM_MULT, ASM_OP_NUM_MULT },	
	{ 0, TK_OP_NUM_MOD,	ASM_OP_NUM_MOD },	
	{ 0, TK_OP_STR_EQ, ASM_OP_STR_EQ },	
	{ 0, TK_OP_STR_NE, ASM_OP_STR_NE },	
	{ 0, TK_OP_STR_LT, ASM_OP_STR_LT },	
	{ 1, TK_OP_STR_GT, ASM_OP_STR_LT },
	{ 0, TK_OP_STR_LE, ASM_OP_STR_LE },	
	{ 1, TK_OP_STR_GE, ASM_OP_STR_LE },
	{ 0, TK_OP_STR_EQ, ASM_OP_STR_EQ },	
	{ 0, TK_OP_STR_NE, ASM_OP_STR_NE },	
	{ 0, TK_OP_STR_LT, ASM_OP_STR_LT },	
	{ 0, TK_OP_STR_CAT, ASM_OP_STR_CAT },	
	{ 0, TK_OP_STR_REGEXMATCH, ASM_OP_STR_REGEXMATCH },	
	{ 0, TK_OP_STR_CAT, ASM_OP_STR_CAT },	
	{ -1, -1, -1 },
};

/*
  Function name - get_asm_op
 
  Description -
		map operator token of epression in AST node to assembly instructions, symetric cases (like <=, >=) 
		are reduced to the same operator , by switchign right and left sides in expression params.
		issue compiler error if no equivalent is found.
		
  Input - pointer to expression AST node.      
 
  Return Code/Output - assembly instruction enum.
 */
int get_asm_op(struct tagAST_EXPRESSION *expr)
{
	int i;
	
	for(i=0;map[i].swap_args != -1; i++) {
		if (map[i].token == expr->val.param.op) {

			if (map[i].swap_args) {
				AST_EXPRESSION *exp;

				exp = expr->val.param.var_left;
				expr->val.param.var_left = expr->val.param.var_right;
				expr->val.param.var_right = exp;
			}
			return map[i].asm_op;	
		}
	}
	do_yyerror( expr->super.location, "Internal error. Ups. Operation is not implemented in assembler. ");
	return -1;
}
