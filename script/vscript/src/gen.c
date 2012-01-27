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
#include "ast.h"
#include "asm.h"
#include "sym.h"
#include "vm.h"
#include "syntax.h"
#include <stdlib.h>
#include <assert.h>





/*
  Function name - register_foward_decls
 
  Description - first passs - get all named function defs into global scope table
				register labels and functions;

				this function recurses over the AST tree.

				used to find a forward reference to a label or function occurs 
				during compilation

				example

				goto foo // when we compile this, we don't know what foo is; have to find pointer to it quickly.

				foo:
 
  Input -       root - root of AST tree
				is_root - 1 if we are dealing with root node.
 
  Return Code/Output 
 */
void  register_foward_decls(AST_BASE *root, int is_root)
{
	VTREENODE *cur;

	VTREE_FOREACH_CHILD(cur,&root->node) {

		AST_BASE *base = (AST_BASE *) cur;
		
		if (base->type == S_LABEL) {
			SYM_SCOPE_define(base);
		}
		
		if (is_root) {
			if (base->type == S_FUN_DECL) {
 				SYM_SCOPE_define(base);	
				continue;
			}

		}
		if (base->type == S_STATEMENT_LIST) {
			register_foward_decls(base,is_root);	
		}
	}
}


/*
   Check before start of generating any code
   - if we had any errors so far then don't bother with code generation.
     in this situation we only bother to check semantics of code;
 */
#define START_CODE_GEN \
	if (vscript_ctx->my_yy_is_error) {  \
		goto next_ast; \
	}

#if 0
void release_location( AST_BASE *param, AST_BASE *code_add)
{
	if (! SYM_SCOPE_release_location( 
			vscript_ctx->current_scope, get_code(param)->storage ) ) {
		
		ASM_ADD( code_add, ASM_INSTRUCTION_OP3( ASM_VARUNDEF, 		 
							get_code(param)->storage, 0, 0 ) );

	}
}
#endif


void  semantics_check(AST_BASE *root);

/*
  Function name - do_process_expression
 
  Description - symantics check / code generation for expressions.
			
				this function recurses into expressions.
				
  Input -  expr  expression type AST node.     
 
  Return Code/Output 
 */
