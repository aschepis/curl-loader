%locations
%debug

%{
void yyerror (char const *);

int yylex (void);


/*
 * extended structure for location info, 
 * we have to redefine the macro that sets location info for one non terminal
 */
#define YYLLOC_DEFAULT(Current, Rhs, N)                                \
         do                                                                  \
           if (N)                                                            \
             {																 \
			   (Current).file_id	  = YYRHSLOC(Rhs, 1).file_id;			 \
               (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;         \
               (Current).first_column = YYRHSLOC(Rhs, 1).first_column;       \
               (Current).last_line    = YYRHSLOC(Rhs, N).last_line;          \
               (Current).last_column  = YYRHSLOC(Rhs, N).last_column;        \
             }                                                               \
           else                                                              \
             {                                                               \
			   (Current).file_id	  = YYRHSLOC(Rhs, 0).file_id;			 \
               (Current).first_line   = (Current).last_line   =              \
                 YYRHSLOC(Rhs, 0).last_line;                                 \
               (Current).first_column = (Current).last_column =              \
                 YYRHSLOC(Rhs, 0).last_column;                               \
             }                                                               \
         while (0)

/*
 * debug: assert tree structure
 */
#define CHECK_TREE(arg) \
	if (! VTREE_check_tree( arg) ) { \
		yyerror( "Internal error: Invalid parse tree" ); \
	}

#define GET_TREE(arg) (& (((AST_BASE *) (arg)) -> node) )


#define ADD_NODE(root ,child) \
	CHECK_TREE( GET_TREE( child) ); \
	VTREE_insert_child( GET_TREE( root) , GET_TREE(child), VTREE_INSERT_LAST, 0 );\
	CHECK_TREE( GET_TREE( root) );
	

AST_BASE * ADD_NODE_LIST(AST_BASE *lhs, AST_BASE *rhs)
{
	CHECK_TREE( GET_TREE(lhs) );
	CHECK_TREE( GET_TREE(rhs) );
	
	if (GET_TREE(lhs) -> right == 0) {
		GET_TREE(lhs) -> right = GET_TREE(lhs);			
	} 
	
	GET_TREE(rhs) -> right = GET_TREE(lhs) -> right;
	GET_TREE(lhs) -> right = &rhs->node;
	
	return rhs;
}


AST_BASE *MERGE_LIST( AST_BASE *base, AST_BASE *rhs) 
{
	if (rhs) {
		
		AST_BASE *pos, *next;
		
		next = pos = (AST_BASE *) GET_TREE(rhs)->right; 		
		
		if (next) {
			do {
				
				AST_BASE *insert = next;
				
				next = (AST_BASE *) GET_TREE(next)->right;

				GET_TREE(insert)->right = 0;

				ADD_NODE( base, insert );

			} while(next && next != pos);
			
			return pos;
		} else {
			ADD_NODE( base, rhs);
			
			return rhs;
		}
	}
	return 0;
}

%} 


%token TK_ERROR



%left  TK_WHILE TK_UNTIL TK_IF TK_DO TK_ASSIGN TK_ELSE TK_ELSIF TK_END TK_RETURN TK_BREAK TK_CONTINUE TK_ID TK_INT_CONSTANT TK_STRING_CONSTANT TK_DOUBLE_CONSTANT TK_GOTO TK_SUB TK_INCLUDE 

%token TK_OP_NUM_EQ TK_OP_NUM_NE TK_OP_NUM_LT TK_OP_NUM_GT TK_OP_NUM_LE TK_OP_NUM_GE TK_OK_DOT

%token TK_OP_STR_EQ TK_OP_STR_NE TK_OP_STR_LT TK_OP_STR_GT TK_OP_STR_LE TK_OP_STR_GE

%token TK_OP_STR_CAT

%token TK_OP_NUM_ADD TK_OP_NUM_SUBST TK_OP_NUM_DIV TK_OP_NUM_MULT TK_OP_NUM_MOD  

%token TK_OP_NUM_AUTOINCR TK_OP_NUM_AUTODECR

%token TK_OP_TOSTR TK_OP_TOINT

%token TK_VAR_DEF TK_VAR_UNDEF TK_ARR_DEF TK_CODEREF_DEF 

%token TK_OP_STR_REGEXMATCH 

%token TK_COLON TK_SEMICOLON TK_COMMA TK_PARENTHESES_OPEN TK_PARENTHESES_CLOSE TK_BRACE_OPEN TK_BRACE_CLOSE  TK_BRACKET_OPEN  TK_BRACKET_CLOSE TK_CLASS TK_INTERFACE



%%


/* *** start symbol *** */
prog : stmtList	
	{ 
		vscript_ctx->my_ast_root = $<ast>1;		 
	}
		;

stmtList :
		stmtList optColon stmt		 
	{
		AST_BASE *ret;

		if ( $<ast>3 ) {
			if ($<ast>1 -> type == S_STATEMENT_LIST) {
				ret = $<ast>1;
			} else {
				ret = AST_BASE_new(S_STATEMENT_LIST, yyloc );

				if ($<ast>1 -> type == S_VARDEF) {
					MERGE_LIST( (AST_BASE *) ret,  $<ast>1 );		
				} else {
					ADD_NODE( ret, $<ast>1 );
				}
			}

			$<ast>$ = ret;

			if ($<ast>3 -> type == S_VARDEF) {
				MERGE_LIST( (AST_BASE *) ret,  $<ast>3 );		
			} else {
				ADD_NODE( ret, $<ast>3 );
			}

		} else {
			$<ast>$ = $<ast>1;
		}
	}
		| stmt 
	{
		$<ast>$ = $<ast>1;
	}
	
		| error TK_SEMICOLON stmt 
	{
		$<ast>$ = $<ast>3;
	}

		;

optColon :  TK_SEMICOLON 
		 |
		 ;


/* *** statement *** */
stmt : labelDefStmt 
	{
		$<ast>$ = $<ast>1;
	}
		| gotoStmt      
	{
		$<ast>$ = $<ast>1;
	}
		| ifStmt			
	{
		$<ast>$ = $<ast>1;
	}
		| includeStmt
	{
		$<ast>$ = $<ast>1;
	}
		| assignStmt		
	{
		$<ast>$ = $<ast>1;
	}
		| varDef
	{
		$<ast>$ = $<ast>1;				
	}
		| varUnDef	
	{
		$<ast>$ = $<ast>1;				
	}		
		| functionCallStmt
	{
		$<ast>$ = $<ast>1;
	}
		| methodCallStmt
	{
		$<ast>$ = $<ast>1;
	}

		| functionDefStmt
	{
		$<ast>$ = $<ast>1;
	}
		| whileStmt
	{
		$<ast>$ = $<ast>1;
	}
		| returnStmt
	{
		$<ast>$ = $<ast>1;
	}
		| breakStmt
	{
		$<ast>$ = $<ast>1;		
	}
		| continueStmt
	{
		$<ast>$ = $<ast>1;
	}
/*
		| classDef
	{
		$<ast>$ = $<ast>1;		
	}
		| interfaceDef
	{
		$<ast>$ = $<ast>1;		
	}
 */
		;     

/* *** interface definition *** */
/*
interfaceDef : TK_INTERFACE  TK_ID baseClassSpec interfaceDefBody TK_END
	{
	}
		;

interfaceDefBody : interfaceDefBody functionPrototypeDecl
	{
	}
		| functionPrototypeDecl
	{
	}
*/		;

/* *** class definition **** */
/*
classDef : TK_CLASS TK_ID baseClassSpec classDefBody TK_END
	

baseClassSpec : TK_COLON baseClassList
	{
	}
		|
	{
	}
		;

baseClassList : baseClassList TK_ID
	{
	}
		| TK_ID
	{
	}
		;

classDefBody : classDefBody classItem
	{
	}
		|
	{
	}
		;

classItem : varDef | functionDefStmt
*/
		;

/* object construction 
newInstanceExpression : TK_NEW functionCallStmt
*/


/* *** function definition **** */
functionDefStmt  : functionPrototypeDecl stmtList TK_END  
	{
		AST_FUNCTION_DECL *ret = (AST_FUNCTION_DECL *) $<ast>1;

		ret->impl.body = $<ast>2;
		ADD_NODE( ret, $<ast>2 );

		$<ast>$ = &ret->super;
	}
		;

functionPrototypeDecl : TK_SUB varDefTypeSpec functionDefName functionParamsDecl 
	{
		AST_FUNCTION_DECL *ret = (AST_FUNCTION_DECL *) AST_BASE_new(S_FUN_DECL, yyloc);

		ret->ret_value = $<long_value>2;
		ret->name = VSCRIPTCTX_strdup( $<string_value>3 );

		ret->impl.body = 0;
		ret->params =(AST_VARDEF *) MERGE_LIST( (AST_BASE *) ret, $<ast>4 );

		ret->num_param = VTREE_count_child_nodes( &ret->super.node );

		$<ast>$ = &ret->super;
	}
		;

functionDefName : TK_ID 
		{
			$<string_value>$ = $<string_value>1;
		}
	| 
		{
			$<string_value>$ = VSCRIPTCTX_strdup(LAMBDA_FUNCTION_NAME );
		}
	;

functionParamsDecl : TK_PARENTHESES_OPEN funcParamDecls TK_PARENTHESES_CLOSE
	{
		$<ast>$ = $<ast>2;
	}
		| TK_PARENTHESES_OPEN TK_PARENTHESES_CLOSE
	{ 
		$<ast>$ = 0;
	}
		;

funcParamDecls : funcParamDecls TK_COMMA oneVarDefNoAssign
	{
		$<ast>$ = ADD_NODE_LIST( $<ast>1, $<ast>3 );
	}
		| oneVarDefNoAssign
	{
		$<ast>$ = $<ast>1;
	}
		;


/* *** return from function statement *** */
returnStmt : TK_RETURN expr
	{
		AST_COND * ret = (AST_COND *) AST_BASE_new(S_RETURN, yyloc);

		ret->cond = (AST_EXPRESSION *) $<ast>2;
		ret->statement = 0;

		$<ast>$ = & ret->super ;

		ADD_NODE( ret, $<ast>2 );
	}
		;

/* *** undefine variables **** */
varUnDef : TK_VAR_UNDEF varUndefList
	{
		$<ast>$ = $<ast>2;
	}
		;

varUndefList : varUndefList TK_COMMA oneVarUndef
	{
		$<ast>$ = ADD_NODE_LIST( $<ast>1, $<ast>3 );
	}
		| oneVarUndef
	{
		$<ast>$ = $<ast>1;
	}
		;

oneVarUndef : TK_ID
	{
		AST_VARDEF *ret = (AST_VARDEF *) AST_BASE_new(S_VARUNDEF, yyloc);

		ret->var_type = 0;
		ret->var_name = VSCRIPTCTX_strdup( $<string_value>1 );
		ret->expr = 0;

		$<ast>$	= &ret->super;
	}
		;
	
/* *** define variables **** */
varDef  : TK_VAR_DEF  varDefList
	{
		$<ast>$ = $<ast>2;
	}
		;

varDefList  : varDefList TK_COMMA oneVarDef
	{
		$<ast>$ = ADD_NODE_LIST( $<ast>1, $<ast>3 );
	}
		 | oneVarDef
	{
		$<ast>$ = $<ast>1;
	}
		;

oneVarDef : oneVarDefNoAssign
	{
		$<ast>$ = $<ast>1;
	}
		| varDefTypeSpec  TK_ID TK_ASSIGN expr 
	{
		AST_VARDEF *ret = (AST_VARDEF *) AST_BASE_new(S_VARDEF, yyloc);

		ret->var_type = $<long_value>1;
		ret->var_name = VSCRIPTCTX_strdup( $<string_value>2 );
		ret->expr = (AST_EXPRESSION *) $<ast>4;

		ADD_NODE( ret, $<ast>4 ); 				

		$<ast>$	= &ret->super;
	}
		;

oneVarDefNoAssign : varDefTypeSpec TK_ID
	{
		AST_VARDEF *ret = (AST_VARDEF *) AST_BASE_new(S_VARDEF, yyloc);

		ret->var_type = $<long_value>1;
		ret->var_name = VSCRIPTCTX_strdup( $<string_value>2 );
		ret->expr = 0;

		$<ast>$	= &ret->super;
	}
		;

varDefTypeSpec :
	{
		$<long_value>$ = S_VAR_SCALAR;
	} 
			| TK_ARR_DEF 
	{
		$<long_value>$ = S_VAR_ARRAY;
	} 
			| TK_OP_NUM_MOD
	{
		$<long_value>$ = S_VAR_HASH;
	} 
			| TK_CODEREF_DEF
	{
		$<long_value>$ = S_VAR_CODEREF;
	} 
		;

/* *** include statement **** */
includeStmt : TK_INCLUDE TK_STRING_CONSTANT
	{
		int ret = MY_YY_open_file($<string_value>2);
		 
		if (ret < 0) {
			do_yyerror(yylloc, "Can't open include file %s", $<string_value>2 );
		}
		$<ast>$ = 0;
	}
		;

 
/* *** label definition statement **** */
labelDefStmt : TK_ID TK_COLON  
	{
		AST_LABEL *ret = (AST_LABEL *) AST_BASE_new(S_LABEL, yyloc);

		ret->label_name = VSCRIPTCTX_strdup( $<string_value>1 );

		$<ast>$	= &ret->super;
	}
		;

/* *** goto statement **** */
gotoStmt  : TK_GOTO TK_ID
 	{
		AST_LABEL *ret = (AST_LABEL *) AST_BASE_new(S_GOTO, yyloc);

		ret->label_name = VSCRIPTCTX_strdup( $<string_value>2 );

		$<ast>$	= &ret->super;
	}
		;

/* *** assign statement **** */
assignStmt : varRef TK_ASSIGN expr 
	{
		AST_ASSIGN * ret= (AST_ASSIGN *) AST_BASE_new(S_ASSIGN, yyloc);

		ret->lhs =  (AST_EXPRESSION *) $<ast>1;
		ret->rhs = (AST_EXPRESSION *) $<ast>3;

		ADD_NODE( ret, $<ast>1 ); 				
		ADD_NODE( ret, $<ast>3 ); 				

		$<ast>$ = &ret->super;
	}
           ;

/* *** while statement **** */
whileStmt:	TK_WHILE expr stmtList TK_END
	{
		AST_COND * ret = (AST_COND *) AST_BASE_new(S_WHILE, yyloc);

		ret->cond = (AST_EXPRESSION *) $<ast>2;
		ret->statement = $<ast>3;

		$<ast>$ = & ret->super ;

		ADD_NODE( ret, $<ast>2 );
		ADD_NODE( ret, $<ast>3 );
	}   
		|
	TK_DO stmtList TK_UNTIL expr 
	{
		AST_COND * ret = (AST_COND *) AST_BASE_new(S_UNTIL, yyloc);

		ret->cond = (AST_EXPRESSION *) $<ast>4;
		ret->statement = $<ast>2;

		$<ast>$ = & ret->super ;

		ADD_NODE( ret, $<ast>4 );
		ADD_NODE( ret, $<ast>2 );
	}   
		;


/* *** break out of loop statement *** */
breakStmt : TK_BREAK
	{
		$<ast>$ = AST_BASE_new(S_BREAK, yyloc);
	}
		;

continueStmt : TK_CONTINUE
	{
		$<ast>$ = AST_BASE_new(S_CONTINUE, yyloc);
	}
		;



/* *** if statement **** */
ifStmt : TK_IF condClause elseClauses TK_END
	{          
		AST_IF * ret = (AST_IF *) AST_BASE_new(S_IF, yyloc);
	
		ret->expr = (AST_COND *) $<ast>2;

		ADD_NODE( ret, $<ast>2 );
		ret->first_else = (AST_COND *) MERGE_LIST( (AST_BASE *) ret,  $<ast>3 );		
	
		$<ast>$ = & ret->super;
	}
		  ;

elseClauses : elsifClause TK_ELSE stmtList
	{
		if ($<ast>1) {

			if ($<ast>3) {
				$<ast>$ = ADD_NODE_LIST( $<ast>1 , $<ast>3 );
			} else {
				$<ast>$ = $<ast>1;
			}

		}	else {
			$<ast>$ = $<ast>3;
		}
	}

			| elsifClause 
	{
		$<ast>$ = $<ast>1;
	}
			;

elsifClause : elsifClause TK_ELSIF condClause
	{		
		if ($<ast>3) {	
			if ($<ast>1) { 
				$<ast>$ = ADD_NODE_LIST( $<ast>1 , $<ast>3 );
			} else {
				$<ast>$ = $<ast>3;
			}
		} else {
			$<ast>$ = 0;
		}
	}
		|			
	{
		$<ast>$ = 0;
	}
		;



condClause:	expr stmtList	
	{
		AST_COND * ret = (AST_COND *) AST_BASE_new(S_COND, yyloc);

		ret->cond = (AST_EXPRESSION *) $<ast>1;
		ret->statement = $<ast>2;

		ADD_NODE( ret, $<ast>1 );
		ADD_NODE( ret, $<ast>2 );
	 
		$<ast>$ = & ret->super;
	 }
		;

/* *** method call declatation *** */
methodCallStmt : TK_ID TK_OK_DOT TK_ID TK_PARENTHESES_OPEN functionArgList TK_PARENTHESES_CLOSE
	{
	}
		;	


/* *** function call statement **** */
functionCallStmt : TK_ID TK_PARENTHESES_OPEN functionArgList TK_PARENTHESES_CLOSE
	{
		AST_FUNCTION_CALL *ret = (AST_FUNCTION_CALL *) AST_BASE_new(S_FUN_CALL, yyloc);

		ret->name = VSCRIPTCTX_strdup( $<string_value>1 );

		ret->expression  = (AST_EXPRESSION *) MERGE_LIST( (AST_BASE *) ret, $<ast>3 );
		ret->func_decl = 0;
		$<ast>$ = &ret->super;	
	}
		;


functionArgList  : functionArgList TK_COMMA expr
	{
		$<ast>$ = ADD_NODE_LIST( $<ast>1, $<ast>3 );
	}

		 | expr
	{
		$<ast>$ = $<ast>1;
	}
		 | 
	{
		$<ast>$ = 0;
	}
		 ;


/* *** expression clause **** */
expr : compExp  
	{
		$<ast>$ = $<ast>1;
	}
		;

compExp : compExp  compExpOp addExp
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc);

		ret->exp_type = S_EXPR_BINARY;
		ret->val.param.op = $<int_value>2;
		ret->val.param.var_left = (AST_EXPRESSION *) $<ast>1;
		ret->val.param.var_right = (AST_EXPRESSION *) $<ast>3;

		ADD_NODE( ret, $<ast>1 );
		ADD_NODE( ret, $<ast>3 );

		$<ast>$ = & ret->super;
	} 
		| addExp
	{
		$<ast>$ = $<ast>1;
	}
		;

