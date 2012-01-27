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

#ifndef _VBASE_VSRING_H_
#define _VBASE_VSRING_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>

/**
 * @brief Single linked circular list data structure; where each list element can be of different length. Each element has a pointer to the next element of the list.
 *
 * In a circularly-linked list, the first and final nodes are linked together. 
 * To traverse a circular linked list, you begin at any node and follow the list 
 * in either direction until you return to the original node. Viewed another way, 
 * circularly-linked lists can be seen as having no beginning or end. This type of 
 * list is most useful for managing buffers for data ingest, and in cases where you 
 * have one object in a list and wish to see all other objects in the list.
 * The pointer pointing to the whole list is usually called the end pointer.
 * (quoted from http://en.wikipedia.org/wiki/Linked_list )
 *
 * Usage: If the user wants to link his struct(ure) into a list, then he must embed a VSLIST_entry into his structure.
 * Access to user defined structure is via embedded VDRING.
 *
 * Note: unlike VSLIST it is not easy to get the last element in the list; in VSLIST we have a pointer to the last element
 * int the list header; with VSRING we have to traverse the whole list in order to get to the last element.
 */
typedef struct tagVSRING 
{
  struct tagVSRING *next;
}  
  VSRING;

typedef void	 (*VSRING_VISITOR_V)(VSRING *entry, void *context);
typedef V_INT32  (*VSRING_VISITOR)	(VSRING *entry, void *context);
typedef V_INT32  (*VSRING_COMPARE)	(VSRING *, VSRING *);

/**
 * @brief initialises an empty list head
 * @param head list head 
 */
V_INLINE void VSRING_init( VSRING *list ) 
{ 
  list->next = list; 
}

/**
 * @brief checks if argument list is empty
 * @param list pointer to list.
 * @returns not zero for non empty list.
 */
V_INLINE int VSRING_isempty( VSRING *list ) 
{
  return list->next == list;
}

/**
 * @brief insert new entry after a given entry.
 * @param newentry pointer to new list entry (to be inserted).
 * @param list current position (newentry inserted after this one).
 */
V_INLINE void VSRING_insert_after( VSRING *list, VSRING *newentry) 
{
   newentry->next = list->next;
   list->next = newentry;
}


V_INLINE VSRING * VSRING_unlink_after( VSRING *list ) 
{
   VSRING *ret;
   
   if (VSRING_isempty(list)) {
		return 0;
   }

   ret = list->next;
   list->next = list->next->next;
   return ret;
}


V_INLINE void VSRING_push_front( VSRING *list, VSRING *newentry)
{
  VSRING_insert_after( list, newentry );
}

/**
 * @brief remove the first element from list (used to maintain double ended queue)
 * @param elem (in) list head
 * @return the first element of list, NULL if list empty
 */
V_INLINE VSRING * VSRING_pop_front( VSRING *elem )
{
  VSRING *ret;
  if (VSRING_isempty( elem )) {
    return 0;
  }
  ret = elem->next;
  VSRING_unlink_after(elem);
  return ret;  
}


V_INLINE VSRING *VSRING_get_first(VSRING *list)
{

  if (VSRING_isempty(list)) {
    return 0;
  }  
  return list->next;
}


V_INLINE VSRING *VSRING_get_last(VSRING *list)
{
  VSRING *cur;

  if (VSRING_isempty(list)) {
    return 0;
  }  
  
  for(cur = list;cur->next != list; cur = cur->next);

  return cur;
}

V_INLINE VSRING *VSRING_get_next( VSRING *end, VSRING *cur ) 
{
  if (cur->next == end) { 
    return 0;
  }
  return cur->next;

}

/**
 * @brief Macro for iterate over all elements of a list, the list is traversed in forward direction from first element to the last element.
 * @param loopvarname (type VSRING *) pointer to the current element
 * @param list (type VSRING *) pointer to the list that is traversed
 */
#define VSRING_FOREACH( loopvarname, list )\
  for((loopvarname) = (list)->next;\
      (loopvarname) != (list);\
      (loopvarname) = (loopvarname )->next )

/*
 * @brief: return number of elements in list
 * the whole list structure is traversed.
 */
V_INLINE size_t VSRING_size( VSRING *list )
{
	size_t sz;
	VSRING * cur;
	
	VSRING_FOREACH( cur, list ) {
		sz += 1;
	}
	return sz;
}

/*
 * @brief get the nth element of a list as counted from the beginning of the list.
 * @brief list (IN) the object
 * @brief nth  (IN) index of element to retrieve (null based).
 */
V_INLINE VSRING *VSRING_get_nth( VSRING *list, size_t nth)
{

	VSRING *elm;

	for(elm = VSRING_get_first(list); elm != 0 && nth > 0; nth -= 1, elm = VSRING_get_next(list, elm)); 

	return elm;
}

/*
 * @brief get the nth element of a list as counted from the end of the list.
 * @brief list (IN) the object
 * @brief nth  (IN) index of element to retrieve (null based).
 * traverses the list twice: once to get size of list, then to get the element.
 */
V_INLINE VSRING *VSRING_get_nth_reverse( VSRING *list, size_t nth)
{	
	return VSRING_get_nth( list, VSRING_size( list ) - nth );
}


/**
 * @brief iterate over all elements of a list, callback is invoked for either element of the list.
 *
 * @param lst (in) the object.
 * @param eval (in) callback that is called to visit each set (or cleared) bit.
 * @param context (in) pointer that is passed as context on each invocation of callback.
 */
