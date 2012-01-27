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

#ifndef _VBASE_VSLIST_H_
#define _VBASE_VSLIST_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>


/**
 * an entry in double linked list; 
 * If the user wants to link his struct(ure) into a VSLIST linked list, then he must embed a VDLIST_entry into his structure.
 * Access to user defined structure is via embedded VSLIST_entry.
 */
typedef struct tagVSLIST_entry 
{
  struct tagVSLIST_entry *next;
}  
  VSLIST_entry;


/**
 * @brief Single linked list data structure; where each list element can be of different length.
 *
 * This single linked list has a list header (VSLIST).
 * The list header points to the first and last element in the list.
 * last elements next pointer is NULL.
 *
 * Usage: If the user wants to link his struct(ure) into a list, then he must embed a VSLIST_entry into his structure.
 * Access to user defined structure is via embedded VSLIST_entry.
 *
 * The list header contains a counter of elements (feature can be surpressed by defining #define VSLIST_NO_ELMCOUNT
 */
typedef struct {
#ifndef VSLIST_NO_ELMCOUNT
	size_t elmcount;   /** number of elements in list */ 
#endif
	struct tagVSLIST_entry *prev, *next; /** root.prev - pointer to first element,  root.next - pointer to last element */
} VSLIST;



typedef void	(*VSLIST_VISITOR_V)(VSLIST *list, VSLIST_entry *entry, void *context);
typedef V_INT32 (*VSLIST_VISITOR)  (VSLIST *list, VSLIST_entry *entry, void *context);
typedef V_INT32 (*VSLIST_COMPARE)  (VSLIST_entry *,  VSLIST_entry * );


/**
 * @brief initialises an empty list head
 * @param head list head 
 */
V_INLINE void VSLIST_init( VSLIST *head ) 
{ 
  head->next = head->prev = 0; 
#ifndef VSLIST_NO_ELMCOUNT
  head->elmcount = 0;
#endif
}

/**
 * @brief checks if argument list is empty
 * @param list pointer to list.
 * @returns not zero for non empty list.
 */
V_INLINE int VSLIST_isempty( VSLIST *head ) 
{
  return head->next == 0;
}


/**
 * @brief insert new entry after a given entry.
 * @param list pointer to list head
 * @param pos current posision (newentry inserted after this one).
 * @param newentry pointer to new list entry (to be inserted).
 */
V_INLINE void VSLIST_insert_after( VSLIST *list, VSLIST_entry *pos, VSLIST_entry *newentry) 
{
	if (list->next) {

		if (pos){
		
			newentry->next = pos->next;
			pos->next = newentry;

			if (!newentry->next) {
				list->next = newentry;
			} 

		} else  {
			newentry->next = list->prev;
			list->prev = newentry;
		} 


	} else {

		list->next = list->prev = newentry;
		newentry->next =  0;		

	}


#ifndef VSLIST_NO_ELMCOUNT
	list->elmcount ++;
#endif
}


/**
 * @brief delete an element from a list.
 * @param list deletes this list element
 */
V_INLINE VSLIST_entry *VSLIST_unlink_after( VSLIST *list, VSLIST_entry *link ) 
{
	VSLIST_entry *ret;

	if (!link) {
		ret = list->prev;
		list->prev = ret->next;
		if (!list->prev) {
			list->next = 0;
		}
		
	} else {
		ret = link->next;	
		if (!ret) {
			return 0;
		}

		link->next = ret ? ret->next : 0;

		if (link->next == 0) {
			list->next = link;
		}

	}
#ifndef VSLIST_NO_ELMCOUNT
		list->elmcount --;
#endif

	return ret;
}


/**
 * @brief insert element as last in list.
 * @param list list head.
 * @param newentry entry to be inserted into list
 */
V_INLINE void VSLIST_push_back( VSLIST *list, VSLIST_entry *newentry)
{
  VSLIST_insert_after( list, list->next, newentry );
}

/**
 * @brief insert element as first in list (used to maintain queue)
 * @param list list head
 * @param newentry entry to be inserted into list
 */
V_INLINE void VSLIST_push_front( VSLIST *list, VSLIST_entry *newentry)
{
  VSLIST_insert_after( list, 0, newentry );
}

/**
 * @brief append contents of one list onto the other
 * @param dest 
 * @param src  content of this list is appended onto dest list. postcondition: src list is empty
 */

V_INLINE void VSLIST_append( VSLIST *dest, VSLIST *src) 
{
	if (VSLIST_isempty(dest)) {
		
		if (!VSLIST_isempty(src)) {

			dest->prev = src->prev;
			dest->next = src->next;
#ifndef VSLIST_NO_ELMCOUNT
			dest->elmcount = src->elmcount;
#endif	

			//VSLIST_init(src);
		} 

	} else {

		if (!VSLIST_isempty(src)) {
			dest->next->next = src->prev;
			dest->next = src->next;
#ifndef VSLIST_NO_ELMCOUNT
			dest->elmcount += src->elmcount;
#endif	
			//VSLIST_init(src);

		}
	}


}