compExpOp : TK_OP_NUM_EQ 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_NE 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_LT 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_GT 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_LE 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_GE 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_STR_EQ 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_STR_NE 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_STR_LT 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_STR_GT 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_STR_LE 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_STR_GE 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_STR_REGEXMATCH
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	;

addExp  : addExp  addExpOp multExp
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
			
		ret->exp_type = S_EXPR_BINARY;
		ret->val.param.op = $<int_value>2;
		ret->val.param.var_left = (AST_EXPRESSION *) $<ast>1;
		ret->val.param.var_right = (AST_EXPRESSION *) $<ast>3;

		ADD_NODE( ret, $<ast>1 );
		ADD_NODE( ret, $<ast>3 );	

		$<ast>$ = & ret->super;
	} 
		| multExp
	{
		$<ast>$ = $<ast>1;
	}
		;

addExpOp : TK_OP_NUM_SUBST 
		{
			$<int_value>$ = yytoken + 0xFF;
		}		
		| TK_OP_STR_CAT  
		{
			$<int_value>$ = yytoken + 0xFF;
		}
		| TK_OP_NUM_ADD
		{
			$<int_value>$ = yytoken + 0xFF;
		}
		; 

multExp : multExp multExpOp  unaryExp
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type = S_EXPR_BINARY;
		ret->val.param.op = $<int_value>2;
		ret->val.param.var_left = (AST_EXPRESSION *) $<ast>1;
		ret->val.param.var_right = (AST_EXPRESSION *) $<ast>3;

		ADD_NODE( ret, $<ast>1 );
		ADD_NODE( ret, $<ast>3 );

		$<ast>$ = & ret->super;
	} 
		| unaryExp
	{
		$<ast>$ = $<ast>1;
	}
		;

