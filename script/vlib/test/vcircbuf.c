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

#include <util/vcircbuf.h>
#include <stdio.h>
#include "vtest.h"

#define TEST_SIZE 100

extern VCONTEXT * test_alloc;

void VCIRCBUF_test()
{
	VCIRCBUF circbuf;
	int      data, i;


	VASSERT( !VCIRCBUF_init( test_alloc, &circbuf, sizeof(int), TEST_SIZE, V_FALSE, 0, 0, 0) );

    VASSERT( VCIRCBUF_isempty(&circbuf) );

	for(i=0;i<TEST_SIZE;i++) {
		data = i;
		VASSERT( !VCIRCBUF_push( &circbuf, &data, sizeof(data) ) );

		if (i < (TEST_SIZE-1)) {
			VASSERT( !VCIRCBUF_isfull( &circbuf) );
		} else {
			VASSERT( VCIRCBUF_isfull( &circbuf) );
		}
	}

	VASSERT( VCIRCBUF_size(&circbuf) == TEST_SIZE);

	for(i=0;i<TEST_SIZE;i++) {
		VASSERT( !VCIRCBUF_pop( &circbuf, &data, sizeof(data) ) );
		VASSERT(  data == i);
		
		if (i < (TEST_SIZE-1)) {
			VASSERT(  ! VCIRCBUF_isempty( &circbuf) );
		} else {
			VASSERT(  VCIRCBUF_isempty( &circbuf) );
		}
	}

	VCIRCBUF_free(&circbuf);

}
