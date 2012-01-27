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
#include <stdio.h>
#include <stdlib.h>
#include "syntax.h"
#include "ast.h"
#include "asm.h"
#include "vm.h"
#include "vmvalue.h"
#include "vmconst.h"
#include "exe.h"


#if 0		

#define DbgBreak  __asm { int 3 };

#define Return(x) \
	if (x) { DbgBreak } \
	return (x)

#else 

#define Return(x) \
	return (x)

#define DbgBreak   do {  } while(0);	

#endif

static int EXE_write_initial_header(FILE *fp)
{
	EXE_FILE_HEADER exe;
	EXE_SECTION_DESCRIPTOR hdrs[ EXE_HDR_MAX_HEADER ];
	int i;

	exe.magic_constant = EXE_HEADER_MAGIC_CONSTANT;
	exe.version_number = EXE_VERSION;
	exe.hdrs_count	   = EXE_HDR_MAX_HEADER;
	
	if (fwrite( &exe, 1, sizeof(EXE_FILE_HEADER), fp) != sizeof(EXE_FILE_HEADER)) {
		Return(-1);
	}

	for(i = 0; i < EXE_HDR_MAX_HEADER;i++) {
		hdrs[i].EXE_header_type_num = i;
		hdrs[i].header_rva = 0;
		hdrs[i].header_size = 0;
	}
	if (fwrite( &hdrs, 1, sizeof(hdrs), fp) != sizeof(hdrs)) {
		Return(-1);
	}
	fflush( fp );
	return 0;
}

static int align_next_offset(FILE *fp)
{
	 int pos;

	 pos = (ftell(fp) & 3);

	 if (pos!= 0) {
		unsigned long align = 0xABABABAB;

		if (fwrite( &align, 1, pos , fp) != (size_t) pos) {
			Return(-1);
		}
		fflush( fp );
	 }
	 return ftell(fp);
}

static int update_segment_size(FILE *fp, int segment_num, V_UINT32 segment_size, V_UINT32 segment_offset)
{
	//off_t pos;
	int old_pos, pos;
	EXE_SECTION_DESCRIPTOR new_hdr;


	fflush( fp );

	old_pos = ftell(fp);
	if (old_pos == -1) {
		Return(-1);
	}

	new_hdr.EXE_header_type_num = segment_num;
	new_hdr.header_rva = segment_offset,
	new_hdr.header_size = segment_size;

	pos = sizeof(EXE_FILE_HEADER) + (sizeof(EXE_SECTION_DESCRIPTOR) * segment_num);
	

	if ( fseek(fp, pos, SEEK_SET) ) {
		Return(-1);
	}

	if ( fwrite(&new_hdr, 1, sizeof(EXE_SECTION_DESCRIPTOR), fp ) !=  sizeof(EXE_SECTION_DESCRIPTOR)) {
		Return(-1);
	}	
	fflush( fp );

	if ( fseek(fp, old_pos, SEEK_SET) ) {
		Return(-1);
	}

	return 0;
}

int fwrite_seg(void *buf, size_t sz, FILE *fp, size_t *seg_size)
{

	if (fwrite( buf, 1, sz, fp) != sz) {
		Return(-1);
	}

	*seg_size += (size_t) sz;

	return 0;
}

static int write_code_segment(FILE *fp, struct tagVSCRIPTCTX *ctx)
{
	VSLIST_entry *entry;
	char buf[ VM_MAX_INSTRUCTION_SIZE ];
	int ret;
	size_t seg_size = 0;
	size_t seg_offset;
	EXE_CODE_SECTION_HDR hdr;
	VSLIST *my_code = ctx->my_code;

	seg_offset = align_next_offset(fp);

	//* header with ranges of global variables. * 
	hdr.const_count = ASM_CONSTANT_POOL_size(ctx);  
    hdr.global_count = ctx->count_global_vars;

	if (fwrite_seg( &hdr, sizeof(hdr), fp, &seg_size)) {
		Return(-1);
	}

	fflush(fp);
	
	//* serialize each instrcution *
	VSLIST_FOREACH(entry, my_code) {

		ret = ASM_INSTRUCTION_to_bytecode( 
					(ASM_INSTRUCTION *) entry, buf, sizeof(buf), hdr.const_count, hdr.global_count );
		if (ret < 0) {
			Return(-1);
		}

		if ( ret > 0) {
			if ( fwrite_seg(buf, ret, fp, &seg_size) ) {
				Return(-1);
			}
		}
	}

	if (update_segment_size( fp, EXE_HDR_CODE_SECTION, seg_size, seg_offset)) {
		Return(-1);
	}

	return 0;
}

static int copy_from_temp_file( FILE *fp_tmp, FILE *fp, size_t *seg_size ) 
{
	int rd;
	char buf[512];

	// copy temporary file of constant strings into string area in exe file
	fseek(fp_tmp, 0, SEEK_SET);

	while( (rd = fread(buf, 1, sizeof(buf), fp_tmp)) > 0 ) {
		if ( fwrite_seg( buf, rd, fp, seg_size) ) {
			DbgBreak
			return -1;
		}
	}

	return 0;
}

