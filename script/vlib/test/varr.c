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

#include <util/varr.h>
#include "vtest.h"


extern VCONTEXT * test_alloc;

void VARR_test()
{
	VARR arr;
	int  elm;

	VASSERT( ! VARR_init( test_alloc, &arr, sizeof(int), 0 ) );

	elm = 1;
	VASSERT( ! VARR_push_back( &arr, &elm, sizeof(elm) ) );

	elm = 3;
	VASSERT( ! VARR_push_back( &arr, &elm, sizeof(elm) ) );

	elm = 4;
	VASSERT( ! VARR_push_back( &arr, &elm, sizeof(elm) ) );

	elm = 2;
	VASSERT( ! VARR_insert_at( &arr, 1, &elm, sizeof(elm) ) );


	VASSERT( VARR_maxsize(&arr) == 6);

	VASSERT (  VARR_size(&arr) == 4 );
	VASSERT ( *( (int *) VARR_at( &arr, 0)) == 1 );
	VASSERT ( *( (int *) VARR_at( &arr, 1)) == 2 );
	VASSERT ( *( (int *) VARR_at( &arr, 2)) == 3 );
	VASSERT ( *( (int *) VARR_at( &arr, 3)) == 4 );

	
	VASSERT( ! VARR_delete_at( &arr, 1 ) );
	VASSERT( ! VARR_pop_back( &arr, &elm, sizeof(int) ) );

	VASSERT (  VARR_size(&arr) == 2 );
	VASSERT ( *( (int *) VARR_at( &arr, 0)) == 1 );
	VASSERT ( *( (int *) VARR_at( &arr, 1)) == 3 );


	VARR_free( &arr );


}

