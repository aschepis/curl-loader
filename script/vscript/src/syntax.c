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
#define YYDEBUG 1
#define YYERROR_VERBOSE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <util/varr.h>
#include <util/vsring.h>
#include "yystype.h"
#include "syntax.h"
#include "asm.h"
#include "sym.h"
#include "xlib.h"

VSCRIPTCTX *vscript_ctx;

#include "scr.tab.txt"


#if 0
/*
  Function name -
 
  Description -
 
  Input -       
 
  Return Code/Output 
 */
static void MY_YY_fancy_header(int len) {

    int i;
    fprintf(stdout, "        |");
    for (i=1; i<len; i++)
      if (  i % 10 == 0  )
        fprintf(stderr, ":"); 
      else if (  i % 5 == 0  )
        fprintf(stderr, "+"); 
      else
        fprintf(stderr, ".");
    fprintf(stderr, "\n"); 
}
#endif
/*
  Function name - MY_YY_fancy_error
 
  Description - write error report line to stderr, where the error token is underlined with ^^^ characters
 
  Input -    line			input line
			 line_num		line number
			 column_start   column start of error token
			 column_end     column end of error token
			 
 
  Return Code/Output 
 */
static void MY_YY_fancy_error(const char *line, int line_num, int column_start, int column_end )
{
	int pos;

	fprintf(stderr, " %6d |%s\n", line_num, line);
	fprintf(stderr , "        |");
	for(pos = 0; pos < (column_start-1); pos++) {
		fprintf(stderr,".");
	}
	
	do {
		fprintf(stderr,"^");
	} while( ++pos < column_end);

	fprintf(stderr,"\n");
}
 

/*
  Function name - MY_YY_fancy_error
 
  Description - write error report line to stderr, where the error token is underlined with ^^^ characters
 
  Input -    location location of error (columns, lines)
			 file filename of source file that contains the error
			 
 
  Return Code/Output - 0 - ok, -1 error
 */
static int MY_YY_fancy_error_report(YYLTYPE location, const char *file)
{
	char buf[4096];
	int	 line_num = 1;
	int	 rd;
	VARR line;


	FILE *fp = fopen(file,"r");
	if (!fp) {
		return -1;
	}

	VARR_init( 0, &line, sizeof(char), 0 ) ;

	while ( (rd = fread(buf, 1, sizeof(buf), fp)) > 0) {
		int pos;
		int found = 0;

		for( pos=0; pos < rd; pos++) {
			if (line_num == location.first_line) {
				found = 1;
				break;
			}
			if (buf[ pos ] == '\n')  {
				line_num ++;
			}
		}

		if (found) {
			/* get the whole line into a buffer */

			int finished = 0;

retry:

			for(; pos < rd; pos++) {
				if (buf[pos] == '\n') {
					finished = 1;
					break;
				} 				
				VARR_push_back(&line,& buf[pos], sizeof(char) );
			}

			if (finished) {
				char ch;

display:
				ch = 0;

				VARR_push_back(&line, &ch, sizeof(char) );

				if (location.first_line < location.last_line) {
					location.last_column = strlen((char *) line.buffer);
				}



				MY_YY_fancy_error( (char *) line.buffer, location.first_line, location.first_column, location.last_column );
				

			} else {
				/* end of buffer found */
				if ((rd = fread(buf, 1, sizeof(buf), fp)) <= 0) {
					goto display;
				}
				pos = 0;
				goto retry;
			}



		}

	}

	
	VARR_free( &line );

	fclose(fp);
	return 0;
}

/*
  Function name - do_yyerror
 
  Description - called by yac/lex as well as syntax analysis/code generation to display an error.
 
  Input -    loc location of error (columns, lines, file_id)
			 fomat format spec of error message (variable line error message)
			 
 
  Return Code/Output - none
 */
void do_yyerror (YYLTYPE loc, const char  *format, ...)
{
  char msg[ERROR_MSG_LEN];
  int len;
  va_list ap;

  va_start(ap, format);
  len = 
#ifdef WIN32  
	_vsnprintf
#else
	vsnprintf
#endif
	
	(msg, sizeof(msg) - 1, format, ap);
  va_end(ap);

  msg[len] = '\0';

  vscript_ctx->my_yy_is_error = 1;
  fprintf (stderr, "\n%s(%d): error %s\n", MY_YY_get_file_name( loc.file_id ), loc.first_line, msg);

  
  MY_YY_fancy_error_report( loc, MY_YY_get_file_name(loc.file_id) );
}


/*
  Function name - yyerror
 
  Description - called by yac/lex as to display an error.
 
  Input -    msg - error message text 
			  
  Return Code/Output - none
 */
void yyerror (char const *msg)
{
  do_yyerror( yylloc, msg );
}

/*
  Function name - VSCRIPTCTX_init
 
  Description - initialize script context, script context is passed around compiler through
				different stages of compilation/code generation
 
  Input -    vscript_ctx - context object
			  
  Return Code/Output - 0 ok, !=0 failure
 */
int VSCRIPTCTX_init(VSCRIPTCTX *vscript_ctx, VCONTEXT *ctx)
{
	if (VZONEHEAP_init(0, &vscript_ctx->zone_heap, 4095, sizeof(double))) {
		return -1;
	}

	vscript_ctx->ctx = ctx ? ctx : VCONTEXT_get_default_ctx();
	vscript_ctx->open_count = 0;

	VSCRIPTCTX_init_tokenizer(vscript_ctx);
	
	vscript_ctx->my_yy_is_error = 0;
	vscript_ctx->my_ast_root = 0;

	vscript_ctx->current_scope = 0;
	vscript_ctx->global_scope = 0;


	vscript_ctx->xmethods = 0;

	ASM_CONSTANT_POOL_init(vscript_ctx);
	vscript_ctx->output_dir = 0;;
	vscript_ctx->trace_opts = 0;
	
	return 0;
}


/*
  Function name - VSCRIPTCTX_init
 
  Description - free script context.
 
  Input -    vscript_ctx - context object
			  
  Return Code/Output - none

 */
void VSCRIPTCTX_free(VSCRIPTCTX *vscript_ctx)
{
	VSCRIPTCTX_free_tokenizer( vscript_ctx );
	VZONEHEAP_free( &vscript_ctx->zone_heap );
	ASM_CONSTANT_POOL_free( vscript_ctx );
}

/*
  Function name - MY_YY_parse
 
  Description - parse a source file with flex/bison.
				during this process the AST - abstract syntax treee - is built.
 
  Input -    
			  
  Return Code/Output -  0 - ok, != 0 failure.
						root base node of abstract syntax tree.
					   

 */
int MY_YY_parse(AST_BASE **root)
{
	int ret = 0;

#if 0
	yydebug = 1;
#endif
	ret = yyparse();

	if (ret == 0 && vscript_ctx->my_yy_is_error == 0) {
		*root = vscript_ctx->my_ast_root;		
	}

	if (ret > vscript_ctx->my_yy_is_error) {
		return ret;
	}
	return vscript_ctx->my_yy_is_error;
}

