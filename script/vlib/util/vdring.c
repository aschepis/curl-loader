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
#include <util/vdring.h>



V_EXPORT void VDRING_foreach( VDRING *lst, VDRING_VISITOR_V eval, void *data, int save_from_del)
{
    VDRING *cur, *next;

	if (!eval) {
	  return;
	}

	if (save_from_del) {
		VDRING_FOREACH_SAVE( cur, next, lst) {
	   		eval( cur, data );
		}
	} else {
		VDRING_FOREACH(  cur, lst ) {
			eval( cur, data );
		}
	}
		
}


V_EXPORT void VDRING_foreach_reverse( VDRING *lst, VDRING_VISITOR_V eval, void *data, int save_from_delete)
{
	VDRING *cur, *next;

	if (!eval) {
	  return ;
	}

	if ( save_from_delete ) {
		VDRING_FOREACH_REVERSE_SAVE( cur, next, lst ) {

	   		eval( cur, data );
		}
	} else {
		VDRING_FOREACH_REVERSE( cur, lst ) {

	   		eval( cur, data );
		}
	}
}




V_EXPORT void VDRING_insert_sorted( VDRING *list, VDRING_COMPARE compare, VDRING *newentry) 
{
	VDRING *cur;
	
	VDRING_FOREACH(  cur, list ) {
		if (compare(cur,newentry) > 0) {
			VDRING_insert_before(cur,newentry);
			return;
		}
	}

	VDRING_push_back( list, newentry );
}

