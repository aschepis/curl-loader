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
#include "syntax.h"
#include "ast.h"
#include "asm.h"
#include <string.h>

typedef void (*PRINT_NODE) (AST_BASE *node, FILE *fp);

/*
   map internal name of node to display node (as it is written in xml dump)
 */
typedef struct tagAST_NAMES {
	char *internal_name;
	char *display_name;
} AST_NAMES;

/*
   information about one node in abstract syntax tree (AST)
 */
typedef struct {
  size_t	node_size; // size of struct
  AST_NAMES node_name; // internal name; display name
  PRINT_NODE info;     // callback function: print content of node in xml dump

} AST_INFO;


static AST_NAMES expr_name[] = {

	{ "S_EXPR_SCALAR_REF",	"scalar variable" },

	{ "S_EXPR_HASH_REF",	"hash variable" },

	{ "S_EXPR_ARRAY_REF",	"array variable" },

	{ "S_EXPR_CODE_REF",	"code reference" },

	{ "S_EXPR_BINARY",		"expression with two operands" },

	{ "S_EXPR_UNARY",		"expression with one operand" },
	
	{ "S_EXPR_INT_CONST",	"integer constant" },

	{ "S_EXPR_STRING_CONST","string constant" },
	
	{ "S_EXPR_DOUBLE_CONST","real constant" },

	{ "S_EXPR_FUNCALL",		"function call" },		

	{ "S_EXPR_CAST_INT",	"use as int" },
	
	{ "S_EXPR_CAST_STR",	"use as string" },

};

const char * AST_BASE_get_expression_type_name(S_EXPR_TYPE type)
{
	return expr_name[ type ].display_name;
}


static AST_NAMES var_type[] = 
{
	{ "S_VAR_SCALAR", "scalar" },

	{ "S_VAR_HASH",	"hash" },
	
	{ "S_VAR_ARRAY", "array" },

	{ "S_VAR_CODEREF","reference to a function" },

	{ "S_VAR_ANYTYPE", "any-type" },

	{ "S_VAR_ERROR","error value" },

};


/* 
   turn variable type enum into human redable string
   (is used to format error messages)
 */
const char * AST_BASE_get_var_type_name(AST_VAR_TYPE type)
{
	return var_type[ type ].display_name;
}


const char *MY_YY_get_op_token_name(int token);

/*
	Callback per AST node type:
	each callback prints content of relevant AST node type.
 */

static void print_noop(AST_BASE *node,FILE *fp) 
{
	V_UNUSED(node);
	V_UNUSED(fp);
}

static void  print_label(AST_BASE *node,FILE *fp)
{
	AST_LABEL *label = (AST_LABEL *) node;
	fprintf(fp, " label=\"%s\"", label->label_name );
}


static void  print_func_call(AST_BASE *node,FILE *fp)
{
	AST_FUNCTION_CALL *func = (AST_FUNCTION_CALL *) node;

	fprintf(fp, " function=\"%s\"", func->name );
}

static void  print_func_decl(AST_BASE *node,FILE *fp)
{
	AST_FUNCTION_DECL *func = (AST_FUNCTION_DECL *) node;

	fprintf(fp, " function=\"%s\" ret_type=\"%s\"", 
		func->name, AST_BASE_get_var_type_name(func->ret_value) );
}

void  AST_print_expression(AST_BASE *node,FILE *fp)
{
	AST_EXPRESSION *expr = (AST_EXPRESSION *) node;

	fprintf(fp, " expr_type=\"%s\"", expr_name[expr->exp_type].internal_name );

	switch( expr->exp_type) {
	case S_EXPR_HASH_REF:
	case S_EXPR_ARRAY_REF:
	case S_EXRP_SCALAR_REF:
		fprintf(fp, " id=\"%s\"", expr->val.ref.lhs );
		break;
	case S_EXPR_INT_CONST:
		fprintf(fp," int_const=\"%ld\"", expr->val.int_value);
		break;

	case S_EXPR_STRING_CONST:
		fprintf(fp," string_const=\"%s\"", expr->val.string_value);
		break;

	case S_EXPR_DOUBLE_CONST:
		fprintf(fp," double_const=\"%f\"", (float) expr->val.double_value);
		break;

	case S_EXPR_UNARY:
		fprintf(fp, " is_prefix=\"%d\" opcode=\"%s\"",
			expr->val.unary.is_prefix, 
			MY_YY_get_op_token_name( expr->val.unary.op ) ); 
		break;

	case S_EXPR_BINARY:
		fprintf(fp, " opcode=\"%s\"", 
			MY_YY_get_op_token_name( expr->val.param.op ) ); 
		break;

	case S_EXPR_FUNCALL:
	case S_EXPR_CAST_INT:
	case S_EXPR_CAST_STR:
		break;

	default:
	    break;
	}
}

