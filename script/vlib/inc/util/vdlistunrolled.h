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

#ifndef _VDLISTUNROLLED_H_
#define _VDLISTUNROLLED_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>
#include <util/vdring.h>
#include <stdlib.h>
#include <string.h>


typedef struct tagVDLISTUNROLLED_entry {
  VDRING  ring;	
  size_t elmcount;
  V_UINT8 buffer[];
} VDLISTUNROLLED_entry;



/**
 * @brief unrolled list where each list node contains an array of nodes.
 * Unlike other lists here, each element in a list is of the same size.

 * an unrolled linked list is a variation on the linked list which stores multiple 
 * elements in each node. It can drastically increase cache performance, 
 * while decreasing the memory overhead associated with storing links to other list elements.. 
 *
 * Each node holds up to a certain maximum number of elements, typically just large 
 * enough so that the node fills a single cache line or a small multiple thereof. 
 * A position in the list is indicated by both a reference to the node and a position 
 * in the elements array. It's also possible to include a previous pointer for an unrolled 
 * doubly-linked linked list.
 * Quoted from http://en.wikipedia.org/wiki/Unrolled_linked_list
 */
typedef struct {
	
	VCONTEXT *ctx;

	size_t elmsize;			/* size of element */
	size_t elmmaxcount;		/* number of elements in one entry */

	size_t elmcount;		/* size of elements in list (global) */
	size_t entrycount;		/* number of entries */

	VDLISTUNROLLED_entry root;

}
	VDLISTUNROLLED;

typedef void (*VDLISTUNROLLED_VISITOR_V)(VDLISTUNROLLED *list, void *entry, void *context);
typedef int  (*VDLISTUNROLLED_VISITOR)  (VDLISTUNROLLED *list, void *entry, void *context);

/**
 * Current position of iteration through an unrolled linked list structure.
 */
typedef struct {
	VDLISTUNROLLED_entry *entry;
	size_t				  index;
} VDLISTUNROLLED_position;


/**
 * @brief initialises a new unrolled list. List entries are all of a fixed size.
 * @param ctx (in) allocator interface. (if null we are using the default allocator)
 * @param list (in) object that is initialised
 * @param elmsize (in) size of one element in list.
 * @param elmmaxcount (in) number of element stored in one list entry
 */
V_INLINE int	VDLISTUNROLLED_init( VCONTEXT *ctx, VDLISTUNROLLED *list, size_t elmsize, size_t elmmaxcount)
{	
	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	list->ctx = ctx;
	list->elmmaxcount = elmmaxcount;
	list->elmsize = elmsize;

	list->elmcount = list->entrycount = 0;

	VDRING_init( &list->root.ring );
	list->root.elmcount = (size_t) -1;

	return 0;
}

/**
 * @brief check validity of unrolled list instance
 * @param list the object
 * @return 0 if list is invalid.
 */
V_INLINE int VDLISTUNROLLED_check(VDLISTUNROLLED *list)
{
	size_t elmcount, size = 0;
	size_t cursize = 0;
	VDRING *cur,*next;

	if (list->root.elmcount != (size_t) -1) {
		return 0;
	}

	VDRING_FOREACH( cur, &list->root.ring) {

		next = cur->next;
		if (!next || next->prev != cur) {		
			return 0;
		}
		elmcount = ((VDLISTUNROLLED_entry *) cur)->elmcount;
		if ( elmcount > list->elmmaxcount) {
			return 0;
		}
		if (elmcount == 0) {
			return 0;
		}
		size++;
		cursize += elmcount;
	}

	if (size != list->entrycount) {
		return 0;
	}
	if (cursize != list->elmcount) {
		return 0;
	}

	return 1;
}


/**
 * @brief checks if argument list is empty
 * @param list pointer to list.
 * @returns not zero for non empty list.
 */
V_INLINE int VDLISTUNROLLED_isempty( VDLISTUNROLLED *list )
{
	return list->elmcount == 0;
}


V_INLINE size_t VDLISTUNROLLED_size( VDLISTUNROLLED *list) 
{
	return list->elmcount;
}


V_INLINE size_t VDLISTUNROLLED_maxsize( VDLISTUNROLLED  *list )
{
	return list->elmmaxcount;
}

/**
 * @brief Returns position structure of first element in unrolled linked list.
 * @param list (in) the object
 * @return position structure of first element
 */