multExpOp : TK_OP_NUM_DIV 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_MULT 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_MOD
		{
			$<int_value>$ = yytoken + 0xFF;
		}
		;

unaryExp :
		  primaryExp unaryExpOpPostfix 
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type = S_EXPR_UNARY;
		ret->val.unary.op = $<long_value>2;
		ret->val.unary.is_prefix = 0;
		ret->val.unary.var = (AST_EXPRESSION *) $<ast>1;

		ADD_NODE( ret, $<ast>1 );

		$<ast>$ = & ret->super;
	}
		| unaryExpOpPrefix primaryExp
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type = S_EXPR_UNARY;
		ret->val.unary.op = $<long_value>1;
		ret->val.unary.is_prefix = 1;
		ret->val.unary.var = (AST_EXPRESSION *) $<ast>2;

		ADD_NODE( ret, $<ast>2 );

		$<ast>$ = & ret->super;
	}
		| primaryExp
	{
		$<ast>$ = $<ast>1;
	}
		;

unaryExpOpPrefix : TK_OP_NUM_AUTOINCR 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_AUTODECR 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_TOSTR 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_TOINT
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_ADD
		{
			$<int_value>$ = yytoken + 0xFF;
		}

		;

unaryExpOpPostfix : TK_OP_NUM_AUTOINCR 
		{
			$<int_value>$ = yytoken + 0xFF;
		}
	| TK_OP_NUM_AUTODECR
		{
			$<int_value>$ = yytoken + 0xFF;
		}
		;				  