V_INLINE void VSRING_foreach( VSRING *lst, VSRING_VISITOR_V eval, void *context)
{
	VSRING *cur;

	if (!eval) {
	  return ;
	}

	VSRING_FOREACH(  cur, lst ) {
		eval( cur, context );

	}		
}

/**
 * @brief find an element within the linked list. callback is invoked for each element of the list, in forward direction from first element to last element; when the callback returns non zero value the iteration stops as we have found what we searched for.
 * 
 * @param lst (in) the object.
 * @param eval (in) callback that is called to visit each set (or cleared) bit.
 * @param context (in) pointer that is passed as context on each invocation of callback.
 * @param retval (out) optional - the first non zero value returned by eval callback, if NULL then ignored.
 * @return the list element that has matched (i.e. element that has been found).
 */
V_INLINE VSRING *VSRING_findif( VSRING *lst, VSRING_VISITOR eval, void *context, V_INT32 *retval)
{
	int ret;
	VSRING *cur;

	if (retval) {
		*retval = 0;
	}

	if (!eval) {
	  return 0;
	}

	VSRING_FOREACH(  cur, lst ) {
		ret = eval( cur, context );
	   	if (ret) {
			if (retval) {
				*retval = ret;
			}
		 	return cur;
	   	}
	}

	return 0;
}

/** 
 * @brief iterate over all entries of the list and delete entries that match predicate from the list, and frees the memory (optionally)
 * @param list (in) the object.
 * @param check_if (in) predicate function; the function returns 1 then inspected argument element will be deleted; if argument pointer is NULL then all entries will be deleted. 
 * @param context (in) data pointer passed to every invocation of check_if
 * @param free_ctx (in) interface used to free data of entry, is argument pointer is NULL then nothing is freed.
 * @param offset_of_link (in) offset of VSRING in embedded structure.
 */
V_INLINE size_t VSRING_deleteif( VSRING *list, VSRING_VISITOR check_if, void *context, VCONTEXT *free_ctx, int offset_of_link)
{
    VSRING *cur,*prev,*del;
	size_t  ret;

	prev = list;
	ret = 0;

	for(cur = prev->next; cur != list;) {
    
		if (!check_if || check_if( cur, context))  {

			cur = cur->next;
			del = VSRING_unlink_after( prev );
			if (free_ctx) {
				V_FREE( free_ctx, V_MEMBEROF(del, offset_of_link) );
			}
			ret++;

		} else {

			prev = cur;
			cur = cur->next;
		}
	}
	return ret;
}

/** 
 * @brief iterate over all entries of the list and delete them.
 * @param list (in) the object

 * @param on_delete(in) if not NULL then this callback is called prior to deleting each list element.
 * @param context (in) if on_delete is not NULL then this pointer is passed as argument to on_delete.

 * @param free_ctx (in) allocation interface used to free data of entry.
 * @param offset_of_link (in) offset of VSRING in embedded structure.
 * @return nunmber of elements deleted.
 */
V_INLINE void VSRING_deleteall( VSRING *list, VSRING_VISITOR_V on_delete, void *context, VCONTEXT *free_ctx, int offset_of_link)
{
    VSRING *cur,*prev,*del;

	prev = list;

	for(cur = prev->next; cur != list;) {
    
		if (on_delete) {
			on_delete( cur, context);
		}

		cur = cur->next;
		del = VSRING_unlink_after( prev );
		if (free_ctx) {
			V_FREE( free_ctx, V_MEMBEROF(del, offset_of_link) );
		}
	}
}


/**
 * @brief insert new element into sorted list; Maintains ordering relationship between elements of linked list (sort by ascending order)
 * A sorting algorithm based on this function is not very efficient; it is of complexity O(n^2); nevertheless usefull if a list is modified and has to be always maintained in sorted order.
 * @param list (in) the object
 * @param compare (in) comparison function.
 * @param newentry (in) element that is to be inserted into list.
 */
V_INLINE void VSRING_insert_sorted( VSRING *list, VSRING_COMPARE compare, VSRING *newentry) 
{
	VSRING *cur,*prev;
	
	prev = list;
	VSRING_FOREACH(  cur, list ) {
		if (compare(cur,newentry) > 0) {

			VSRING_insert_after(prev,newentry);
			return;
		}
		prev = cur;
	}

	VSRING_insert_after(prev,newentry);
}

/**
 * @brief Reverse a list
 * This function turns the first element into the last element, the second element
 * into the one before the last, and so on and on.
 * @param header (in) the object
 */
V_INLINE void VSRING_reverse( VSRING *lst )
{
	VSRING *cur = lst->next, *next, *tmp;

	if (cur) {
		next = cur->next;
		cur->next = lst;

		while(next != lst) {
			tmp = next->next;
			
			next->next = cur;
			cur = next;
			next = tmp;
		}
		lst->next = cur;
	}
}


/**
 * verify list for consistency, i.e. that it does not have loops.
 * Uses Floyds cycle finding algorithm.
 * http://en.literateprograms.org/Floyd%27s_cycle-finding_algorithm_%28C%29
 * @return 0 if list is inconsistent.
 */
V_INLINE size_t VSRING_check( VSRING *head )
{
	VSRING *slow, *fast = head->next;

	VSRING_FOREACH( slow, head ) {
		if (!slow) {
			return 0;
		}
		fast = fast->next;
		if (!fast) {
			return 0;
		}
		if (fast == head) {
			return 1;
		}
		if (fast == slow) {
			return 0;
		}

		fast = fast->next;
		if (!fast) {
			return 0;
		}
		if (fast == head) {
			return 1;
		}
		if (fast == slow) {
			return 0;
		}
	}
	return 1;
}

#ifdef  __cplusplus
}
#endif

#endif
