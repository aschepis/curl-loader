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

#include <util/vbasedefs.h>
#include <util/verror.h>


static char * memory_error_message[] =
{
	"Out of memory",
	"Invalid heap at pointer location",
	"Double free of pointer",
	"Free of pointer that has not been allocated",
	"Can't allocate memory of requested size",
	"General error",
	0
};

V_EXPORT char* VERROR_get_message( int error )
{
	  if (error < 0 || error >= VERROR_LAST) {
		return "Strange error number";	  
	  }
	  return memory_error_message[ error ];
}