V_INLINE VDLISTUNROLLED_position VDLISTUNROLLED_get_first( VDLISTUNROLLED *list )
{
	VDLISTUNROLLED_position ret;

	ret.entry = (VDLISTUNROLLED_entry *) list->root.ring.next;
	ret.index = 0;

	return ret;
}


/**
 * @brief Returns position structure of last element in unrolled linked list.
 * @param list (in) the object
 * @return position structure of first element
 */
V_INLINE VDLISTUNROLLED_position VDLISTUNROLLED_get_last( VDLISTUNROLLED *list )
{
	VDLISTUNROLLED_position ret;

	if (list->elmcount) {
		ret.entry = (VDLISTUNROLLED_entry *) list->root.ring.prev;
		ret.index = ret.entry->elmcount - 1;		
	} else  {
		ret = VDLISTUNROLLED_get_first( list);
	}
	
	return ret;
}

/**
 * @brief Returns position to the next element in unrolled linked list.
 * @param list (in) the object
 * @param pos (in) iteration position in list.
 * @return position structure of the element that follows the element identified by pos structure.
 */
V_INLINE VDLISTUNROLLED_position VDLISTUNROLLED_next(VDLISTUNROLLED_position pos)
{
	VDLISTUNROLLED_position ret;
	VDLISTUNROLLED_entry *entry;

	ret = pos;
	entry = pos.entry;

	ret.index++;
	if (ret.index >= entry->elmcount) {
		ret.entry = (VDLISTUNROLLED_entry *) entry->ring.next;
		ret.index = 0;
	}
	return ret;
}

/**
 * @brief Returns position to the previous element in unrolled linked list.
 * @param list (in) the object
 * @param pos (in) iteration position in list.
 * @return position structure of the element that precedes the element identified by pos structure.
 */
V_INLINE VDLISTUNROLLED_position VDLISTUNROLLED_prev(VDLISTUNROLLED_position pos)
{
	VDLISTUNROLLED_position ret;
	VDLISTUNROLLED_entry *entry;

	ret = pos;
	entry = pos.entry;
	if (ret.index) {
		ret.index--;
	} else {
		ret.entry = (VDLISTUNROLLED_entry *) entry->ring.prev;
		ret.index = 0;
		if (ret.entry && ret.entry->elmcount != (size_t) -1) {
			ret.index = ret.entry->elmcount - 1;
		} 
	}
	return ret;
}


/**
 * @brief verify a position structure.
 */
V_INLINE int VDLISTUNROLLED_check_position( VDLISTUNROLLED_position pos)
{
	if (!pos.entry) {
		return -1;
	}
	if (pos.index >= pos.entry->elmcount) {
		return -1;
	}
	return 0;
}

/** 
 * @brief copy list entry identified by position structure (pos) into  user supplied memory area
 * @param list (in) the object
 * @param pos  (in) position pointer
 * @param data (in|out) user supplied buffer
 * @param size (in) size of user supplied buffer (must be equal to the size of one element).
 * @return 0 on success -1 on failure
 */
V_INLINE int VDLISTUNROLLED_copy_at(VDLISTUNROLLED *list, VDLISTUNROLLED_position pos, void *data, size_t size)
{
	void *ptr;
	if (size != list->elmsize) {
		return -1;
	}
	if (VDLISTUNROLLED_check_position(pos)) {
		return -1;
	}

	ptr = pos.entry->buffer + pos.index * size;
	memcpy(data, ptr, size);

	return 0;
}

/** 
 * @brief return pointer to list entry identified by position structure (pos).
 * @param list (in) the object
 * @param pos  (in) position pointer
 * @return pointer to data entry in linked list.
 */

V_INLINE V_UINT8 * VDLISTUNROLLED_at(VDLISTUNROLLED *list, VDLISTUNROLLED_position pos)
{
	if (VDLISTUNROLLED_check_position(pos)) {
		return 0;
	}

	return pos.entry->buffer + pos.index * list->elmsize;
}

/**
 * @brief insert new entry after a given entry into this unrolled linked list 
 * @param list (in) pointer to list head
 * @param pos  (in) current position (newentry inserted after this one).
 * @param data (in) pointer to element that is to be inserted into this list.
 * @param size (in) size of area identified by data pointer.
 */