void  do_process_expression( AST_EXPRESSION *expr)
{
	AST_BASE *parent = (AST_BASE *) VTREE_parent( &expr->super.node );
	int create_copy = V_FALSE;

	switch( expr->exp_type) {

	case S_EXPR_BINARY: {
		int op;

		// propagate errors.
		if (expr->val.param.var_left->value_type == S_VAR_ERROR || 
			expr->val.param.var_right->value_type == S_VAR_ERROR) {
			expr->value_type = S_VAR_ERROR;
			return;
		}

		// check that arguments are all scalars.
		if (expr->val.param.var_left->value_type == S_VAR_SCALAR || 
			expr->val.param.var_right->value_type == S_VAR_SCALAR) {
			expr->value_type = S_VAR_SCALAR;
		} else {
			expr->value_type = S_VAR_ERROR;

			do_yyerror(expr->super.location, 
							"Can't mix %s and %s in this expression", 
							AST_BASE_get_var_type_name( expr->val.param.var_left->value_type ),
							AST_BASE_get_var_type_name( expr->val.param.var_right->value_type )
							);
		}

		START_CODE_GEN

		// get location of return value.
		get_code( expr )->storage = SYM_SCOPE_make_new_location_reusable( vscript_ctx->current_scope );

		// enter code of parameter expressions
		ASM_MERGE_CODE( expr, expr->val.param.var_left );
		ASM_MERGE_CODE( expr, expr->val.param.var_right );

		op = get_asm_op(expr);
		if (op != -1) {
			ASM_ADD( expr, 
				ASM_INSTRUCTION_OP3( op, 
						get_code( expr )->storage,
						get_code( expr->val.param.var_left )->storage, 
						get_code( expr->val.param.var_right )->storage ) );
		}

		// temporaries may or may not be reused, no longer needed.
		SYM_SCOPE_release_location( vscript_ctx->current_scope, get_code(expr->val.param.var_left)->storage );
		SYM_SCOPE_release_location( vscript_ctx->current_scope, get_code(expr->val.param.var_right)->storage );

		}
		break;


	case S_EXPR_UNARY:
		expr->value_type = expr->val.unary.var->value_type;

		//** TODO: check type

		START_CODE_GEN

		break;


	case S_EXPR_FUNCALL:
		// propagate errors.
		if (!expr->val.fcall->func_decl) {
			expr->value_type = S_VAR_ERROR;
			return;
		}
		// return value of expression eq to return value of function.
		expr->value_type = expr->val.fcall->func_decl->ret_value;
		
		get_code(expr)->storage = get_code(expr->val.fcall)->storage; 
		
		START_CODE_GEN
		
		ASM_MERGE_CODE( expr, expr->val.fcall );
		
		if (parent->type == S_FUN_CALL) {
			create_copy = V_TRUE;
		}


		break;

	case S_EXRP_SCALAR_REF:
	case S_EXPR_HASH_REF:
	case S_EXPR_ARRAY_REF: {
		SYM_SCOPE *scope;
		AST_BASE  *def;

		// propagate errors.
		if (expr->value_type != S_VAR_ERROR && 
			SYM_SCOPE_is_defined_ex((AST_BASE *) expr, &def, &scope)) {
			expr->value_type = S_VAR_ERROR;
		}

		START_CODE_GEN

		if (expr->exp_type == S_EXRP_SCALAR_REF) {

			get_code(expr)->storage = get_code(def)->storage;
			//expr->value_type = S_VAR_SCALAR;


		} else {
			/* in assignment RHS - lookup, in assignment LHS this is modification of entry hash|array entry */

			int op = expr->exp_type == S_EXPR_HASH_REF ?  ASM_HASH_LOOKUP : ASM_ARRAY_LOOKUP;

			ASM_MERGE_CODE( expr, expr->val.ref.index_exp );
			
			//get_code(expr)->storage = get_code(def)->storage;
			//expr->value_type = S_VAR_SCALAR;

			get_code( expr )->storage = SYM_SCOPE_make_new_location_reusable( vscript_ctx->current_scope ); 

			ASM_ADD( expr, 
				ASM_INSTRUCTION_OP3( op,
						get_code(expr)->storage,
						get_code(def)->storage, 
						get_code(expr->val.ref.index_exp)->storage ) );

			expr->value_type = S_VAR_ANYTYPE;
		}	
		
		if (parent->type == S_FUN_CALL) {
			create_copy = V_TRUE;
		}

		}
		break;

	case S_EXPR_INT_CONST:
	case S_EXPR_STRING_CONST:
	case S_EXPR_DOUBLE_CONST: {


		expr->value_type = S_VAR_SCALAR;

		START_CODE_GEN

		// register constant in pool of constants - so it will have its index in initialised data segment.
		get_code(expr)->storage = ASM_CONSTANT_POOL_location( vscript_ctx, expr );

		if (parent->type == S_FUN_CALL) {
			create_copy = V_TRUE;			
		}
		}
		break;

		/*
	case S_EXPR_CAST_INT:
		expr->value_type = S_VAR_SCALAR;
		break;

	case S_EXPR_CAST_STR:
		expr->value_type = S_VAR_SCALAR;
		break;
*/	
	default:
		break;
	}
	
next_ast: ;


	if (!vscript_ctx->my_yy_is_error) {  

		if (create_copy) {
			/*
				in *some* situations where rsult is a function parameter.
				we have to make a local definition, due to our calling convention
				(which is that function parameters are evalueated in place - no push instruction
				and we have a function invocation record that points to start of parameter area on stack)

			 */
			//if (!SYM_SCOPE_is_location_on_stack( vscript_ctx->global_scope, get_code(expr)->storage ) ) {
				
				ASM_STORAGE_LOCATION loc = get_code(expr)->storage;

				get_code( expr )->storage = SYM_SCOPE_make_new_location_reusable( vscript_ctx->current_scope ); 

				ASM_ADD( expr, 
					ASM_INSTRUCTION_OP3(ASM_MOVE,	
					get_code(expr)->storage, loc, 0) );


			//}
			
		}
	}

}

