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

V_EXPORT int VCIRCBUF_resize( VCIRCBUF *buf, size_t new_numofelem)
{	
	V_UINT8 *ptr;
	int      compact_later = 0;

	if (!buf->ctx) {
		return -1;
	}	

	if (new_numofelem > buf->elmcount) {
		return -1;
	}

	/*do we have to compact?*/
	if (buf->start < buf->end) {

		if (buf->end > new_numofelem) {
			
			memmove( buf->buffer, 
					 buf->buffer + buf->start * buf->elmsize, 
					 buf->end * buf->elmsize);

			buf->end = buf->end - buf->start;
			buf->start = 0;
		}

	} else {

		if (new_numofelem < buf->elmmaxcount) {

			size_t at_end = buf->elmmaxcount - buf->end;

			memmove(buf->buffer + (new_numofelem - at_end) * buf->elmcount,
					buf->buffer + buf->end * buf->elmcount,
					at_end * buf->elmcount);
			
			buf->end = new_numofelem - at_end;
		} else {
			compact_later = 1;
		}
	}

	ptr = V_REALLOC(buf->ctx, buf->buffer, new_numofelem * buf->elmsize);
	if (!ptr) {
		return -1;
	}
	buf->buffer = ptr;
	buf->elmmaxcount = new_numofelem;

	if (compact_later) {
		size_t at_end = buf->elmmaxcount - buf->end;

		memmove(buf->buffer + (new_numofelem - at_end) * buf->elmcount,
				buf->buffer + buf->end * buf->elmcount,
				at_end * buf->elmcount);
		
		buf->end = new_numofelem - at_end;
	}

	return 0;
}





