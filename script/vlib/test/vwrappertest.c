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

#include <wrapper/vwrapper.h>
#include <string.h>
#include "vtest.h"

#define TEST_FILE "../../testfile.txt"

void VWRAPPER_test_mmap()
{
	VOS_MAPFILE map;
	void *mem;

	map = VOS_MAPFILE_open( TEST_FILE, VOS_MAPPING_READ, 0);
	
	VASSERT( map );
	
	mem = VOS_MAPFILE_get_ptr( map );

	VASSERT( memcmp( mem, "123456", 6) == 0) ;
	
	VASSERT( (size_t) VOS_MAPFILE_get_length( map ) == 6 );

	VOS_MAPFILE_close( map );
	
}