int MATCH_TYPES(AST_VAR_TYPE lh, AST_VAR_TYPE rh)
{
	if (rh == S_VAR_ANYTYPE || lh == S_VAR_ANYTYPE) {
		return 0;
	}
	return rh != lh;
}

void check_cond( AST_COND *cond )
{
#if 1
	semantics_check( (AST_BASE *) cond->cond );
			
	/* check if condition return value is a scalar expression */
	if (cond->cond->value_type  != S_VAR_SCALAR) {
		do_yyerror(cond->super.location, 
					"Condition must return a scalar value, instead it returns %s",
					AST_BASE_get_var_type_name( cond->cond->value_type ) );
	}
#endif
}

int set_expression_target( AST_BASE *base, int target)
{
	ASM_INSTRUCTION  *last;
	get_code(base)->storage = target;
	last = get_last_instruction( base );
	if (last && 
			!(ASM_INSTRUCTION_get_opcode(last) == ASM_CALL ||
			  ASM_INSTRUCTION_get_opcode(last) == ASM_XCALL) ) {
		last->res.arg =  target;
		return 1;
	}
	return 0;
}

/*
  Function name - semantics_check
 
  Description - symantics check / code generation for whole cod.

				We generate code 
					1) first for the global scope
					2) then for each function

				This function recurses into the tree.
			
				
  Input -  expr  expression type AST node.     
 
  Return Code/Output 
 */