static int write_string_constant(const char *expr_value, VM_CONSTANT_VALUE *val, FILE *fp_tmp,size_t *str_offset)
{
	size_t length;

	length = strlen( expr_value );

	val->type.base_type = VM_STRING | VM_CONSTANT;
	val->val.offset_value = *str_offset + sizeof(size_t);

	*str_offset += length + 1 + sizeof(size_t);

	// write length of string (right before to string data)
	if (fwrite( &length, 1, sizeof(size_t),  fp_tmp) != sizeof(size_t)) {
		DbgBreak;
		return 1;
	}

	// write string to temporary file of constant string sections.
	if (fwrite(expr_value, 1, length + 1,  fp_tmp ) != length + 1) {
		DbgBreak
		return 1;
	}
	return 0;
}

static int write_constant_data(FILE *fp,  struct tagVSCRIPTCTX *ctx)
{
	size_t seg_size = 0;
	VSLIST_entry *cur;
	size_t str_offset = 0;
	size_t seg_offset; 
	FILE *fp_tmp;

	seg_offset = align_next_offset(fp);

	fp_tmp = tmpfile();
	if (!fp_tmp) {
		Return(-1);
	}
		
	VSLIST_FOREACH( cur, &ctx->constants ) {
		ASM_CONSTANT_ENTRY *centry  = V_MEMBEROF_STC( cur, ASM_CONSTANT_ENTRY, next );
		AST_EXPRESSION *expr = centry->value;

		VM_CONSTANT_VALUE val;

		switch( expr->exp_type ) {
			
			case S_EXPR_INT_CONST:
				
				val.type.base_type = VM_LONG | VM_CONSTANT;
				val.val.long_value = expr->val.int_value;
				
				break;

			case S_EXPR_DOUBLE_CONST: 
		
				val.type.base_type = VM_DOUBLE | VM_CONSTANT;
				val.val.double_value = expr->val.double_value;

				break;

			case S_EXPR_STRING_CONST: {
				size_t length;

				length = strlen(expr->val.string_value);


				if (write_string_constant( expr->val.string_value, &val, fp_tmp, &str_offset ) ) {
					DbgBreak;
					goto err;
				}
				}
				break;

			default:
				DbgBreak
				goto err;
		}

#if 0
		// after end of constants - write file table (i.e paths of files of source files)
		for( i = 0; i< VARR_size( &ctx->file_name_table ); i++ ) {
			const char *file_name = MY_YY_get_file_name(i);

			if (write_string_constant( file_name, fp_tmp, &str_offset ) ) {
				DbgBreak;
				goto err;
			}
		}
#endif

		if ( fwrite_seg( &val, sizeof(val), fp, &seg_size) ) {
			DbgBreak
			goto err;
		}

	}

	if (update_segment_size( fp, EXE_HDR_CONSTANT_SECTION, seg_size, seg_offset)) {
		DbgBreak
		goto err;
	}

	// write variable size string area
	seg_size = 0;

	seg_offset = align_next_offset(fp);

	if (copy_from_temp_file( fp_tmp, fp, &seg_size)) {
		goto err;
	}

		
	if (update_segment_size( fp, EXE_HDR_CONSTANT_STRING , seg_size, seg_offset)) {
		Return(-1);
	}

	return 0;

err:
	fclose(fp_tmp);
	Return(-1);
}

#if 0
static int write_debug_info(FILE *fp, AST_BASE *base)
{
	VTREENODE  *curnode,on_disk,tmp;
	AST_BASE  *cbase;
	V_UINT32   currva;
	size_t seg_size = 0;
	size_t seg_offset;
#if 0
	size_t str_offset;
	FILE  *fp_strings, *fp_strings_data;
#endif

	seg_offset = align_next_offset(fp);

#if 0
	fp_strings = tmpfile();
	if (!fp_strings) {
		Return(-1);
	}
	
	fp_strings_data = tmpfile();
	if (!fp_strings_data) {
		fclose(fp_strings_data);
		Return(-1);
	}
#endif
	// each AST entry gets a rva - relative to start of debug section
	// can reuse code generation attribute - no longer needed ?
	VTREE_FOREACH_PREORDER( curnode, &base->node )
		AST_BASE *base = (AST_BASE *) curnode;

		get_code(base)->storage = currva;

		currva += AST_BASE_get_node_size(base->type);
	VTREE_FOREACH_PREORDER_END


	// now that we know address of each element - serialize the whole tree structure onto disk 
	VTREE_FOREACH_PREORDER( curnode, &base->node )
		on_disk = *curnode;

		// UPS: assmumption that sizeof(void *) == unsigned long

		on_disk.parent =  (struct tagVTREENODE *) get_code( on_disk.parent )->storage;
		on_disk.nextlevel_first = (struct tagVTREENODE *) get_code( on_disk.nextlevel_first )->storage;
		on_disk.nextlevel_last = (struct tagVTREENODE *) get_code( on_disk.nextlevel_last )->storage;
		on_disk.right = (struct tagVTREENODE *) get_code( on_disk.right )->storage;
		on_disk.left = (struct tagVTREENODE *) get_code( on_disk.left )->storage;

#if 0
		switch( base->type) {
		  case S_FUN_DECL:
			  on_disk)->

		  case S_FUN_CALL,
		  
			  write_string_constant(
				((AST_FUNCTION_DECL *) on_disk)->name, 
				fp_strings_data,
				&str_offset);

			  ((AST_FUNCTION_DECL *) on_disk)->name = 
			   break;
		  
		  S_FUN_DECL,

		  S_VARDEF,

		  S_VARUNDEF,

		  S_LABEL,
  		
		}
