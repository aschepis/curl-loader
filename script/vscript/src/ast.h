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
#ifndef _AST_H_
#define _AST_H_

#include <stdio.h>
#include <util/vtree.h>
#include <util/vslist.h>
#include "yystype.h"


#define LAMBDA_FUNCTION_NAME "#LAMBDA"

struct tagASM_INSTRUCTION;

typedef enum 
{
  S_LABEL,
  
  S_GOTO,

  S_STATEMENT_LIST,
  
  S_ASSIGN,
  
  S_VARDEF,

  S_VARUNDEF,

  S_IF,

  S_COND,

  S_WHILE,

  S_UNTIL,
  
  S_FUN_CALL,

  S_FUN_DECL,

  S_RETURN, 

  S_CONTINUE,
  
  S_BREAK,

  S_EXPRESSION,

} 
  S_TYPE;


typedef enum {
   
   S_VAR_SCALAR,

   S_VAR_HASH,

   S_VAR_ARRAY,

   S_VAR_CODEREF,

   S_VAR_ANYTYPE,
   
   S_VAR_ERROR,  // one of the types in an expression evaluated to an error, don't propagate the error further

}  AST_VAR_TYPE;



typedef enum tagS_EXPR_TYPE {
/*!!! first four elements agree with enumeration of AST_VAR_TYPE  !!! */
  S_EXRP_SCALAR_REF,

  S_EXPR_HASH_REF,

  S_EXPR_ARRAY_REF,

  S_EXPR_CODE_REF,

/*!!!*/
  
  S_EXPR_BINARY,

  S_EXPR_UNARY,

  S_EXPR_INT_CONST,

  S_EXPR_STRING_CONST,

  S_EXPR_DOUBLE_CONST,


// S_EXPR_NIL_CONST,

  S_EXPR_FUNCALL,


  S_EXPR_CAST_INT,
  
  S_EXPR_CAST_STR,

}
  S_EXPR_TYPE;


#define IS_EXPR_TYPE_REF(x) (/*(x) >= S_EXRP_SCALAR_REF && */ (x) <= S_EXPR_CODE_REF)



typedef struct tagAST_BASE {
	VTREENODE node;
	S_TYPE    type;
	YYLTYPE   location;	

}	AST_BASE;

V_INLINE void AST_BASE_init(AST_BASE *base, S_TYPE type, YYLTYPE location)
{
	VTREE_init_root(&base->node);
	base->type = type;
	base->location = location;
}

AST_BASE * AST_BASE_new(S_TYPE type, YYLTYPE location);

const char * AST_BASE_get_name(S_TYPE type);

void AST_BASE_dump_as_xml( FILE *fp, AST_BASE *base );
int AST_BASE_show_node(FILE *fp, AST_BASE *ast, int show_location);
void  AST_print_expression(AST_BASE *node,FILE *fp);

/* ** S_TYPE: S_LABEL S_GOTO S_BREAK ** */

typedef struct tagAST_LABELONLY {

	AST_BASE super;
	char *label_name;

}	AST_LABEL;

/* ** S_TYPE: S_VARDEF S_VARUNDEF ** */

typedef struct {

	AST_BASE super;
	char *var_name;
	AST_VAR_TYPE var_type;
	struct tagAST_EXPRESSION *expr;

}
	AST_VARDEF;


/* ** S_TYPE: S_ASSIGN ** */

typedef struct tagAST_ASSIGN {

	AST_BASE super;
	struct tagAST_EXPRESSION *lhs; // either one of the following: S_EXRP_SCALAR_REF S_EXRP_HASH_REF  S_EXRP_ARRAY_REF
	struct tagAST_EXPRESSION *rhs;

}	AST_ASSIGN;

/* ** S_TYPE: S_IF */ 

typedef struct tagAST_IF {

	AST_BASE super;
	struct tagAST_COND *expr;
	struct tagAST_COND *first_else;

}	AST_IF;	

/* ** S_TYPE: S_COND S_WHILE S_RETURN  ** */

typedef struct tagAST_COND {

	AST_BASE super;
	struct tagAST_EXPRESSION *cond;
	AST_BASE *statement;

	// for loops.
	struct tagASM_INSTRUCTION *label;
	struct tagASM_INSTRUCTION *label_start;	

}	AST_COND;





/* ** S_TYPE: S_FUN_CALL ** */ 

typedef struct tagAST_FUNCTION_CALL {

	AST_BASE super;
	char  *name;
	struct tagAST_EXPRESSION *expression;
	struct tagAST_FUNCTION_DECL *func_decl; //resolved later during semantic check

}
	AST_FUNCTION_CALL;

/* ** S_TYPE: S_FUN_DECL ** */ 

typedef struct  tagAST_FUNCTION_DECL {

	AST_BASE super;
	char  *name;
	
	AST_VAR_TYPE ret_value; /* return type value */
	
	AST_VARDEF *params;	    /* first parameter */
	unsigned int num_param;
	
	union {
		AST_BASE *body;
		int xmethod_id;
	}
		impl;

}
	AST_FUNCTION_DECL;


/* ** S_TYPE: S_EXPRESSION ** */

typedef struct tagAST_EXPRESSION {

	AST_BASE    super;
	S_EXPR_TYPE exp_type;
	AST_VAR_TYPE value_type;
	
	union {
		struct {
			int	   op;
			struct tagAST_EXPRESSION *var_left, *var_right;
		} param;

		struct {
			int    op;
			int	   is_prefix;
			struct tagAST_EXPRESSION *var;
		} unary;

		struct {
			char  *lhs;
			struct tagAST_EXPRESSION *index_exp;
			AST_VARDEF *var_def;
		} ref;

		long   int_value;

		double double_value;

		char   *string_value;

		struct tagAST_FUNCTION_CALL *fcall;

	} val;

} AST_EXPRESSION;


const char * 
b(S_TYPE type);


const char * AST_BASE_get_display_name(S_TYPE type);


const char * AST_BASE_get_expression_type_name(S_EXPR_TYPE type);

const char * AST_BASE_get_var_type_name(AST_VAR_TYPE type);

size_t AST_BASE_get_node_size( S_TYPE type );

/*
   compare two function signatures for equality
 */
int AST_compare_signatures(AST_FUNCTION_DECL *fdecl_lhs, AST_FUNCTION_DECL *fdecl_rhs);

/*
  check if expression type of function call matches expression of function parameters
  if yes then return FUNC_DECL_WITH_SAME_SIGNATURE and set found retval
 */
int AST_compare_fcall(AST_FUNCTION_CALL *fcall_lhs, AST_FUNCTION_DECL *fdecl_rhs);

AST_FUNCTION_DECL *AST_make_xmethod_decl( const char *func_name, const char *prototype);

#endif


  









