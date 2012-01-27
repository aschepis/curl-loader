
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
#ifndef _EXE_H_
#define _EXE_H_
#include <util/vcontext.h>

#include <stdio.h>

/* *** exe file format *** */

typedef enum {
	/* code section */
	EXE_HDR_CODE_SECTION,

	/* initialised data section (fixed size entries) */
	EXE_HDR_CONSTANT_SECTION,

	/* initialised data section (string area) */
	EXE_HDR_CONSTANT_STRING,

	/* debug info */
	EXE_HDR_DEBUG_INFO,

	/* line number table */
	EXE_HDR_DEBUG_LINE_TABLE,

	/* strings referenced by debug info (fixed size entries) */
	EXE_HDR_DEBUG_CONSTANT_SECTION,

	/* string referenced by debug info (string area) */
	EXE_HDR_DEBUG_CONSTANT_STRING,

	/* no such header */
	EXE_HDR_MAX_HEADER,

} EXE_header_type;


#define EXE_HEADER_MAGIC_CONSTANT  (0xACDCACDC)
#define EXE_VERSION  (0x00010001)

typedef struct tagEXE_SECTION_DESCRIPTOR {	
  V_UINT8  EXE_header_type_num;
  V_UINT32 header_rva;
  V_UINT32 header_size;

} EXE_SECTION_DESCRIPTOR;



typedef struct tagEXE_FILE_HEADER 
{
	V_UINT32 magic_constant;

	V_UINT32 version_number;
	
	V_UINT8 hdrs_count;
	
	V_UINT32 checksum;
	
	EXE_SECTION_DESCRIPTOR hdrs[0];
}
	EXE_FILE_HEADER;


/* *** header of code section *** */
typedef struct tagEXE_CODE_SECTION_HDR {	
  V_UINT32 const_count;
  V_UINT32 global_count;

} EXE_CODE_SECTION_HDR;

/* *** exe section content - debu line table *** */
typedef struct tagEXE_HDR_DEBUG_LINE_ENTRY {
	V_UINT32 pc;		// high location of code (range is specified by [previous entry].pc_high ... pc_high)
	V_UINT32 dbg_data_rva;  // offset of debug data entry (debug entry data is serialized AST_BASE structure)
}
	EXE_HDR_DEBUG_LINE_ENTRY;


int EXE_write_exe_file(FILE *fp, struct tagVSCRIPTCTX *ctx);

int EXE_is_header_ok(void *hdr);

V_INLINE void *EXE_get_section_ptr(void *hdr, EXE_header_type ty)
{
	EXE_FILE_HEADER *fhdr = ((EXE_FILE_HEADER *) hdr); 
	size_t offset = fhdr->hdrs[ ty ].header_rva;
	return VPTR_ADD( hdr, offset, void * ); 
}

V_INLINE size_t EXE_get_section_size(void *hdr, EXE_header_type ty)
{
	return ((EXE_FILE_HEADER *) hdr)->hdrs[ ty ].header_size; 
}

#endif