static void print_vardef(AST_BASE *node,FILE *fp)
{
	AST_VARDEF *var = (AST_VARDEF *) node;

	fprintf(fp, " var_name=\"%s\" expr_type=\"%s\"", var->var_name, var_type[var->var_type].internal_name );
}


/*
    Table: contains definition of each AST node; see AST_INFO.
 */
AST_INFO ast_names[] =
{
	{ sizeof(AST_LABEL) , { "S_LABEL", "Label" }, print_label },

	{ sizeof(AST_LABEL), { "S_GOTO",  "goto" }, print_label },

	{ sizeof(AST_BASE), { "S_STATEMENT_LIST", "statement list" }, 0 },

	{ sizeof(AST_ASSIGN), { "S_ASSIGN", "assignment statement" }, print_noop },

	{ sizeof(AST_VARDEF), { "S_VARDEF",  "variable definition" }, print_vardef },

	{ sizeof(AST_VARDEF), { "S_VARUNDEF", "undefine variable" },  print_vardef },

	{ sizeof(AST_IF), { "S_IF",	"if statement" }, 0 },

	{ sizeof(AST_COND), { "S_COND", "else/elsif condition" }, 0 },

	{ sizeof(AST_COND), { "S_WHILE", "while statement" }, 0 },

	{ sizeof(AST_COND), { "S_UNTIL", "do .. until statement" }, 0 },

	{ sizeof(AST_FUNCTION_CALL), { "S_FUN_CALL", "function call" }, print_func_call },

	{ sizeof(AST_FUNCTION_DECL), { "S_FUN_DECL", "function declaration" }, print_func_decl },

	{ sizeof(AST_COND), { "S_RETURN",  "return statement" }, 0 },

	{ sizeof(AST_LABEL),{ "S_CONTINUE",	 "continue with next iteration of loop" }, 0 },
	
	{ sizeof(AST_LABEL),{ "S_BREAK",	 "break from loop" }, 0 },

	{ sizeof(AST_EXPRESSION), { "S_EXPRESSION", "expression" }, AST_print_expression },

};


/*
  Function name - AST_BASE_get_node_size
 
  Description - 
 
  Input -       type - enumeration of type of ast node
 
  Return Code/Output size of ast node in bytes
 */
size_t AST_BASE_get_node_size( S_TYPE type )
{
	return ast_names[ type ].node_size;
}


/*
  Function name - AST_BASE_get_name
 
  Description - 
 
  Input -      type - enumeration of type of ast node
 
  Return Code/Output the name of the node (string)
 */
const char * AST_BASE_get_name(S_TYPE type)
{
	return ast_names[ type ].node_name.internal_name;
}


/*
  Function name - AST_BASE_get_name
 
  Description - 
 
  Input -      type - enumeration of type of ast node
 
  Return Code/Output a human readable name of the node (string); used to format error messages.
 */
const char * AST_BASE_get_display_name(S_TYPE type)
{
	return ast_names[ type ].node_name.display_name;
}



/*
  Function name - AST_BASE_show_node
 
  Description - displays content of one AST node as opening of XML tag.
				all tree node properties are displayed as attributes
  Input -       fp - output file
				ast - current tree node that we are printing.
				show_location - flag: do we print location info of this node?
 
  Return Code/Output 
 */