void  semantics_check(AST_BASE *base)
{
	// create global scope
	VTREENODE *ccur;
	int has_func_decl = 0;
	
	/*  visit nodes under current one (depth first traversal) */
	VTREE_FOREACH_CHILD(ccur,&base->node) {
		AST_BASE *curnode = (AST_BASE *) ccur;

		// special case - no depth first traversal for for expressions,
		// otherwise we generate code for expressions before we have enough data.
#if 0
		if ((base->type == S_COND || 
			 base->type == S_WHILE || 
			 base->type == S_FUN_CALL ||
			 base->type == S_UNTIL) && curnode->type == S_EXPRESSION) {
			
			continue;
		}
#endif
		if (base->type == S_FUN_CALL && curnode->type == S_EXPRESSION) {			
			continue;
		}

		// don't do function declarations, do them later; first deal with global scope
		if (curnode->type != S_FUN_DECL) {
			semantics_check(curnode); 
		} else {
			// code for function is done after processings global scope,
			// so that location of variable declarations in global scope are available
			// to the function.
			has_func_decl = 1;
		}
	}


	/* process current node in AST */
	switch(base->type) {
		case S_LABEL:
			START_CODE_GEN			
			ASM_ADD( base, ASM_INSTRUCTION_FORWARD_DECL_AST(ASM_LABEL, base, -1) );
			break;

		case S_GOTO: {
			AST_LABEL *label = (AST_LABEL *) base;
			STR2AST *rt;
			SYM_SCOPE *find_scope;

			rt = SYM_SCOPE_find_ex( vscript_ctx->current_scope, label->label_name, &find_scope ); 

			if (rt == 0) {
					do_yyerror(label->super.location, 
						"trying to go to a label that is not defined within the current scope",
						label->label_name );

			} else if (!rt->ast || rt->ast->type != S_LABEL) {
					do_yyerror(label->super.location, 
						"Target of goto must be a label, here it is a %s",
						rt->ast ? AST_BASE_get_display_name(rt->ast->type) : "undefined");

			} else if (find_scope != vscript_ctx->current_scope) {
					do_yyerror(label->super.location, 
						"Target of goto must within the same scope as goto statement");

			}

			
			START_CODE_GEN

			ASM_ADD( base, 
				ASM_INSTRUCTION_FORWARD_DECL_AST( ASM_GOTO, rt->ast, -1) );
			}
			break;
	
		case S_STATEMENT_LIST: {
			VTREENODE *node;
		
			/* code generation - combine code of all component statements */
			START_CODE_GEN

			VTREE_FOREACH_CHILD(node, &base->node) {

				AST_BASE *node_ast = (AST_BASE *) node;

				if (node_ast->type != S_FUN_DECL) {
					ASM_MERGE_CODE( base, node );
				}
			}
			}
			break;

		case S_ASSIGN: {
			AST_ASSIGN *as = (AST_ASSIGN *) base;

			if (as->lhs->exp_type == S_EXRP_SCALAR_REF &&					
				as->lhs->val.ref.var_def != 0 &&
				as->rhs->value_type != S_VAR_ERROR) {
				

				/* if variable type of definition of left hand side reference  NOT EQUAL 
				   right hand side value type expression
				 */
				
				if (MATCH_TYPES(as->lhs->val.ref.var_def->var_type, as->rhs->value_type) ) {
					do_yyerror(as->super.location, 
								"Assigning a %s to variable declared as %s",
								AST_BASE_get_var_type_name( as->rhs->value_type ),
								AST_BASE_get_var_type_name( as->lhs->val.ref.var_def->var_type ) );
				} 					
			}

			/* code generation */
			START_CODE_GEN


			ASM_MERGE_CODE( base, as->rhs );
			
			if (as->lhs->exp_type == S_EXPR_HASH_REF || 
				as->lhs->exp_type == S_EXPR_ARRAY_REF) {
				
				ASM_INSTRUCTION *lhs = get_first_instruction(as->lhs);
				
				lhs->type += 2;
				lhs->res.arg = get_code(as->rhs)->storage;
				
				ASM_MERGE_CODE( base, as->lhs );
			
			} else {
			
				ASM_MERGE_CODE( base, as->lhs );

				ASM_ADD( base, 
					ASM_INSTRUCTION_OP3(ASM_CPY,
					get_code(as->lhs)->storage,
					get_code(as->rhs)->storage, 0) );


			}
			SYM_SCOPE_release_location( vscript_ctx->current_scope, get_code(as->rhs)->storage );

			}
			break;

		case S_VARUNDEF: {
			AST_VARDEF *vd = (AST_VARDEF *) base;

			if (SYM_SCOPE_undef(vscript_ctx->current_scope, vd->var_name)) {							
				do_yyerror(vd->super.location, 
					 "Can't undefine variable %s, it is not defined in the current scope",
					 vd->var_name );				
			}

			/* code gen */
			START_CODE_GEN

			ASM_ADD( base, 
						ASM_INSTRUCTION_OP3( ASM_VARUNDEF, 		 
							get_code(vd)->storage, 0, 0 ) );

			}
			break;

		case S_VARDEF: {

			AST_VARDEF *vdef;

			SYM_SCOPE_define(base);

			vdef = (AST_VARDEF *) base;
			
			/* code gen */
			START_CODE_GEN

			get_code(vdef)->storage = SYM_SCOPE_make_new_location( vscript_ctx->current_scope );

			if (vdef->var_type != S_VAR_SCALAR) {
				ASM_ADD( base, 
							ASM_INSTRUCTION_OP3( ASM_VARDEF,
								vdef->var_type, 
								get_code(vdef)->storage, 0 ) );
			}

			if (vdef->expr) {

				ASM_MERGE_CODE( base, vdef->expr );

				/* generate optional assignment */
				ASM_ADD( base, 
					ASM_INSTRUCTION_OP3(ASM_MOVE,
									get_code(vdef)->storage,
									get_code(vdef->expr)->storage, 0) );
			}
			}
			break;

		case S_IF: {
			VTREENODE *node;
			ASM_INSTRUCTION *label, *label_eof;


			/* code gen */
			START_CODE_GEN

			label_eof = ASM_INSTRUCTION_PC( ASM_LABEL, 0);

			VTREE_FOREACH_CHILD(node, &base->node) {

				label = ASM_INSTRUCTION_PC( ASM_LABEL, 0);

				if (((AST_BASE *) node)->type == S_COND) {
					AST_COND *cond = (AST_COND *) node;
					
					/* evaluate expression right before writing ASM_JZ - this to proper reuse register */							
					//check_cond( cond );
					START_CODE_GEN
					
					/*   expression */
					ASM_MERGE_CODE( base, cond->cond );

					/* conditional */
					ASM_ADD( base, 
							ASM_INSTRUCTION_COND_JUMP_ASM( ASM_JZ, 
									get_code(cond->cond)->storage, 
									label ) );

					/* mark condition result for reuse */
					SYM_SCOPE_release_location( vscript_ctx->current_scope , get_code(cond->cond)->storage );


					/* statement executed on condition */
					ASM_MERGE_CODE( base, cond->statement );

					
					/* goto end of if clause (in case there are other elses */
					ASM_ADD( base, 
								ASM_INSTRUCTION_FORWARD_DECL_ASM(ASM_GOTO, label_eof) );

					/* label - get here if condition failed */
					ASM_ADD( base, label);


				} else {
					/* else clause */
					ASM_MERGE_CODE( base, node );
				}
			}
			
			ASM_ADD( base, label_eof );
			}
			break;

		case S_UNTIL:
		case S_WHILE: {
			AST_COND *cond;
			ASM_INSTRUCTION *label_start, *label_eof; 


			/* code gen */
			START_CODE_GEN

			cond = (AST_COND *) base;
				
			label_eof = ASM_INSTRUCTION_PC( ASM_LABEL, 0);
			label_start = ASM_INSTRUCTION_PC( ASM_LABEL, 0);

			/* break / continue statements will use the following ref */
			/*
			cond->label = label_eof;
			cond->label_start = label_start;
			*/


			/* label at start of loop */
			ASM_ADD( base, label_start);

			if (base->type == S_WHILE) {

				/* evaluate expression right before writing ASM_JNZ - this to proper reuse register */							
				//check_cond( cond );
				
				ASM_MERGE_CODE( base, cond->cond );
				
				/* conditional */
				ASM_ADD( base, 
						ASM_INSTRUCTION_COND_JUMP_ASM( ASM_JZ, 
								get_code(cond->cond)->storage, 
								label_eof ) );
 				/* mark condition result for reuse */
				SYM_SCOPE_release_location( vscript_ctx->current_scope , get_code(cond->cond)->storage );

			}

			/* loop body */
			ASM_MERGE_CODE( base, cond->statement );

			if (base->type == S_UNTIL) {

				/* evaluate expression right before writing ASM_JZ - this to proper reuse register */							
				//check_cond( cond );
				START_CODE_GEN

				/* evaluate expression */
				ASM_MERGE_CODE( base, cond->cond );
			
				/* conditional */
				ASM_ADD( base, 
						ASM_INSTRUCTION_COND_JUMP_ASM( ASM_JZ, 
								get_code(cond->cond)->storage, 
								label_eof) );		

				/* mark condition result for reuse */
				SYM_SCOPE_release_location( vscript_ctx->current_scope , get_code(cond->cond)->storage );

			} 


			/* jump back to start of loop */
			ASM_ADD( base, 
				ASM_INSTRUCTION_FORWARD_DECL_ASM(ASM_GOTO, label_start) );
			
			
			/* label after end of loop */
			ASM_ADD( base, label_eof);

			}
			break;
		

		case S_CONTINUE:
		case S_BREAK: {
			AST_BASE *parent = (AST_BASE *) VTREE_parent( &base->node );

			/* semantic check - is there an inclosing loop statement?  */
			while(parent) {

				if (parent->type == S_WHILE || 
					parent->type == S_UNTIL || 
					parent->type == S_FUN_DECL) {
					break;
				}
				parent = (AST_BASE *) VTREE_parent( &parent->node );
			}

			if (!parent || parent->type == S_FUN_DECL)  {

				do_yyerror(base->location, 
						   "break must be within a while loop, right here it is not the case");

			} else {
			
				ASM_INSTRUCTION *instr;

				START_CODE_GEN


				/* 
					   CODE GEN: while statement is not yet generated, write into op code
					   a reference to enclosing while statement, later this reference
					   must be resolved:
						for S_BREAK to point to instruction after the while statemen
						
				*/
				instr = ASM_INSTRUCTION_FORWARD_DECL_AST(ASM_GOTO, parent, 0 );

				ASM_ADD( base, instr );

				instr->rhs.arg = base->type;

			}
			
			}
			break;

		case S_COND	: {
				/* no code generation - this construct has different meanings depending on context */
			}
			break;



		case S_FUN_CALL: {
			VTREENODE *node;
			AST_FUNCTION_DECL *fdef;
			AST_BASE *ret = 0;
			int  param_count;
			int  is_global = SYM_SCOPE_is_global_scope();

			if (SYM_SCOPE_is_defined_ex(base, &ret, 0)) {
				return; 
			}

 			fdef = (AST_FUNCTION_DECL *) ret;

			START_CODE_GEN

			assert( fdef->super.type == S_FUN_DECL );

			get_code(base)->storage = is_global ? 0 : SYM_SCOPE_make_new_location( vscript_ctx->current_scope );

 			/* merge code for function parameters */
			param_count = 0;
			VTREE_FOREACH_CHILD(node, &base->node) {
				AST_BASE *param = (AST_BASE *) node;
			
				/*
				 code generation for argument expression of function.
				*/
 				semantics_check( param );
				if (is_global) {
					set_expression_target( param, param_count + 1 );
				}
				ASM_MERGE_CODE( base, node );
				param_count++;
			}


			if (VTREE_parent( &fdef->super.node ) == 0) {
				/* *** if AST entry is not in tree then it faked entry - and we have to call an extension method *** */
				ASM_ADD( base, 
					ASM_INSTRUCTION_OP3( ASM_XCALL, fdef->impl.xmethod_id , param_count, 0) );

			} else {
				ASM_ADD( base, 
					ASM_INSTRUCTION_FORWARD_DECL_AST( ASM_CALL, &fdef->super, param_count) );
			}

			/* function parameter expression results can be reused */
			VTREE_FOREACH_CHILD(node, &base->node) {
				AST_BASE *param = (AST_BASE *) node;
				if (param->type == S_EXPRESSION) { // parameter
					SYM_SCOPE_release_location( 
						vscript_ctx->current_scope, get_code(param)->storage );

				}				
			}
			}
			break;
			


		case S_FUN_DECL: {
			AST_FUNCTION_DECL *fdecl;
			VTREENODE *cparam;
			
			fdecl= (AST_FUNCTION_DECL *) base;

			vscript_ctx->current_scope->fdecl = fdecl;
//			register_foward_decls(fdecl->impl.body,0);

			/* merge parameters */
			START_CODE_GEN
		

			VTREE_FOREACH_CHILD(cparam,&base->node) {
				AST_BASE *param = (AST_BASE *) cparam;
				if (param->type == S_VARDEF) {
					ASM_MERGE_CODE( base, param );			
				}
			}					
			/* merge function body */
			ASM_MERGE_CODE (base, fdecl->impl.body );			

			/* don't know if we had a return statement in function body, but
			   here we add one to be sure (may lead to one unreachable instruction though)
			 */
			ASM_ADD(base, 
						ASM_INSTRUCTION_OP3(ASM_RETURN, -1, 0, 0 ) );

			}
			break;


		case S_RETURN: {
			AST_COND  *retnode = (AST_COND *) base;
			
			/* check if condition is a scalar expression */

			/* check if expression type is in agreement with function type */
			if (vscript_ctx->current_scope->fdecl && 
				vscript_ctx->current_scope->fdecl->ret_value != retnode->cond->value_type) {
				do_yyerror(retnode->super.location, 
					"Return statement passes a %s back, but Function returns %s",
					AST_BASE_get_var_type_name( retnode->cond->value_type ),
					AST_BASE_get_var_type_name( vscript_ctx->current_scope->fdecl->ret_value ) );			
			}	


			START_CODE_GEN

			ASM_MERGE_CODE ( base, retnode->cond );			

			ASM_ADD( base, 
						ASM_INSTRUCTION_OP3(ASM_RETURN, 				
							get_code( retnode->cond )->storage, 0, 0 ) );

			}
			break;

		case S_EXPRESSION: {
			AST_EXPRESSION *expr = (AST_EXPRESSION *) base;

			do_process_expression( expr );
			
			}
			break;
	}

#if 0
	AST_BASE_show_node(stderr, base, 1 );
	fprintf(stderr,"\n");
	ASM_write_listing(stderr, &get_code(base)->code);
#endif

next_ast: ;

	if (has_func_decl) {

		VTREE_FOREACH_CHILD(ccur,&base->node) {
			AST_BASE *curnode = (AST_BASE *) ccur;

			if (curnode->type == S_FUN_DECL) {
				SYM_SCOPE_push();
				register_foward_decls(curnode,0);
				semantics_check(curnode); 
				SYM_SCOPE_pop();
			}
		}
	}


}

