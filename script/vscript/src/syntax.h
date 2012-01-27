#ifndef _SYNTAX_H_
#define _SYNTAX_H_

#include "ast.h"
#include "yystype.h"

#include <util/varr.h>
#include <util/vbuckethash.h>
#include <util/vslist.h>

#include <memalloc/vzonealloc.h>

#define ERROR_MSG_LEN 4096

/* *** lexical analysis  *** */

struct tagVSCRIPTCTX; 

typedef struct tagMY_YY_POSITION {
	int file_id;
	int line;
	int column;
} 
	MY_YY_POSITION;

int		MY_YY_open_file(const char *file_name);

typedef enum {
  MY_YY_PARSE_STATUS_OK,
  MY_YY_PARSE_STATUS_SYNTAX_ERROR,
  MY_YY_PARSE_STATUS_NO_MEMORY,
}
  MY_YY_PARSE_STATUS;

int  MY_YY_parse(AST_BASE **root);

const char *MY_YY_get_file_name(int file_index);

void VSCRIPTCTX_init_tokenizer(struct tagVSCRIPTCTX *);

void VSCRIPTCTX_free_tokenizer(struct tagVSCRIPTCTX *);

/*  *** syntax analysis  *** */

void    MY_YY_dump_as_xml(FILE *fp, AST_BASE *);

/*  *** semantics analysis *** */
int		MY_YY_code_gen(AST_BASE *root);

/*  *** link opcodes *** */
int    ASM_resolve_backrefs(VSLIST *code);


/* *** error reporting *** */

void    yyerror(char const *msg);

void	do_yyerror (YYLTYPE loc, const char  *msg, ...);


/***  global context structure *** */

typedef struct tagVSCRIPTCTX {

	/* *** allocator for 'regular' allocations (not part of AST) *** */
	VCONTEXT *ctx;

	/*  *** memory allocator *** */
	VZONEHEAP zone_heap;

	/*  *** error status *** */
	int my_yy_is_error;

	/*  *** lexical analysis *** */
	MY_YY_POSITION cur_location;
	VARR token_buf;
	VARR nested_buffers;
	VARR file_name_table;
	int  open_count;

	/* *** syntax and semantics check *** */
	AST_BASE * my_ast_root;	
	struct tagSYM_SCOPE *current_scope; 
	struct tagSYM_SCOPE *global_scope;
	struct tagXMETHODLIB *xmethods;
	size_t count_global_vars;

	/* *** code gen *** */
	VBUCKETHASH  constant_pool;
	VSLIST constants; // keeps them sorted by storage ids.
	int	next_constant;
	VSLIST  * my_code;

	/* *** extension method library *** */
	struct tagXMETHODLIB *extension_library;

	/* *** trace options *** */
	char *output_dir;
	int	  trace_opts;

} VSCRIPTCTX;

int VSCRIPTCTX_init(VSCRIPTCTX *vscript_ctx, VCONTEXT *ctx);

void VSCRIPTCTX_free(VSCRIPTCTX *vscript_ctx);


extern VSCRIPTCTX *vscript_ctx;

#if 1
/*
   Memory allocator - compiler allocs lots of stuf (AST, BYTE CODE), but most of this
   stuff is needed until the whole process finishes, most stuff (except for symbol table entries
   of scope that has gone) cannot be freed.

   Ideal case for memory allocator that only knows how to alloc, but not how to free stuff.
   Welcome to VZONEHEAP
   
   until whole compilation process 

 */
#define VSCRIPTCTX_ctx (&vscript_ctx->zone_heap.base_interface)

#define VSCRIPTCTX_malloc( x ) V_MALLOC( &vscript_ctx->zone_heap.base_interface, (x) )

#define VSCRIPTCTX_strdup( str ) VUTIL_strdup( &vscript_ctx->zone_heap.base_interface, str )

#else 

#define VSCRIPTCTX_ctx VCONTEXT_get_default_ctx()

#define VSCRIPTCTX_malloc( x ) malloc( x )

#define VSCRIPTCTX_strdup( str ) strdup(str)

#endif

#endif