int AST_BASE_show_node(FILE *fp, AST_BASE *ast, int show_location)
{
  	fprintf(fp,"<%s", AST_BASE_get_name( ast->type ) );
	
	// node attributes are displayed by callback contained in AST node descriptor;
	if (ast_names[ ast->type ].info) {
		ast_names[ ast->type ].info( ast, fp );
	}

	if (show_location) {
		if (ast->location.first_line == ast->location.last_line) {
			fprintf(fp," pos=\"%d,%d-%d\"",
				ast->location.first_line,
				ast->location.first_column,
				ast->location.last_column);
		} else {
			fprintf(fp," pos=\"%d,%d-%d,%d\"",
				ast->location.first_line,
				ast->location.first_column,
				ast->location.last_line,
				ast->location.last_column);
		}
	}

	fprintf(fp,">");
	return 0;
}


/*
  Function name - tree_to_xml
 
  Description - dumps one AST node tree entry to XML file.
 
  Input -       entry - tree node entry.
				visit - if 1: write start of tag
						if 0: write end of tag
				fp	  - output file
 
  Return Code/Output 
 */
int  tree_to_xml(VTREENODE *entry, int visit, FILE *fp)
{
	AST_BASE *ast = (AST_BASE *) entry;

	if (visit) {
		AST_BASE_show_node(fp, ast, 0);
		fprintf(fp,"\n");
	}
	else {
		fprintf(fp,"</%s>\n", AST_BASE_get_name( ast->type ));

	}
	// we are debugging: so this entry can be the last one...
	fflush(fp);
	return 0;

}


/*
  Function name - MY_YY_dump_as_xml
 
  Description - format AST tree into XML file;
				used only for debug purposes;
				walks AST tree recursively and dumps each node as XML string.

  Input -  fp - output file
		   base - the AST tree.
 
  Return Code/Output 
 */
void MY_YY_dump_as_xml( FILE *fp, AST_BASE *base )
{
	VTREENODE *ch;

	tree_to_xml( &base->node, 1, fp );
	
	for(ch = VTREE_first_child( &base->node ); ch != 0; ch = VTREE_right_sibling(ch)) {
		MY_YY_dump_as_xml( fp, (AST_BASE *) ch );		
	}

	tree_to_xml( &base->node, 0, fp );
}


/*
  Function name - AST_BASE_new_internal
 
  Description - constructor of empty AST node
				

 
  Input -       ctx - memory allocator that is used.
				type  - the type enum for AST nodes
				location - position of text in source file that is the source of this AST node.
 
  Return Code/Output  
				pointer to base structure of all AST nodes - AST_BASE

				A structure that holds attributes set during code generation is prepended
				to this node; but we are returning pointer to AST node;
 */

AST_BASE * AST_BASE_new_internal(VCONTEXT *ctx, S_TYPE type, YYLTYPE location) 
{
	AST_BASE * ret = 0;
	void *mem = 0;
	size_t sz;

	sz = AST_BASE_get_node_size(type) + sizeof(AST_ASM_ATTRIBUTE);

	mem = V_MALLOC( ctx, sz );
	if (!mem) {
		return 0;
	}

	/* 
		init attributes for code generation 
	 */
	AST_ASM_ATTRIBUTE_init( (AST_ASM_ATTRIBUTE *) mem );

	/* code generation info prepends the AST for following reasons
		- ast will be turned into debug info during later stage,
		  and code generation info is not relevant for this one
	 */
	ret = (AST_BASE *) ((unsigned char *) mem + sizeof(AST_ASM_ATTRIBUTE));

	AST_BASE_init(ret, type, location);
	VTREE_init_root( &ret->node );

#if 0
	fprintf(stderr,"%s\n", AST_BASE_get_name( ret->type ) );
#endif
	return ret;
}

/*
  Function name - AST_BASE_new
 
  Description - allocate AST node from compiler allocator;
				memory is allocated but kept until end of compilation,
				where it is freed in on step.
 
  Input -       type  - the type enum for AST nodes
				location - position of text in source file that is the source of this AST node.
 
  Return Code/Output see AST_BASE_new_internal
 */
AST_BASE * AST_BASE_new( S_TYPE type, YYLTYPE location) 
{
	return AST_BASE_new_internal(VSCRIPTCTX_ctx, type, location);
}


