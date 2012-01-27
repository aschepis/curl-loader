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

#include <util/vsparsearray.h>
#include "vtest.h"

extern VCONTEXT * test_alloc;

#define ENTRIES_PER_NODE 1024
#define ARRAY_ENTRIES (1024 * 1000)

void VSPARSEARRAY_test()
{
	VSPARSEARRAY arr;
	size_t i, last_i;
	size_t *ptr = 0,last;
	V_UINT8 *data;

	VASSERT( ! VSPARSEARRAY_init( test_alloc, &arr, sizeof( size_t ), ARRAY_ENTRIES, ENTRIES_PER_NODE) );

	VASSERT( VSPARSEARRAY_check( &arr ) );

	for(i = ENTRIES_PER_NODE / 2; i < (3 * ENTRIES_PER_NODE); i += 2 ) {
		VASSERT( ! VSPARSEARRAY_set( &arr, i, &i, sizeof( size_t) ) );
		VASSERT( VSPARSEARRAY_check( &arr ) );
	}
	VASSERT( VSPARSEARRAY_check( &arr ) );
	
	for(i = (ENTRIES_PER_NODE / 2); i < (3 * ENTRIES_PER_NODE); i += 1 ) {

		if ((i & 1) == 0) {
			VASSERT( VSPARSEARRAY_isset( &arr, i ) );
			VASSERT( ! VSPARSEARRAY_get( &arr, i, (void **) &ptr, 0 ) );
			VASSERT( *ptr == i );
		} else {
			VASSERT( ! VSPARSEARRAY_isset( &arr, i ) );
		}
	}

    for(i = (ENTRIES_PER_NODE / 2) + 1; i < (3 * ENTRIES_PER_NODE); i += 2 ) {
		VASSERT( ! VSPARSEARRAY_set( &arr, i, &i, sizeof( size_t) ) );
	}

    for(i = (ENTRIES_PER_NODE / 2); i < (3 * ENTRIES_PER_NODE); i += 1 ) {
		VASSERT( VSPARSEARRAY_isset( &arr, i ) );
		VASSERT( ! VSPARSEARRAY_get( &arr, i, (void **) &ptr, 0 ) );
		VASSERT( *ptr == i );
	}

	last = (size_t) -1;
	last_i = 0;
	VSPARSEARRAY_FOREACH( i, data, &arr )

		ptr = (size_t *) data;
		if (last != (size_t) -1) {
			VASSERT( (last + 1) == *ptr );
		}
		last = *ptr;
		
		VASSERT( last_i < i);
		last_i = i;
	VSPARSEARRAY_FOREACH_END
	VASSERT( *ptr == ((3 * ENTRIES_PER_NODE) - 1) );


    for(i = (ENTRIES_PER_NODE / 2) + 1; i < (3 * ENTRIES_PER_NODE); i += 2 ) {

		VASSERT( ! VSPARSEARRAY_get( &arr, i, (void **)  &ptr, 0 ) );
		*ptr += 1;
		VASSERT( ! VSPARSEARRAY_set( &arr, i, ptr, sizeof( size_t) ) );
	}

    for(i = (ENTRIES_PER_NODE / 2); i < (3 * ENTRIES_PER_NODE); i += 1 ) {
		VASSERT( VSPARSEARRAY_isset( &arr, i ) );
		VASSERT( ! VSPARSEARRAY_get( &arr, i, (void **) &ptr, 0 ) );

		VASSERT( *ptr == i + (i & 1) );
	}

	VSPARSEARRAY_free(&arr);

}