// 
/*
  Function name - link_contexts
 
  Description - traverse AST tree recursively,
				for each function declaration, append code to global scope (argument root)
				
				links code for all contexts/functions into one sequence of instructions
 
  Input -       
 
  Return Code/Output 
 */
 void link_contexts(AST_BASE *root, AST_BASE *cur_ast)
{
	VTREENODE *cur;

	// add return statement at end of code for global scope (return 0).
	if (cur_ast == root) {
		//add return instruction as last opcode.
		ASM_ADD( root, 
			ASM_INSTRUCTION_OP3(ASM_RETURN, -1, 0, 0 ) );
	}
	
	VTREE_FOREACH_CHILD(cur, &cur_ast->node) {

		AST_BASE *base = (AST_BASE *) cur;

		if (base->type == S_FUN_DECL) {
			ASM_MERGE_CODE( root, base );			
		}
		link_contexts(root, base);
	}
}

/*
  Function name - ASM_number_instructions
 
  Description - given that all instructions have been linked into one list;
				assign a address to each instruction,
				a labels doesn't get an address of its own, as it refers to the instruction
				immedeatly after it.
 
  Input -       
 
  Return Code/Output 
 */
 void ASM_number_instructions(VSLIST *code)
{
	VSLIST_entry *entry;
	ASM_PROGRAM_COUNTER pc = 0;

	VSLIST_FOREACH(entry,code) {
		ASM_INSTRUCTION *instr = (ASM_INSTRUCTION *) entry;

		instr->pc = pc;

		pc = pc + ASM_INSTRUCTION_to_bytecode_size( 
					ASM_INSTRUCTION_get_opcode(instr) );	
		
	}

}