V_EXPORT int VDLISTUNROLLED_insert_after(VDLISTUNROLLED *list, VDLISTUNROLLED_position pos, void *data, size_t size);

/**
 * @brief insert new entry before a given entry into this unrolled linked list 
 * @param list (in) pointer to list head
 * @param pos  (in) current position (newentry inserted before this one).
 * @param data (in) pointer to element that is to be inserted into this list.
 * @param size (in) size of area identified by data pointer.
 */
V_INLINE int VDLISTUNROLLED_insert_before(VDLISTUNROLLED *list, VDLISTUNROLLED_position pos, void *data, size_t size)
{
	VDLISTUNROLLED_position insert_pos = VDLISTUNROLLED_prev(pos);
	
	return VDLISTUNROLLED_insert_after( list, insert_pos, data, size);
}


/**
 * @brief delete an element from a unrolled list.
 * @param list (in) the object
 * @param pos (in)  deletes element identified by this position structure
 * return -1 on failure 0 on success.
 */
V_EXPORT int VDLISTUNROLLED_unlink(VDLISTUNROLLED *list, VDLISTUNROLLED_position pos);


/**
 * @brief insert element as last in list (used to maintain queue)
 * @param list (in) the object
 * @param data (in) pointer to element that is to be inserted into this list.
 * @param size (in) size of area identified by data pointer.
 */
V_INLINE int VDLISTUNROLLED_push_back(VDLISTUNROLLED *list, void *data, size_t size)
{
	VDLISTUNROLLED_position last = VDLISTUNROLLED_get_last( list );
	return VDLISTUNROLLED_insert_after(list, last, data, size);
		
}

/**
 * @brief insert element as last in list (used to maintain queue)
 * @param list (in) the object
 * @param data (in) pointer to element that is to be inserted into this list.
 * @param size (in) size of area identified by data pointer.
 */
V_INLINE int VDLISTUNROLLED_push_front(VDLISTUNROLLED *list, void *data, size_t size)
{
	VDLISTUNROLLED_position last = VDLISTUNROLLED_get_first( list );
	return VDLISTUNROLLED_insert_before(list, last, data, size);
}


/**
 * @brief copy data of last element into user supplied buffer and remove the last element from list (used to maintain double ended queue)
 * @param list (in) the object
 * @param data (in) pointer to element that is to receive data of list entry
 * @param size (in) size of area identified by data pointer.
 * @return return 0 on success.
 */
V_INLINE int VDLISTUNROLLED_pop_back(VDLISTUNROLLED *list, void *data, size_t size)
{
	VDLISTUNROLLED_position pos;

	
	if (VDLISTUNROLLED_isempty(list)) {
		return -1;
	}
	
	pos = VDLISTUNROLLED_get_last( list );
	
	if (VDLISTUNROLLED_copy_at(list, pos, data, size)) {
		return -1;
	}

	if (VDLISTUNROLLED_unlink(list,pos)) {
		return -1;
	}
	return 0;
}

/**
 * @brief copy data of first element into user supplied buffer and remove the first element from list (used to maintain double ended queue)
 * @param list (in) the object
 * @param data (in) pointer to element that is to receive data of list entry
 * @param size (in) size of area identified by data pointer.
 * @return return 0 on success.
 */
V_INLINE int VDLISTUNROLLED_pop_front(VDLISTUNROLLED *list, void *data, size_t size)
{
	VDLISTUNROLLED_position pos;

	
	if (VDLISTUNROLLED_isempty(list)) {
		return -1;
	}
	
	pos = VDLISTUNROLLED_get_first( list );

	if (VDLISTUNROLLED_copy_at(list, pos, data, size)) {
		return -1;
	}
	if (VDLISTUNROLLED_unlink(list,pos)) {
		return -1;
	}
	return 0;
}

/**
 * @brief Macro for iterate over all elements of a list, the list is traversed in forward direction from first element to the last element.
 * @param loopvarname (type VDLISTUNROLLED_position) struct that identifies current element
 * @param list (type VDLISTUNROLLED *) pointer to the list that is traversed
 */
#define VDLISTUNROLLED_FOREACH( loopvarname, list )\
  for((loopvarname) = VDLISTUNROLLED_get_first( list );\
      (loopvarname).entry != (VDLISTUNROLLED_entry *) &(list)->root;\
      (loopvarname) = VDLISTUNROLLED_next( loopvarname ) )

