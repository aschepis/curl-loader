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
#include "yystype.h"
#include <util/varr.h>
#include <stdio.h>
#include <stdlib.h>
#include "syntax.h"



/* ****************************************************************************** 
	   support for BISON Locations - declarations

  bison allows you to track position of token (row + column start + column end)
  this feature is not quire enabled by default, so here are some helpers.
 * ****************************************************************************** */


void MY_YY_begin_token(char *text);
int  MY_YY_get_next(char *buff, int max_buffer);


/*
	Invoked from lex file: advance current column counter by one length of current token.
 */
#define MY_YY_NEXT_CHAR \
		vscript_ctx->cur_location.column += yyleng;

/*
	Invoked from lex file when parsing sequence of newlines: 
	advance current line by length of token..
 */
#define MY_YY_NEXT_LINES \
		vscript_ctx->cur_location.column = 1; \
		vscript_ctx->cur_location.line += yyleng;

/*
	Invoked from lex file when parsing one line comments: 
	advance current line by length of token..
 */
#define MY_YY_NEWLINE \
		vscript_ctx->cur_location.column = 1; \
		vscript_ctx->cur_location.line += 1;

/*
	Invoked from lex file when parsing one token: 
	  - advance current column counter by one length of current token
	  - copy position of current token in bison global yyloc (required for position tracking)
*/
#if 1

#define MY_YY_RETURN(x) \
	MY_YY_NEXT_CHAR \
	MY_YY_begin_token( yytext ); \
	return(x);

#else 

#define MY_YY_RETURN(x) \
	MY_YY_begin_token( yytext ); \
	fprintf(stdout,"\n!->Token %s(%d) (%d,%d)-(%d,%d)\n", yytext, x,  yylloc.first_line,yylloc.first_column,yylloc.last_line,yylloc.last_column ); fflush(stdout); \
	return(x);

#endif

int MY_YY_input();


/* ****************************************************** 
		nested file support - declarations
 * ****************************************************** */

int MY_YY_close_file();

/* ****************************************************** 
					FLEX parsers
 * ****************************************************** */

#define ECHO

#define YY_NO_UNPUT

#include "lex.yy.txt"


/* ****************************************************** 
	   support for BISON Locations - after lex include
 * ****************************************************** */

 /*
  Function name - MY_YY_input
 
  Description - get next char of input and keep track of current line number / current column number.
 
  Input -       
 
  Return Code/Output - current input char.
 */
int MY_YY_input()
{
	int c;
	
	while( (c = input()) == 0xD );

	if (c == '\n') {
		MY_YY_NEWLINE;
	} else {
		MY_YY_NEXT_CHAR;
	}
	return c;
}

/*
  Function name - MY_YY_begin_token
 
  Description - copy current position of token into bison global yyloc;
				this is required to support position tracking.
				current position is tracked in global context struct vscript_ctx->cur_location
 
  Input -       
 
  Return Code/Output 
 */
void MY_YY_begin_token(char *text)
{
  yylloc.file_id     = vscript_ctx->cur_location.file_id;

  yylloc.first_line = vscript_ctx->cur_location.line;
  yylloc.last_line	= vscript_ctx->cur_location.line;

  yylloc.first_column = vscript_ctx->cur_location.column - strlen(text);
  yylloc.last_column  = vscript_ctx->cur_location.column - 1;

}


/* ****************************************************** 
		nested file support (include directives)
 * ****************************************************** */

typedef struct tagMY_YY_BUFFER_STATE {
	
	MY_YY_POSITION pos;
	int	file_id;
	YY_BUFFER_STATE state;

} MY_YY_BUFFER_STATE;

#define MAX_INCLUDE_DEPTH 10


	

static char *copy_file_name(const char *file_name)
{
	char *ret = strdup(file_name);
	char *tmp;

	tmp = ret;
	while(*tmp != '\0') {
		if (*tmp == '\\') {
			*tmp = '/';
		}
		tmp ++;
	}
    return ret;
}
	
static int is_file_already_opened(const char *file_name)
{
	unsigned int  i;
	char **ptr;
	char *scopy = copy_file_name(file_name);

	for(i=0; i<VARR_size(&vscript_ctx->file_name_table); i++) {
		ptr = (char **) VARR_at( &vscript_ctx->file_name_table, i );

		if (strcmp(*ptr, scopy) == 0) {
			return 1;
		}
	}

	free(scopy);
	return 0;
}

/*
  Function name - MY_YY_open_file
 
  Description - open a source file.
				supports nested include directives, 
				i.e. maintains a stack of currently parsed files; when finishing with nested file, 
				then we return to file that had include directive.

				therefore each open file has its numeric id, this id is kept as part of token position.
				 
  Input -       
 
  Return Code/Output 
 */
