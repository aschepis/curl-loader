
%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scr.tab.h"

%}

%option  noyywrap

%x	 comment

INT_CONST	[0-9]+
ID			[a-zA-Z][a-zA-Z0-9_]*
DOUBLE		[0-9]+"."[0-9]+

%%


";"		{ MY_YY_RETURN(TK_SEMICOLON); }

":"		{ MY_YY_RETURN(TK_COLON); }

","		{ MY_YY_RETURN(TK_COMMA); }

"("		{ MY_YY_RETURN(TK_PARENTHESES_OPEN); }

")"		{ MY_YY_RETURN(TK_PARENTHESES_CLOSE); }

"{"		{ MY_YY_RETURN(TK_BRACE_OPEN); }

"}"		{ MY_YY_RETURN(TK_BRACE_CLOSE); }

"["		{ MY_YY_RETURN(TK_BRACKET_OPEN); }

"]"		{ MY_YY_RETURN(TK_BRACKET_CLOSE); }

"if"	{ MY_YY_RETURN(TK_IF);	  }

"else"	{ MY_YY_RETURN(TK_ELSE); }

"elsif"	{ MY_YY_RETURN(TK_ELSIF); }  

"end"	{ MY_YY_RETURN(TK_END);  }

"goto"	{ MY_YY_RETURN(TK_GOTO); }

"sub"	{ MY_YY_RETURN(TK_SUB);  }

"include" { MY_YY_RETURN(TK_INCLUDE); }

"=="	{ MY_YY_RETURN(TK_OP_NUM_EQ); }

"="		{ MY_YY_RETURN(TK_ASSIGN);    }

"!="	{ MY_YY_RETURN(TK_OP_NUM_NE); }

"<"		{ MY_YY_RETURN(TK_OP_NUM_LT); }

">"		{ MY_YY_RETURN(TK_OP_NUM_GT); } 

"<="	{ MY_YY_RETURN(TK_OP_NUM_LE); }

">="	{ MY_YY_RETURN(TK_OP_NUM_GE); }

"eq"	{ MY_YY_RETURN(TK_OP_STR_EQ); }

"ne"	{ MY_YY_RETURN(TK_OP_STR_NE); }

"lt"	{ MY_YY_RETURN(TK_OP_STR_LT); }

"gt"	{ MY_YY_RETURN(TK_OP_STR_GT); }

"le"	{ MY_YY_RETURN(TK_OP_STR_LE); }

"ge"	{ MY_YY_RETURN(TK_OP_STR_GE); }

".."	{ MY_YY_RETURN(TK_OP_STR_CAT); }

"."     { MY_YY_RETURN(TK_OK_DOT); }

"=~"	{ MY_YY_RETURN(TK_OP_STR_REGEXMATCH); }

"+"		{ MY_YY_RETURN(TK_OP_NUM_ADD); }

"-"		{ MY_YY_RETURN(TK_OP_NUM_SUBST); }

"*"		{ MY_YY_RETURN(TK_OP_NUM_MULT); }

"%"		{ MY_YY_RETURN(TK_OP_NUM_MOD); }  

"/"		{ MY_YY_RETURN(TK_OP_NUM_DIV); }

"++"	{ MY_YY_RETURN(TK_OP_NUM_AUTOINCR); }

"--"	{ MY_YY_RETURN(TK_OP_NUM_AUTODECR); }

"str"	{ MY_YY_RETURN(TK_OP_TOSTR); }

"num"	{ MY_YY_RETURN(TK_OP_TOINT); }

"var"	{ MY_YY_RETURN(TK_VAR_DEF); }

"undef" { MY_YY_RETURN(TK_VAR_UNDEF); }

"return" { MY_YY_RETURN(TK_RETURN); }

"while" { MY_YY_RETURN(TK_WHILE); }

"until" { MY_YY_RETURN(TK_UNTIL); }

"break" { MY_YY_RETURN(TK_BREAK); }

"continue" { MY_YY_RETURN(TK_CONTINUE); }

"do"	{ MY_YY_RETURN(TK_DO); }

"@"		{ MY_YY_RETURN(TK_ARR_DEF); }
T
"#"		{ MY_YY_RETURN(TK_CODEREF_DEF); }

"class" { MY_YY_RETURN(TK_CLASS); }

"interface" { MY_YY_RETURN(TK_INTERFACE); }



{ID}	{ 
			  yylval.string_value = strdup(yytext); 
			  MY_YY_RETURN(TK_ID); 
		}	

{INT_CONST} { 
		yylval.long_value = atol(yytext);
		MY_YY_RETURN(TK_INT_CONSTANT); 
	}

{DOUBLE} {
		yylval.double_value = atof(yytext);
		MY_YY_RETURN(TK_DOUBLE_CONSTANT);
	}

"\"" {
		int c;
		

		VARR_reset( &vscript_ctx->token_buf );
		
		
		while ( (c = MY_YY_input()) != '"' && c != EOF) {
			if (c == '\\') {
			   c = MY_YY_input();
			   if (c==EOF) {
				 break;
			   }
			   if (c=='\n') {
				 continue;
			   }
				switch(c) {
			     case 'n': 
				   c = '\n';
				   break;
				 case 't':
				   c = '\t';
				   break;
				 case 'v':
				   c = '\v';
				   break;
				 case 'b':
				   c = '\r';
				   break;
				 case 'r':
				   c = '\r';
				   break;
				 case 'f':
				   c = '\f';
				   break;
				 case 'a':
				   c = '\a';
				   break;
				}			   			   			   
			} else {
			if (c == '\n') {
			   /*TODO: write where string constant started. */			  
			   yyerror( "string constant is not closed until the end of file" );
			   return TK_ERROR;
			}
			}
			// add c to returned input
			VARR_push_back( &vscript_ctx->token_buf, &c, sizeof(char) );
		}
			
		if (c == '"') {

		  c = 0;
		  VARR_push_back( &vscript_ctx->token_buf, &c, sizeof(char) );

		  yylval.string_value = strdup( (char *) vscript_ctx->token_buf.buffer );
		  return TK_STRING_CONSTANT;
		}

		return TK_ERROR;
	}
			
"/*" {
		int c;

		while ( (c = MY_YY_input()) != EOF )  {
			if (c == '\\') {
			  c = MY_YY_input();
			  if (c == EOF) {
				break;
			  }
			} else if ( c == '*' ) {
			  c = MY_YY_input();	
			  if (c == EOF) {
				break;
			  }
			  if ( c == '/' )
				break;    /* found the end */
			  }
           
		 }
		 if ( c == EOF ) {
			  /*TODO: write where comment started. */			  
			  yyerror( "comment is not closed until the end of file" );
			  return TK_ERROR;                 
		 }
	 }


"//"[^\n]*\n  { /* eat up single line comments */
			MY_YY_NEWLINE;
		}


[ \t]+	{
			MY_YY_NEXT_CHAR;
		}


[\n]+	{
			MY_YY_NEXT_LINES;
		}	 

<<EOF>> {
			if (MY_YY_close_file()) {
				yyterminate();
			}
		}
