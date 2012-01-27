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

#include <util/vstra.h>
#include "vtest.h"

extern VCONTEXT * test_alloc;

void VSTRA_test()
{	
	VSTRA    str,s_copy,s_stack;

	/* init */
	VASSERT( ! VSTRA_init(test_alloc,&str) );

	VASSERT( ! VSTRA_scpy( &str,"abc") );

	VASSERT(  strcmp( VSTRA_cstr(&str), "abc" ) == 0);

	VSTRA_init_stack( &s_stack, 7);


	VASSERT( ! VSTRA_cpy( VNEW_STRING, &s_copy, &str) );

	VASSERT( ! VSTRA_cat( &str, &s_copy) );

	VASSERT(   strcmp( VSTRA_cstr(&str), "abcabc" ) == 0);

	VASSERT( ! VSTRA_cpy( VCOPY_STRING, &s_stack, &s_copy) );

	VASSERT(   strcmp( VSTRA_cstr(&s_copy), VSTRA_cstr(&s_stack) )  == 0);




	/* free */
	VSTRA_free( &str);
	VSTRA_free( &s_copy);
	VSTRA_free( &s_stack);
}