int MY_YY_open_file(const char *file_name)
{
	MY_YY_BUFFER_STATE state;
	char *scopy;
	FILE *fp;

	vscript_ctx->open_count++;

	/* is this file already opened ? can't a.v:include b.v -> b.v:include a.v
       this is not an error though.
	 */
	if (is_file_already_opened( file_name)) {
		return 1;
	}

	/* check if file can be opened before pushing include stack */
	fp = fopen( file_name, "r" );
	if (!fp) {
		return -1;
	}
	

	/* enter file name into file name table */
	scopy = copy_file_name(file_name);
	
	VARR_push_back( &vscript_ctx->file_name_table, (void *) &scopy , sizeof(char *) );

	/* first time open - just use this file descriptor */
	if (vscript_ctx->open_count == 1) {
		yyin = fp;
		return 0;
	}
	
	/* insert buffer into stack; so that we can return to current file contex, once we are finished */
	state.state = YY_CURRENT_BUFFER;
	state.pos  = vscript_ctx->cur_location;

	vscript_ctx->cur_location.file_id = state.file_id = VARR_size( &vscript_ctx->file_name_table ) - 1;
	vscript_ctx->cur_location.line = vscript_ctx->cur_location.column = 1;

	VARR_push_back( &vscript_ctx->nested_buffers, & state, sizeof(MY_YY_BUFFER_STATE) );

	yyin = fp;
	
	/* switch lex buffers */
	yy_switch_to_buffer( yy_create_buffer( yyin, YY_BUF_SIZE ) );


	BEGIN(INITIAL);

	return 0;
}

/*
  Function name - MY_YY_get_file_name
 
  Description - given a file id, return the file name of that file.
 
  Input -       
 
  Return Code/Output 
 */const char *MY_YY_get_file_name(int file_index)
{
	 char **ptr = (char **) VARR_at( &vscript_ctx->file_name_table, file_index );
	 return *ptr;
}
	
int MY_YY_close_file()
{
	if (VARR_size( &vscript_ctx->nested_buffers ) > 0) {

		MY_YY_BUFFER_STATE state;
		
		
		VARR_pop_back( &vscript_ctx->nested_buffers, &state, sizeof(MY_YY_BUFFER_STATE) );
		
        yy_delete_buffer( YY_CURRENT_BUFFER );
        yy_switch_to_buffer(  state.state );

		vscript_ctx->cur_location = state.pos;


	} else  {

	   VARR_free( &vscript_ctx->nested_buffers );
       return -1;
    }  
    return 0;
}


void yyrestart(FILE *);


typedef struct {
	int op;
	char *val;
} INT2STR;


/* 
	map token id of operator token to a xml display string  (i.e. can be part of xml)
 */
static INT2STR token_name[] = {
	{ TK_OP_NUM_EQ, "==" },
	{ TK_OP_NUM_NE, "!=" },
	{ TK_OP_NUM_LT, "&lt;" },
	{ TK_OP_NUM_GT, "&gt;" }, 
	{ TK_OP_NUM_LE, "&lt;=" },
	{ TK_OP_NUM_GE, "&gt;=" },
	{ TK_OP_STR_EQ, "EQ" },
	{ TK_OP_STR_NE, "NE" },
	{ TK_OP_STR_LT, "LT" },
	{ TK_OP_STR_GT, "GT" },
	{ TK_OP_STR_LE, "LE" },
	{ TK_OP_STR_GE, "GE" },
	{ TK_OP_STR_CAT,".." },
	{ TK_OP_NUM_ADD,	"+" },
	{ TK_OP_NUM_SUBST,"-" },
	{ TK_OP_NUM_DIV,	"/" },
	{ TK_OP_NUM_MULT,	"*" },
	{ TK_OP_NUM_MOD,	"%" },
	{ TK_OP_NUM_AUTOINCR,"++" },
	{ TK_OP_NUM_AUTODECR,"--" },
	{ TK_OP_TOSTR,"TOSTR" },
	{ TK_OP_TOINT,"TOINT" },
	{ TK_OP_STR_REGEXMATCH,"~=" },
	{ -1,0 }
	};

/*
  Function name - MY_YY_get_op_token_name
 
  Description - for token id of operator token: return a xml display string  (i.e. can be part of xml)
 
  Input -       
 
  Return Code/Output 
 */
const char *MY_YY_get_op_token_name(int token)
{
	int i;

	for(i = 0; token_name[ i ].op != -1; i++ ) {
		if (token_name[i].op == token) {
			return token_name[i].val;
		}
	}
	return "<undefined>";
}


/*
  Function name - VSCRIPTCTX_init_tokenizer 
 
  Description - initialise tokeniser
 
  Input -       
 
  Return Code/Output 
 */
void VSCRIPTCTX_init_tokenizer(struct tagVSCRIPTCTX *vscript_ctx)
{
	vscript_ctx->cur_location.file_id = 0;
	vscript_ctx->cur_location.line = 1;
	vscript_ctx->cur_location.column = 1;

	VARR_init( 0, &vscript_ctx->token_buf, sizeof(char), 0 ) ;

	VARR_init( 0, &vscript_ctx->nested_buffers, sizeof(MY_YY_BUFFER_STATE), 0 ) ;
	VARR_init( 0, &vscript_ctx->file_name_table, sizeof(char *), 0 );
}

/*
  Function name - VSCRIPTCTX_free_tokenizer
 
  Description - free tokeniser
 
  Input -       
 
  Return Code/Output 
 */
void VSCRIPTCTX_free_tokenizer(struct tagVSCRIPTCTX *vscript_ctx)
{
	VARR_free( &vscript_ctx->token_buf ) ;
	VARR_free( &vscript_ctx->nested_buffers ) ;
	VARR_free( &vscript_ctx->file_name_table );
}
