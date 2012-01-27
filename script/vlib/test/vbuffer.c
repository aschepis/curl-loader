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

#include <util/vbuffer.h>
#include <stdio.h>
#include "vtest.h"


#define TEST_SIZE 10

extern VCONTEXT * test_alloc;

void VBUFFER_test()
{
	VBUFFER buf;
	size_t  head_area_size = 3;
	char sbuf[3];
	
	memset(sbuf,1,sizeof(sbuf));

	VASSERT( ! VBUFFER_init( test_alloc, &buf, TEST_SIZE, head_area_size, 0 ) );



	VASSERT( VBUFFER_check( &buf ) );


	VBUFFER_free( &buf );

}