primaryExp : varRef
		| TK_INT_CONSTANT
 	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type = S_EXPR_INT_CONST;
		ret->val.int_value = $<long_value>1;

		$<ast>$ = &ret->super;
	}
		| TK_DOUBLE_CONSTANT
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type = S_EXPR_DOUBLE_CONST;
		ret->val.double_value = $<double_value>1;

		$<ast>$ = &ret->super;
	}
		| TK_STRING_CONSTANT
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type = S_EXPR_STRING_CONST;
		ret->val.string_value = VSCRIPTCTX_strdup( $<string_value>1 );

		$<ast>$ = &ret->super;
	}
		| TK_PARENTHESES_OPEN expr TK_PARENTHESES_CLOSE
	{
		$<ast>$ = $<ast>2;
	}
		| functionCallStmt 
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type = S_EXPR_FUNCALL;
		ret->val.fcall = (AST_FUNCTION_CALL *) $<ast>1;

		ADD_NODE( ret, $<ast>1 );

		$<ast>$ = &ret->super;
	}
	   ;

		
varRef : TK_ID
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type   = S_EXRP_SCALAR_REF;
		ret->value_type = S_VAR_SCALAR;

		ret->val.ref.lhs = VSCRIPTCTX_strdup( $<string_value>1 );
		ret->val.ref.index_exp = 0;
		ret->val.ref.var_def = 0;

		$<ast>$ = &ret->super;
	}
		| TK_ID TK_BRACE_OPEN expr TK_BRACE_CLOSE
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type = S_EXPR_HASH_REF;
		ret->value_type	= S_VAR_HASH;

		ret->val.ref.lhs = VSCRIPTCTX_strdup( $<string_value>1 );
		ret->val.ref.index_exp = (AST_EXPRESSION *) $<ast>3;
		ret->val.ref.var_def = 0;

		ADD_NODE( ret, $<ast>3 );

		$<ast>$ = &ret->super;
	}
		| TK_ID TK_BRACKET_OPEN expr TK_BRACKET_CLOSE
	{
		AST_EXPRESSION *ret = (AST_EXPRESSION *) AST_BASE_new(S_EXPRESSION, yyloc); 
	
		ret->exp_type = S_EXPR_ARRAY_REF;
		ret->value_type = S_VAR_ARRAY;

		ret->val.ref.lhs = VSCRIPTCTX_strdup( $<string_value>1 );
		ret->val.ref.index_exp = (AST_EXPRESSION *) $<ast>3;
		ret->val.ref.var_def = 0;

		ADD_NODE( ret, $<ast>3 );

		$<ast>$ = &ret->super;
	}
		;

%%
	