#endif
		tmp = *curnode;
		*curnode = on_disk;

		cbase = (AST_BASE  *) curnode;
		if ( fwrite_seg( curnode, AST_BASE_get_node_size( cbase->type ) , fp, &seg_size) ) {
			Return(-1);
		}
			
		*curnode = tmp;
	VTREE_FOREACH_PREORDER_END

	if (update_segment_size( fp, EXE_HDR_DEBUG_INFO , seg_size, seg_offset)) {
		Return(-1);
	}

	// write debug string sections
#if 0
	seg_offset = align_next_offset(fp);
	seg_size = 0;
	if (copy_from_temp_file( fp_strings, fp, &seg_size )) {
		Return(-1);
	}
	if (update_segment_size( fp, EXE_HDR_DEBUG_CONSTANT_SECTION , seg_size, seg_offset)) {
		Return(-1);
	}

	seg_offset = align_next_offset(fp);
	seg_size = 0;
	if (copy_from_temp_file( fp_strings_data, fp, &seg_size )) {
		Return(-1);
	}
	if (update_segment_size( fp, EXE_HDR_DEBUG_CONSTANT_STRING , seg_size, seg_offset)) {
		Return(-1);
	}


	fclose(fp_strings);
	fclose(fp_strings_data);
#endif

	return 0;
}

// sort by range of addresses.
static int compare_ast_entries_by_pc(const void *lhs, const void *rhs )
{
	ASM_PROGRAM_COUNTER pc_lhs = get_first_instruction(lhs)->pc;
	ASM_PROGRAM_COUNTER pc_rhs = get_first_instruction(rhs)->pc;
	
	if (pc_lhs < pc_rhs) {
		return -1;
	}
	if (pc_lhs > pc_rhs) {
		return 1;
	}

	pc_lhs = get_last_instruction(lhs)->pc;
	pc_rhs = get_last_instruction(rhs)->pc;

	if (pc_lhs < pc_rhs) {
		return -1;
	}
	if (pc_lhs > pc_rhs) {
		return 1;
	}	

	return 0;
}


static int write_debug_line_info(FILE *fp, AST_BASE *base)
{
	VARR lines;
	VTREENODE *curnode;
	size_t i,sz;
	int ret = 0;
	size_t seg_size = 0;
	size_t seg_offset;

	seg_offset = align_next_offset(fp);

	if (VARR_init(0, &lines, sizeof( void * ), 10)) {
		Return(-1);
	}

	// for each AST entry that has some code - generate index into lines.
	VTREE_FOREACH_PREORDER( curnode, &base->node )		
			

		if (! has_code( curnode ) ) {
			continue; 
		}

		VARR_push_back( &lines, &curnode, sizeof(void *) );

	VTREE_FOREACH_PREORDER_END

	// sort the whole entries by pc of generate instruction
	qsort( (void *) lines.buffer, 
	       sizeof( EXE_HDR_DEBUG_LINE_ENTRY ),
		   (size_t) VARR_size(&lines),
		   compare_ast_entries_by_pc);

	sz = VARR_size( &lines );
	for(i = 0; i < sz; i++) {
		AST_BASE *ptr;
		EXE_HDR_DEBUG_LINE_ENTRY hdr;

		ptr = * ((AST_BASE **) VARR_at( &lines, i ) );

		hdr.pc = get_first_instruction(ptr)->pc;
		hdr.dbg_data_rva = get_code(ptr)->storage;
		
		if (fwrite_seg( &hdr, sz, fp, &seg_size) ) {
			ret = -1;
			break;
		}
	}

	if (!ret && update_segment_size( fp, EXE_HDR_DEBUG_LINE_TABLE, seg_size, seg_offset)) {
		ret = -1;
	}

	// cleanup
	VARR_free( &lines );
	
	Return(ret);
}
#endif

int EXE_write_exe_file(FILE *fp, struct tagVSCRIPTCTX *ctx)
{
	if (EXE_write_initial_header(fp)) {
		Return(-1);
	}

	if (write_code_segment(fp,ctx) ||
		write_constant_data(fp,ctx) 
#if 0		
		||
		write_debug_info(fp,ctx->my_ast_root) ||
		write_debug_line_info(fp, ctx->my_ast_root)
#endif		
		) {
		
		Return(-1);
	}

	return 0;
}

int EXE_is_header_ok(void *base)
{
	EXE_FILE_HEADER *hdr = (EXE_FILE_HEADER *) base;

	if (hdr->magic_constant != EXE_HEADER_MAGIC_CONSTANT) {
		Return(-1);
	}
	if (hdr->version_number != EXE_VERSION) {
		Return(-1);
	}
	return 0;
}