/*
  Function name - ASM_resolve_backrefs
 
  Description - given that all instructions have their address in code segment:
				resolve address of jump instructions, funciton calls
				
				- until now those instructions refer to a given AST node, after this
				functions they refer to a location in code.
				
				(during code generation we call functions, but don't yet know the instruction number of the call;
				here we do know it, so resolve those back references here.)

  Input -       
 
  Return Code/Output 
 */
int ASM_resolve_backrefs(VSLIST *code)
{
	VSLIST_entry *entry;
	int opcode;
	S_TYPE  ty;
	

	VSLIST_FOREACH(entry,code) {
		ASM_INSTRUCTION *instr = (ASM_INSTRUCTION *) entry;

		if ((instr->type & ASM_FORWARD_DECL_MASK) == 0) {
			continue;
		}
  
		// here goes the show
		opcode = ASM_INSTRUCTION_get_opcode( instr );
		
		switch(opcode) {
		case ASM_LABEL:
			break;

		case ASM_GOTO:

			if (instr->type  & ASM_FORWARD_DECL_ASM_RES) {

				ASM_INSTRUCTION_set_forwarded(instr, instr->res.forward_decl_asm->pc);
			} else {
				assert( instr->type  & ASM_FORWARD_DECL_AST_RES );

				ty = instr->res.forward_decl_ast->type;
				
				if (ty == S_WHILE || ty == S_UNTIL) {
					if (instr->rhs.arg == S_BREAK) {
						// break out of loop
						ASM_INSTRUCTION_set_forwarded(instr, get_last_instruction( instr->res.forward_decl_ast )->pc );
					} else if (instr->rhs.arg == S_CONTINUE) {
						// jump to start of loop - to do next iteration
						ASM_INSTRUCTION_set_forwarded(instr, get_first_instruction( instr->res.forward_decl_ast )->pc );
					}

				} else {	
					// jump to target (label forward decl)
					ASM_INSTRUCTION_set_forwarded(instr, get_first_instruction( instr->res.forward_decl_ast )->pc );
				}
			}
			break;

		case ASM_CALL:
			//ASM_INSTRUCTION_FORWARD_DECL_AST ( AST_FUNCTION_DECL )

			assert( instr->type  & ASM_FORWARD_DECL_AST_RES );

			// jump to target (label forward decl)
			ASM_INSTRUCTION_set_forwarded(instr, get_first_instruction( instr->res.forward_decl_ast )->pc );

			break;

		case ASM_JZ:
		case ASM_JNZ:

			assert( instr->type  & ASM_FORWARD_DECL_ASM_RES );

			// conditional jump - already know where from construct
			if (instr->res.forward_decl_asm) {
				ASM_INSTRUCTION_set_forwarded(instr, instr->res.forward_decl_asm->pc );
			}
			break;

		default:
			// only the obove mentioned opcodes should have references
			printf("%d",opcode);
			assert(0);
		}

	}
	return 0;
}

/*
  Function name - MY_YY_code_gen
 
  Description - semantics/type check and intermedeate code generation step.
				
  Input -  root - AST tree as received from parsing stap;     
 
  Return Code/Output - returns 0 if 
 */
int MY_YY_code_gen(AST_BASE *root)
{
	// create global scope
	SYM_SCOPE_push();

	// put function defs and labels into hash, so we find ref to them if not yet declared..
	register_foward_decls(root,1);

	// semantic check and code generation, first for global scope, then foreach function
	semantics_check(root);	

	if (!vscript_ctx->my_yy_is_error) {

		// link code for all contexts/functions into one sequence of instructions
		link_contexts( root, root);

		//add return instruction as last opcode.
		//ASM_ADD( root, 
		//	ASM_INSTRUCTION_OP3(ASM_RETURN, -1, 0, 0 ) );

		// no each instruction will have its own number
		ASM_number_instructions(& get_code(root)->code );

		vscript_ctx->my_code = &get_code(root)->code;

	}
	
    vscript_ctx->count_global_vars = SYM_SCOPE_count_vars( vscript_ctx->global_scope );	
	SYM_SCOPE_pop();

	return vscript_ctx->my_yy_is_error;
}
