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
#include <util/vdlistunrolled.h>

V_STATIC_INLINE VDLISTUNROLLED_entry *VDLISTUNROLLED_new_list_entry(VCONTEXT *ctx, size_t datasize)
{
	VDLISTUNROLLED_entry *entry;

	entry = V_MALLOC( ctx, sizeof(VDLISTUNROLLED_entry) + datasize );
	if (!entry) {
		return 0;
	}
	entry->elmcount = 0;
	return entry;
}


V_STATIC_INLINE void *VDLISTUNROLLED_insert_entry(VDLISTUNROLLED_entry *entry, size_t pos, size_t elmmaxcount, size_t elmsize)
{
	if (!entry) {
		return 0;
	}

	if (entry->elmcount >= elmmaxcount || entry->elmcount == (size_t) -1) {
		return 0;
	}

	pos ++;

	if (pos < entry->elmcount) {	
		memmove(entry->buffer + (pos + 1) * elmsize,
			    entry->buffer +  pos * elmsize, 
				(entry->elmcount - pos) * elmsize);
		
	} 
	
	entry->elmcount ++ ;
	return entry->buffer + (pos * elmsize);
}


V_STATIC_INLINE void *VDLISTUNROLLED_insert_newnode(VDLISTUNROLLED *list,
												  VDLISTUNROLLED_position insert_pos)
{
	VDLISTUNROLLED_entry *newnode;

	newnode = VDLISTUNROLLED_new_list_entry( list->ctx, list->elmsize * list->elmmaxcount);
	if (!newnode) {
		return 0;
	}
	
	VDRING_insert_after( (VDRING *) insert_pos.entry, (VDRING *) newnode );

	newnode->elmcount = 1;
	return newnode->buffer;
}


int VDLISTUNROLLED_insert_after(VDLISTUNROLLED *list, VDLISTUNROLLED_position pos, void *data, size_t size)
{
	size_t elmsize = list->elmsize;
	size_t elmmaxcount = list->elmmaxcount;
	VDLISTUNROLLED_position insert_pos = pos;
	VDLISTUNROLLED_entry *next;
	void *newnodepos;

	if (size != list->elmsize) {
		return -1;
	}

	if (!insert_pos.entry) {
		insert_pos.entry = (VDLISTUNROLLED_entry *) list->root.ring.prev;
	}

	if ((newnodepos = VDLISTUNROLLED_insert_entry(
								insert_pos.entry, 
 								insert_pos.index, 
								elmmaxcount, 
								elmsize)) == 0) {
		
		next = (VDLISTUNROLLED_entry *) insert_pos.entry->ring.next;
		 
		if ((newnodepos = VDLISTUNROLLED_insert_entry(next, (size_t) -1, elmmaxcount, elmsize)) == 0){

			if ((newnodepos = VDLISTUNROLLED_insert_newnode(list, insert_pos)) == 0) {
				return -1;
			}
			list->entrycount ++;
			next = (VDLISTUNROLLED_entry *) insert_pos.entry->ring.next;

		} 
			
		if (insert_pos.entry->elmcount != (size_t) -1 && 
			insert_pos.index < (insert_pos.entry->elmcount - 1) ) {

			/* bad bad */
			insert_pos.index ++;

			/* move elements from prev node to new node */
			memcpy(next->buffer, 
				   insert_pos.entry->buffer + (list->elmmaxcount - 1) * elmsize,
				   elmsize);

			/* move elements in current node */		
			memmove(insert_pos.entry->buffer + (insert_pos.index + 1) * elmsize,
					insert_pos.entry->buffer + insert_pos.index  * elmsize,
					(insert_pos.entry->elmcount - insert_pos.index - 1) * elmsize);
		
			/* copy new node into insert_pos.index */
			newnodepos = insert_pos.entry->buffer + insert_pos.index  * elmsize;
		}
	} 
	
	memcpy(newnodepos, data, elmsize);
	list->elmcount ++;
	return 0;
}


int VDLISTUNROLLED_unlink(VDLISTUNROLLED *list, VDLISTUNROLLED_position pos)
{
	VDLISTUNROLLED_entry *entry;
	size_t elmsize = list->elmsize;

	if (VDLISTUNROLLED_check_position(pos)) {
		return -1;
	}
	entry = pos.entry;

	if (entry->elmcount == (size_t) -1) {
		return -1;
	}

	if (entry->elmcount != 1) {
		if (pos.index < (entry->elmcount-1)) {
			memmove(entry->buffer + pos.index * elmsize,
					entry->buffer + (pos.index + 1) * elmsize, 
					(entry->elmcount - pos.index - 1) * elmsize );
		}
		entry->elmcount --;
	} else {
		VDRING_unlink((VDRING *)pos.entry);
		V_FREE(list->ctx, pos.entry);
		list->entrycount --;
	}

	list->elmcount --;
	return 0;
}