#define IS_VARDEF(x) ((x) && (x)->super.type == S_VARDEF )
#define NEXT_VARDEF(x)  (AST_VARDEF *) VTREE_right_sibling((VTREENODE *) x)

#define IS_EXPR(x) ((x) && (x)->super.type == S_EXPRESSION)
#define NEXT_EXPR(x)  (AST_EXPRESSION *) VTREE_right_sibling((VTREENODE *) x)

/*
   compare two function signatures for equality
 */
int AST_compare_signatures(AST_FUNCTION_DECL *fdecl_lhs, AST_FUNCTION_DECL *fdecl_rhs)
{
	AST_VARDEF *left, *right;
	
	left = fdecl_lhs->params;
	right = fdecl_rhs->params;
	
	while(IS_VARDEF(left) && IS_VARDEF(right)) {

		if (left->var_type != right->var_type) {
			return -1;
		}

		left = NEXT_VARDEF(left);
		right = NEXT_VARDEF(right);
	}

	if (!IS_VARDEF(left) && !IS_VARDEF(right)) {
		return 0;
	}
	return 1;
}

/*
  check if expression type of function call matches expression of function parameters
  if yes then return FUNC_DECL_WITH_SAME_SIGNATURE and set found retval
 */
int AST_compare_fcall(AST_FUNCTION_CALL *fcall_lhs, AST_FUNCTION_DECL *fdecl_rhs)
{
	AST_VARDEF *right;
	AST_EXPRESSION *left;
	
	left = fcall_lhs->expression;
	right = fdecl_rhs->params;
	while(IS_EXPR(left) && IS_VARDEF(right)) {

		/*TODO: don't have expression type right now. can't compare it*/

		left = NEXT_EXPR(left);
		right = NEXT_VARDEF(right);
	}

	if (!IS_EXPR(left) && !IS_VARDEF(right)) {
		return 0;
	}

	return 1;
}

AST_FUNCTION_DECL *AST_make_xmethod_decl( const char *func_name, const char *prototype)
{
	YYLTYPE location;
	AST_FUNCTION_DECL *fdecl;
	int state = 0;
	int num_params = 0;
	const char *s;
	VCONTEXT *ctx = VCONTEXT_get_default_ctx();
	AST_VAR_TYPE rvalue;

	YYLTYPE_set_null(location);

	fdecl = (AST_FUNCTION_DECL *) AST_BASE_new_internal( ctx, S_FUN_DECL, location);
	if (!fdecl) {
		return 0;
	}

	fdecl->name = VUTIL_strdup( ctx, func_name );
	fdecl->params = 0;
	/* parse prototype */
	state = 0;  
	
	for(s = prototype; *s != '\0'; ) {

ty:
		switch(*s) {
		case 's':
			rvalue = S_VAR_SCALAR;
			break;
	
		case '@':
			rvalue = S_VAR_ARRAY;
			break;

		case '%':
			rvalue = S_VAR_HASH;
			break;

		case '#':
			rvalue = S_VAR_CODEREF;
			break;
 
		case '!':
			rvalue = S_VAR_ANYTYPE;
			break;
		case ':':
			state = 1;
			break;
	
		case ',':
			++s;
			goto ty;

		default:
			return 0;
		}

	
		if (state == 2) {
			AST_VARDEF *param = (AST_VARDEF *) AST_BASE_new_internal( ctx, S_VARDEF, location);

			param->var_name = "<xmethod-param>";
			param->expr = 0;
			param->var_type = rvalue ;
			
			// add parameter to function declaration
			VTREE_insert_child( &fdecl->super.node , &param->super.node, VTREE_INSERT_LAST, 1);		
		
			fdecl->params = (AST_VARDEF *) VTREE_first_child( &fdecl->super.node );
			num_params += 1;
		}
		if (state == 1) {
			fdecl->ret_value = rvalue;
 			state = 2;
		}
		
		++s;

	}
	
	if (state == 2) {
		fdecl->num_param = num_params;
		return fdecl;
	}

	V_FREE( ctx, fdecl);
	return 0;
}
