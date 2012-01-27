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

#include <string.h>
#include <stdio.h>
#ifdef _WIN32_WCE
#include <windows.h>
#endif

#include <util/vsring.h>
#include <util/vbuckethash.h>
#include <util/vdring.h>
#include <util/vbuffer.h>

#include "vtest.h"
#include "vtestcui.h"

void VSRING_test();
void VDRING_test();
void VDLIST_test();
void VSLIST_test();
void VBUCKETHASH_test();
void VCLOSEDHASH_test();
void VSPARSEARRAY_test();
void VBUFFER_test();
void VHEAP_test();
void VCIRCBUF_test();
void VBITVECTOR_test();
void VSTRA_test();
void VARR_test();
void VTREE_test();
void VTREE_test_unlink();
void VDLISTUNROLLED_test();
void VDLISTUNROLLED_test_insert();


void VMEMORY_test_page_alloc();
void VMEMORY_test_fixed_alloc();
void VMEMORY_zone_alloc();
void VMEMORY_track_alloc();

void VWRAPPER_test_mmap();


#include <util/vstra.h>


VCONTEXT * test_alloc = 0;


VTEST_DEFINE_SUITE( DATASTRCTTEST, 0, 0, MEMALLOCTEST)

	/* list */
	VTEST_TEST( "SRing",	   VSRING_test)
	VTEST_TEST( "DRing",	   VDRING_test)
	VTEST_TEST( "SList",	   VSLIST_test)
	VTEST_TEST( "DList",	   VDLIST_test)
	
	VTEST_TEST( "UnrolDList",  VDLISTUNROLLED_test)
	VTEST_TEST( "UnrolDList2", VDLISTUNROLLED_test_insert)

	/* sequences */
	VTEST_TEST( "ArrayTest",   VARR_test)
	VTEST_TEST( "CircualBuff", VCIRCBUF_test)
	VTEST_TEST( "BitVector",   VBITVECTOR_test)
	VTEST_TEST( "StringTest",  VSTRA_test)
	VTEST_TEST( "Buffer",	   VBUFFER_test)
	
	/* hash */
	VTEST_TEST( "BucketHash",  VBUCKETHASH_test)
	VTEST_TEST( "ClosedHash",  VCLOSEDHASH_test)

	/* sparse data structures */
	VTEST_TEST( "SparseArray", VSPARSEARRAY_test)

	/* trees */
	VTEST_TEST( "Heap",		   VHEAP_test)
	VTEST_TEST( "TreeTest",    VTREE_test)
	VTEST_TEST( "TreeTest2",   VTREE_test_unlink)

VTEST_END_SUITE


VTEST_DEFINE_SUITE( MEMALLOCTEST, 0, 0, WRAPPERTEST)
	VTEST_TEST( "VPAGEALLOC",	VMEMORY_test_page_alloc)
	VTEST_TEST( "VFIXEDSIZE",	VMEMORY_test_fixed_alloc)
	VTEST_TEST( "ZONEALLOC",	VMEMORY_zone_alloc)
	VTEST_TEST( "TRACKALLOC",	VMEMORY_track_alloc)
VTEST_END_SUITE


VTEST_DEFINE_LAST_SUITE( WRAPPERTEST, 0, 0)
	VTEST_TEST( "MMAP",	VWRAPPER_test_mmap)
VTEST_END_SUITE

int main(int argc, const char *argv[])
{ 
    VTEST_CUI_test_runner_cmdline( VTEST_SUITE_GET(DATASTRCTTEST), argc-1, argv+1 );
	return 0;
}