/**
 * @brief remove the first element from list (used to maintain queue)
 * @param list (in) the object
 * @return the first element of list, NULL if list empty
 */
V_INLINE VSLIST_entry * VSLIST_pop_front( VSLIST *list )
{
  return VSLIST_unlink_after(list, 0);
}


V_INLINE VSLIST_entry *VSLIST_get_first(VSLIST *list)
{
  return list->prev;
}

V_INLINE VSLIST_entry *VSLIST_get_last(VSLIST *list)
{
  return list->next;
}


V_INLINE VSLIST_entry *VSLIST_get_next( VSLIST_entry *cur) 
{
  return cur->next;
}




/**
 * @brief Macro for iterate over all elements of a list, the list is traversed in forward direction from first element to the last element.
 * @param loopvarname (type VSLIST_entry *) pointer to the current element
 * @param list (type VSLIST *) pointer to the list that is traversed
 */
#define VSLIST_FOREACH( loopvarname, list )\
  for((loopvarname) = (list)->prev;\
      (loopvarname) != 0;\
      (loopvarname) = (loopvarname)->next )

/**
 * @brief Macro for iterate over all elements of a list, the list is traversed in forward direction from first element to the last element.
 * @param loopvsarname (type VSLIST_entry *) pointer to the current element
 * @param list (type VSLIST *) pointer to the list that is traversed
 */
#define VSLIST_FOREACH_SAVE( loopvarname, loopvarnamenext, list )\
  for((loopvarname) = (list)->prev, (loopvarnamenext) = (loopvarname) ? (loopvarname)->next : 0;\
      (loopvarname) != 0;\
      (loopvarname) = (loopvarnamenext),  (loopvarnamenext) = (loopvarname) ? (loopvarname)->next : 0 )


/*
 * @brief: return number of elements in list
 * if we don't have element count in list (VSLIST_NO_ELMCOUNT defined), then the whole list structure is traversed.
 */
V_INLINE size_t VSLIST_size( VSLIST *list )
{
#ifndef VSLIST_NO_ELMCOUNT
	return list->elmcount;
#else
	size_t sz;
	VSLIST_entry * cur;
	
	VSLIST_FOREACH( cur, list ) {
		sz += 1;
	}
	return sz;
#endif
}

/*
 * @brief get the nth element of a list as counted from the beginning of the list.
 * @brief list (IN) the object
 * @brief nth  (IN) index of element to retrieve (null based).
 */
V_INLINE VSLIST_entry *VSLIST_get_nth( VSLIST *list, size_t nth)
{

	VSLIST_entry *elm;

#ifndef VSLIST_NO_ELMCOUNT
	if (nth >= list->elmcount) {
		return 0;
	}

	for(elm = VSLIST_get_first(list); nth > 0; nth -= 1, elm = VSLIST_get_next(elm)); 
#else
	for(elm = VSLIST_get_first(list); elm != 0 && nth > 0; nth -= 1, elm = VSLIST_get_next(elm)); 
#endif

	return elm;
}

/*
 * @brief get the nth element of a list as counted from the end of the list.
 * @brief list (IN) the object
 * @brief nth  (IN) index of element to retrieve (null based).
 */
V_INLINE VSLIST_entry *VSLIST_get_nth_reverse( VSLIST *list, size_t nth)
{	
	return VSLIST_get_nth( list, VSLIST_size( list ) - nth );
}


/**
 * @brief iterate over all elements of a list, callback is invoked for either element of the list.
 *
 * @param lst (in) the object.
 * @param eval (in) callback that is called to visit each set (or cleared) bit.
 * @param context (in) pointer that is passed as context on each invocation of callback.
 */