/**
 * @brief Macro for iterate over all elements of a list, the list is traversed in reverse direction from last element to the first element.
 * @param loopvarname (type VDLISTUNROLLED_position) pointer to the current element
 * @param list (type VDLISTUNROLLED *) pointer to the list that is traversed
 */
#define VDLISTUNROLLED_FOREACH_REVERSE( loopvarname, list )\
  for((loopvarname) = VDLISTUNROLLED_get_last( list );\
      (loopvarname).entry != (VDLISTUNROLLED_entry *) &(list)->root;\
      (loopvarname) = VDLISTUNROLLED_prev( loopvarname ) )

/**
 * @brief Macro for iterate over all elements of a list, You may delete the current element; the list is traversed in forward direction from first element to the last element.
 * @param loopvarname (type VDLISTUNROLLED_position) pointer to the current element
 * @param loopvarnamenext (type VDLISTUNROLLED_position) do not modify! pointer to next element after current element (used for iteration).
 * @param list (type VDLISTUNROLLED *) pointer to the list that is traversed
 */
#define VDLISTUNROLLED_FOREACH_SAVE( loopvarname, loopvarnext, list )\
  for((loopvarname) = VDLISTUNROLLED_get_first( list ), (loopvarnext) = VDLISTUNROLLED_next( loopvarname );\
      (loopvarname).entry != (VDLISTUNROLLED_entry *) &(list)->root;\
      (loopvarname) = (loopvarnext), (loopvarnext) = VDLISTUNROLLED_next(loopvarname) )

/**
 * @brief Macro for iterate over all elements of a list, You may delete the current element; the list is traversed in reverse direction from last element to the first element.
 * @param loopvarname (type VDLISTUNROLLED_position) pointer to the current element
 * @param loopvarnamenext (type VDLISTUNROLLED_position) do not modify! pointer to next element after current element (used for iteration).
 * @param list (type VDLISTUNROLLED *) pointer to the list that is traversed
 */
#define VDLISTUNROLLED_FOREACH_REVERSE_SAVE( loopvarname, loopvarnext, list )\
  for((loopvarname) = VDLISTUNROLLED_get_last(list);\
      (loopvarname).entry != (VDLISTUNROLLED_entry *) &(list)->root;\
      (loopvarname) = (loopvarnext), (loopvarnext) = VDLISTUNROLLED_prev(loopvarname) )


/**
 * @brief list free all entries of the list. The list will then be an empty list.
 */
V_INLINE void VDLISTUNROLLED_free( VDLISTUNROLLED *list, VDLISTUNROLLED_VISITOR_V free_func, void *context)
{
	VDRING *next,*cur;
	VDLISTUNROLLED_entry *curr;
	size_t pos,elmsize,i;
	
	elmsize = list->elmsize;

	VDRING_FOREACH_SAVE( cur, next, &list->root.ring) {
	
		curr = (VDLISTUNROLLED_entry *) cur;
		if (free_func) {
			for(pos = 0,i=0;i<curr->elmcount;i++) {
				free_func(list,  (void *) (curr->buffer +pos), context);
				pos += elmsize;
			}
		}
		/*list->ctx->free( VDRING_unlink(cur) );*/
		V_FREE( list->ctx, cur );
		
	}	
	VDRING_init( &list->root.ring );
	list->entrycount = 0;
	list->elmcount = 0;
}	

#if 0
V_INLINE void VDLISTUNROLLED_deleteif( VDLISTUNROLLED *list, VDLISTUNROLLED_VISITOR free_func, void *context)
{
	VDRING *next,*cur;
	VDLISTUNROLLED_entry *curr;
	size_t pos,elmsize,i;
	
	elmsize = list->elmsize;

	VDRING_FOREACH_SAVE( cur, next, &list->root.ring) {
	
		curr = (VDLISTUNROLLED_entry *) cur;
		if (free_func) {
			for(pos = 0,i=0;i<curr->elmcount;i++) {
				free_func(list,  (void *) curr->buffer[ pos ], context);
				pos += elmsize;
			}
		}
		/*list->ctx->free( VDRING_unlink(cur) );*/
		list->ctx->free( cur );
		
	}	
	VDRING_init( &list->root.ring );
	list->entrycount = 0;
	list->elmcount = 0;
}	
#endif

#ifdef  __cplusplus
}
#endif

#endif