V_INLINE void VSLIST_foreach( VSLIST *lst, VSLIST_VISITOR_V eval, void *context)
{
    VSLIST_entry *cur;

	if (!eval) {
	  return;
	}

	VSLIST_FOREACH( cur, lst) {
		eval( lst, cur, context);
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
V_INLINE VSLIST_entry *VSLIST_findif( VSLIST *lst, VSLIST_VISITOR eval, void *context, V_INT32 *retval)
{
	int ret;
    VSLIST_entry  *cur;

	if (retval) {
		*retval = 0;
	}

	if (!eval) {
	  return 0;
	}

	VSLIST_FOREACH(  cur, lst ) {
		ret = eval( lst, cur, context);
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
 * @brief insert new element into sorted list; Maintains ordering relationship between elements of linked list (sort by ascending order)
 * A sorting algorithm based on this function is not very efficient; it is of complexity O(n^2); nevertheless usefull if a list is modified and has to be always maintained in sorted order.
 * @param list (in) the object
 * @param compare (in) comparison function.
 * @param newentry (in) element that is to be inserted into list.
 */
V_INLINE void VSLIST_insert_sorted( VSLIST *list, VSLIST_COMPARE compare, VSLIST_entry *newentry) 
{
	VSLIST_entry *cur,*prev;
	
	prev = 0;
	VSLIST_FOREACH(  cur, list ) {
		if (compare(cur,newentry) > 0) {

			VSLIST_insert_after(list, prev,newentry);
			return;
		}
		prev = cur;
	}

	VSLIST_push_back( list, newentry );
}

/** 
 * @brief iterate over all entries of the list and delete entries that match predicate from the list, and frees the memory (optionally)
 * @param list (in) the object.
 * @param check_if (in) predicate function; the function returns 1 then inspected argument element will be deleted; if argument pointer is NULL then all entries will be deleted. 
 * @param context (in) data pointer passed to every invocation of check_if
 * @param free_ctx (in) interface used to free data of entry, is argument pointer is NULL then nothing is freed.
 * @param offset_of_link (in) offset of VDLIST_entry in embedded structure.
 */
V_INLINE void VSLIST_deleteif( VSLIST *list, VSLIST_VISITOR check_if, void *context, VCONTEXT *free_ctx, int offset_of_link)
{
    VSLIST_entry *cur,*prev,*del;

	prev = 0;

	for(cur = list->prev; cur;) {
    
		if (!check_if || check_if( list, cur, context))  {

			cur = cur->next;
			del = VSLIST_unlink_after( list, prev );
			if (free_ctx) {
				V_FREE( free_ctx, V_MEMBEROF(del, offset_of_link) );
			}
			

		} else {

			prev = cur;
			cur = cur->next;
		}
	}
}

/** 
 * @brief iterate over all entries of the list and delete them.
 * @param list (in) the object

 * @param on_delete(in) if not NULL then this callback is called prior to deleting each list element.
 * @param context (in) if on_delete is not NULL then this pointer is passed as argument to on_delete.

 * @param free_ctx (in) allocation interface used to free data of entry.
 * @param offset_of_link (in) offset of VSRING in embedded structure.
 */
V_INLINE void VSLIST_deleteall( VSLIST *list, VSLIST_VISITOR_V on_delete, void *context, VCONTEXT *free_ctx, int offset_of_link)
{
    VSLIST_entry *cur,*del;


	for(cur = list->prev; cur;) {
    
		if (on_delete)  {
			on_delete( list, cur, context );
		}

		cur = cur->next;
		del = VSLIST_unlink_after( list, 0 );
		if (free_ctx) {
			V_FREE( free_ctx, V_MEMBEROF(del, offset_of_link) );
		}
	}
}


/**
 * @brief Reverse a list
 * This function turns the first element into the last element, the second element
 * into the one before the last, and so on and on.
 * @param header (in) the object
 */
V_INLINE void VSLIST_reverse( VSLIST *lst )
{
	VSLIST_entry *cur = lst->prev, *next;
	VSLIST_entry *tmp;
	if (cur) {
		next = cur->next;
		cur->next = 0;

		while(next) {
			tmp = next->next;
			
			next->next = cur;
			cur = next;
			next = tmp;
		}

		tmp = lst->prev;
		lst->prev = lst->next;
		lst->next = tmp;
	}
}

/**
 * @brief check consistency of list
 * This function checks for loops in the list, and if count of elements in list is the same as counter of elements in list header.
 * @param header (in) the object
 * @return zero if list contains errors.
 */
V_INLINE int VSLIST_check(VSLIST *header)
{
	VSLIST_entry *cur,*prev = 0,*fast= 0;
#ifndef VSLIST_NO_ELMCOUNT
	size_t count = 0;
#endif

	if (!header->prev || !header->next) {
		if (header->prev || header->next) {
			return 0;
		}
		return 1;
	}
	
	fast = header->prev;
	VSLIST_FOREACH( cur, header ) {

#ifndef VSLIST_NO_ELMCOUNT
		count += 1;
#endif	    
		prev = fast;
		fast = fast->next;
		if (!fast) {
			break;
		}
		if (fast == cur) {
			return 0;
		}

#ifndef VSLIST_NO_ELMCOUNT
		count += 1;
#endif
	    prev = fast;
		fast = fast->next;
		if (!fast) {
			break;
		}
		if (fast == cur) {
			return 0;
		}
	}

	if (header->next != prev) {
		return 0;
	}


#ifndef VSLIST_NO_ELMCOUNT
	if (header->elmcount != count) {
		return 0;
	}
#endif	
	return 1;
}


#ifdef  __cplusplus
}
#endif

#endif
