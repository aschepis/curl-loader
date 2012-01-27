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

#ifndef	_VBUCKETHASH_H_
#define _VBUCKETHASH_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>
#include <util/vsring.h>
#include <util/vhashfunction.h>

/*
Problem:

resize assumes that we know the hash value; or that we can computer it - bingo. 
currently there is no callback to extract the key from the entry - would need it only for case
when this is defined.

*/

//#define VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY


/**
 * @brief Entry in bucket hash table
 * Each hash table entry has to embed a VBUCKETHASH_Entry as part of its structure.
 */
typedef struct  {
	VSRING				entry;
#ifndef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
	VBUCKETHASH_VALUE	hash;
#endif
}
	VBUCKETHASH_Entry;


typedef int					(*VBUCKETHASH_COMPARE)	(VBUCKETHASH_Entry *, void * key, size_t key_length);
typedef VBUCKETHASH_VALUE	(*VBUCKETHASH_FUNCTION) (void * key, size_t key_length);

typedef void	(*VBUCKETHASH_VISITOR_V)	(VBUCKETHASH_Entry *, void *context);
typedef int		(*VBUCKETHASH_VISITOR)		(VBUCKETHASH_Entry *, void *context);

#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
typedef void *	(*VBUCKETHASH_EXTRACT_KEY)	(VBUCKETHASH_Entry *, size_t *sz);
#endif

/**
 * @brief Hash table that is implemented as bucket hash table.
 
 * Bucket hash table consists of array of linked lists, where each separate list holds elements with same hash number.
 * Entries and keys in hash table can be of variable size; each hash table entry has to embed a VBUCKETHASH_Entry as part of its structure.
 *
 * Compiler options:
 *	VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY - if NOT defined (default) then the entry structure
 *		contains the hash value of the key; so nomparsion of hash values prior to comparing the keys saves recomputation of the hash value
 *		Also resizing the hash table is faster, since we dont't have to recompute hash value.
 *		If DEFINED, then initialisation routines have to receive an additional parameter
 *		pointer to function that retrieves the key value out of an entry (needed for resize).
 *
 *					
 */
typedef struct {

	VCONTEXT		*ctx;
	VRESIZE_policy	sizepolicy;

	VSRING		    *buckets;
	size_t			buckets_size;
	size_t			elmcount;
	size_t			resize_threshold;
	int     		ismultimap;

	VBUCKETHASH_FUNCTION	hash_func;
	VBUCKETHASH_COMPARE		compare_func;  

#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
	VBUCKETHASH_EXTRACT_KEY extract_key;
#endif
}
  VBUCKETHASH;


/**
 * @brief set load factor of hash table that will trigger resize of hash table
 * The laod factor is specified globally for all instances of class VBUCKETHASH
 
 * Event that will trigger resize of hash table:
 *		[number of elements in hash table] * load_factor > [Size of hash table]
 * The load factor is specified as a rational number.
 *
 * @param multiplier (in) multiplier of rational number
 * @param divisor	 (in) divisor of rational number
 */
V_EXPORT int  VBUCKETHASH_set_resize_trigger( int multiplier, int divisor );

/**
 * @brief get current load factor of hash table that will trigger resize of hash table
 *
 * @param multiplier (out) multiplier of rational number
 * @param divisor	 (out) divisor of rational number
 */
V_EXPORT void VBUCKETHASH_get_resize_trigger( int *multiplier, int *divisor );


/** 
 *  @brief Object constructor; initialise the hash that allows multiple entries for a given key.
 
 *  @param ctx				(in) allocator interface. (if null we are using the default allocator)
 *  @param hash				(out) the object.
 *  @param buckets			(in) number of buckets - will be rounded to closes power of two (i.e. number of arrays of linked lists structures, each separate list holds elements with same hash number).
 *  @param hash_func		(in) pointer to hash function
 *  @param compare pointer	(in) to compare function; compares if two keys are equal, (called when hash value of two entries is equal)
 
 *  @returns 0 if ok
 */
V_EXPORT int VBUCKETHASH_init_multimap(
					
					VCONTEXT	*ctx,
					VBUCKETHASH	*hash, 

					size_t		buckets, 

					VBUCKETHASH_FUNCTION hash_func,
					VBUCKETHASH_COMPARE	 compare_keys
#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
					,
					VBUCKETHASH_EXTRACT_KEY extract_key
#endif
					);

/** 
 *  @brief Object constructor; initialise the hash that allows one entry for a given key.
 *  This function creates a bucket hash with 

 *  @param ctx			(in) allocator interface.
 *  @param hash			(out) the object.
 *  @param buckets		(in) number of buckets - will be rounded to closes power of two (i.e. number of arrays of linked lists structures, each separate list holds elements with same hash number).
 *  @param hash_func	(in) pointer to hash function
 *  @param compare		(in) pointer to compare function; compares if two keys are equal, (called when hash value of two entries is equal)
 
 *  @returns 0 if ok
 */
V_EXPORT int VBUCKETHASH_init_uniquemap(
					
					VCONTEXT	*ctx,
					VBUCKETHASH	*hash, 
					
					size_t		buckets,

					VBUCKETHASH_FUNCTION hash_func,
					VBUCKETHASH_COMPARE	 compare_keys
#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
					,
					VBUCKETHASH_EXTRACT_KEY extract_key
#endif					
					);

/**
 * @brief set resize policy
 
 * The resize policy determines how much space is allocated in advance, given
 * that the container has to be resized.

 * @brief arr (in out) the object
 * @brief sizepolicy (in) amount 
 */
V_INLINE void  VBUCKETHASH_set_sizepolicy( VBUCKETHASH *hash, VRESIZE_policy sizepolicy)
{
	hash->sizepolicy = sizepolicy;
}

/** 
 * @brief resize the hash table

 * Change number of buckets in hash table. 
 * The function pass over each element in the hash table and insert it into the resized table.
 
 * @param hash (in out) the object
 * @param buckets (in) new number of buckets
 */
V_EXPORT int VBUCKETHASH_resize(VBUCKETHASH	*hash, size_t buckets);

/**
 * @brief Object destructor; frees a hash table.

 * All elements currently contained in hash table will remain in memory.

 * @param hash (in out) the object
 */
V_INLINE void	VBUCKETHASH_free(VBUCKETHASH		*hash)
{
	if (hash->ctx) {
		if (hash->buckets) {
			V_FREE(hash->ctx, hash->buckets);
			hash->buckets = 0;
		}
	}
}



/** 
 * @brief returns number of elements in hash table
 */
V_INLINE size_t VBUCKETHASH_size( VBUCKETHASH *hash) 
{
	return hash->elmcount;
}

#if 0
/** 
 * @brief returns maximum number of elements that can currently be held by this collection.
 * @param  hash (in) the object
 * @return 
 */
V_INLINE size_t VBUCKETHASH_maxsize( VBUCKETHASH *hash) 
{
	return hash->buckets_size;
}
#endif

/**
 * @brief find entry with given key in hash table; for multimaps this will return the first occurence of the key.

 * @param phash		(in) the object
 * @param key		(in) pointer to key
 * @param key_size	(in) size of key 
 
 * @return pointer to entry if found; NULL if not found
 */
V_EXPORT VBUCKETHASH_Entry *VBUCKETHASH_find( VBUCKETHASH *phash, void *key, size_t key_size );


/**
 * @brief for multimaps - find next occurence of key
 *
 * Usage to retrive all occurences of a key 
 *
 *
 * VBUCKETHASH_Entry * entry;
 * for( entry = VBUCKETHASH_find(hash, key, sizeof(key) ) ; entry; 
 *			entry =  VBUCKETHASH_find_next(hash, entry, key, sizeof(key) ) {
 *
 * }
 *
 *
 * @param phash (in) the object
 * @param prev	(in) value returned by first invocation of VBUCKETHASH_find or previous invocation of VBUCKETHASH_find_next
 * @param key	(in) pointer to key
 * @param key_size (in) size of key. If key is a null terminated string then pass VHASH_STRING as value.
 * @return pointer to entry if found; NULL if not found
 */
V_EXPORT VBUCKETHASH_Entry *VBUCKETHASH_find_next( VBUCKETHASH *phash, VBUCKETHASH_Entry *prev, void *key, size_t key_size );

/**
 * @brief insert new entry in hash table
 * @param hash (in) the object
 * @param key  (in) key 
 * @param size (in) size of key. If key is a null terminated string then pass VHASH_STRING as value.
 */
V_EXPORT int VBUCKETHASH_insert( VBUCKETHASH *hash, VBUCKETHASH_Entry *entry, void *key, size_t key_size );

/**
 * @brief find first hash table entry and unlink it from its bucket. The caller of this function has to free memory held by hash table entry.
 * @param hash
 * @param key
 * @param size
 * @returns if entry with given keys exists - pointer to hash table entry.
 */
V_EXPORT VBUCKETHASH_Entry * VBUCKETHASH_unlink( VBUCKETHASH *hash, void *key, size_t key_size );


/* internal macro not directly used by user */
#define VBUCKETHASH_BUCKET_FOREACH(cur,bucket) \
	for((cur) = (VBUCKETHASH_Entry *) ((bucket)->next);\
		(VSRING *) (cur) != (bucket);\
		(cur) = (VBUCKETHASH_Entry *) (cur)->entry.next )


/**
 * @brief Macro: iterate over all elements that match a given key (multimap)
 *
 * Usage:
 *		VBUCKETHASH_Entry *cur;
 *
 *		VBUCKETHASH_FOREACH_KEY(cur,hash)  {
 *			=do something with cur - don't delete it=
 *		}
 *
 * @param cur  (type VBUCKETHASH_Entry *) current entry
 * @param hash (type VBUCKETHASH *) the object
 * @param key  (type void *) the key
 * @param key_size  (type size_t) length of key
 */
#define VBUCKETHASH_FOREACH_KEY(cur,hash,key,key_size) \
	 for( (cur) = VBUCKETHASH_find( (hash), (key), (key_size ) ) ; \
		  (cur); \
   		  (cur) =  VBUCKETHASH_find_next( (hash), (cur), (key), (key_size) ) )


/**
 * @brief Macro: unlink al entries that match a given key (for multimap); the user has to free the memory in loop code. 
 
 * @param cur  (type VBUCKETHASH_Entry *) current entry
 * @param hash (type VBUCKETHASH *) the object
 * @param key  (type void *) the key
 * @param key_size  (type size_t) length of key
 
 */
#define VBUCKETHASH_DELETEALL_KEY(cur,hash,key,key_size) \
{  \
	VBUCKETHASH_Entry * VBUCKETHASH_DELETEALL_KEY##next = (VBUCKETHASH_Entry *) 0xFFFFFFFF; \
	for( (cur) = VBUCKETHASH_unlink( (hash), (key), (key_size ) ) ; \
		 (cur) && (VBUCKETHASH_DELETEALL_KEY##next); \
		 (VBUCKETHASH_DELETEALL_KEY##next) =  VBUCKETHASH_find_next( (hash), (cur), (key), (key_size) ) ) { \
		 if ((VBUCKETHASH_DELETEALL_KEY##next) != (VBUCKETHASH_Entry *) 0xFFFFFFFF) { \
			 VSRING_unlink_after( (VSRING *) (cur) ); \
			 (hash)->elmcount--; \
			 (cur) = (VBUCKETHASH_DELETEALL_KEY##next); \
		 }

#define VBUCKETHASH_DELETEALL_KEY_END \
	} \
}	


/**
 * @brief Macro: iterate over all elements in hash table
 * @param cur (type VBUCKETHASH_Entry *)
 * @param hash (type VBUCKETHASH *)
 */
#define VBUCKETHASH_FOREACH(cur,hash) \
{ \
	size_t VBUCKETHASH_foreach_i##cur = 0; \
	for(VBUCKETHASH_foreach_i##cur = 0; VBUCKETHASH_foreach_i##cur < (hash)->buckets_size; VBUCKETHASH_foreach_i##cur ++) \
		VBUCKETHASH_BUCKET_FOREACH( cur, &(hash)->buckets[ VBUCKETHASH_foreach_i##cur ] ) {
		


/** @brief Macro: close block opened by VBUCKETHASH_FOREACH
 */
#define VBUCKETHASH_FOREACH_END \
	}\
}

/**
 * @brief Macro: iterate over all elements in hash table and delete them from the table; allow the user to free the memory of each element.
 * @param cur (type VBUCKETHASH_Entry *)
 * @param hash (type VBUCKETHASH *)
 */
#define VBUCKETHASH_DELETEALL(cur,hash) \
{ \
	size_t VBUCKETHASH_delallforeach_i##cur = 0; \
	for(VBUCKETHASH_delallforeach_i##cur = 0; VBUCKETHASH_delallforeach_i##cur < (hash)->buckets_size; VBUCKETHASH_delallforeach_i##cur ++) \
	while( !VSRING_isempty( &(hash)->buckets[ VBUCKETHASH_delallforeach_i##cur ]) ) { \
		(cur) = (VBUCKETHASH_Entry *) VSRING_unlink_after( &(hash)->buckets[ VBUCKETHASH_delallforeach_i##cur ] );\
		(hash)->elmcount--;

/** 
 * @brief Macro: close block opened by VBUCKETHASH_DELETEALL
 */			
#define VBUCKETHASH_DELETEALL_END \
	}\
}


/** 
 * @brief iterate over all entries of the hash table that match a given key and invoke callback with those elements.
 *
 * @param hash		(in) the object

 * @param key		(in) key
 * @param key_size	(in) size of key

 * @param eval_func	(in) function invoked for every element in hash table.
 * @param context	(in) data pointer passed to every invocation of eval_func
 */

V_INLINE void VBUCKETHASH_foreach_key( VBUCKETHASH *hash, void *key, size_t key_size, VBUCKETHASH_VISITOR_V eval_func, void *context )
{
	VBUCKETHASH_Entry *cur;

	if (!eval_func) {
		return;
	}
	VBUCKETHASH_FOREACH_KEY(cur,hash,key,key_size) {
		eval_func( cur, context);
	}
}

/** 
 * @brief iterate over all entries with the given key in the hash table and deletes them.
 
 * @param hash		(in) the object
 
 * @param key		(in) key
 * @param key_size	(in) size of key

 * @param free_ctx	(in) allocation interface used to free data of entry.
 * @param offset_of_link (in) offset of VBUCKETHASH_Entry in embedded structure.
 */
V_INLINE void VBUCKETHASH_deletall_key( VBUCKETHASH *hash, void *key, size_t key_size, 
									    VBUCKETHASH_VISITOR_V on_delete, void *context, 
										VCONTEXT *free_ctx, int offset_of_link)
{
	VBUCKETHASH_Entry *cur;


	VBUCKETHASH_DELETEALL_KEY(cur,hash,key,key_size)		
		if (on_delete) {
			on_delete(cur, context );
		}
	    if (free_ctx) {
			V_FREE( free_ctx, V_MEMBEROF(cur,offset_of_link) );
		}
	VBUCKETHASH_DELETEALL_KEY_END
}

/** 
 * @brief iterate over all entries of the hash table and invoke callback with each element.
 * Equivalent of Lisp foldl,mapcar and friends.
 * @param hash 
 * @param eval_func  function invoked for every element in hash table.
 * @param context data pointer passed to every invocation of eval_func
 */
V_INLINE void VBUCKETHASH_foreach( VBUCKETHASH *hash, VBUCKETHASH_VISITOR_V eval_func, void *context )
{
	VBUCKETHASH_Entry *cur;

	if (!eval_func) {
		return;
	}

	VBUCKETHASH_FOREACH(cur,hash)
		eval_func( cur, context);
	VBUCKETHASH_FOREACH_END
}


/**
 * @brief find an element within the hash. callback is invoked for each element of the list, when the callback returns non zero value the iteration stops as we have found what we searched for. 
 * 
 * @param hash (in) the object.
 * @param eval_func (in) callback that is called to visit each set (or cleared) bit.
 * @param context (in) pointer that is passed as context on each invocation of callback.
 * @param retval (out) optional - the first non zero value returned by eval callback, if NULL then ignored.
 * @return the list element that has matched (i.e. element that has been found).
 *
 */
V_INLINE VBUCKETHASH_Entry * VBUCKETHASH_findif( VBUCKETHASH *hash, VBUCKETHASH_VISITOR eval_func, void *context, int *retval)
{
	size_t i;
	VSRING *cur;
	int ret;	

	if (!eval_func) {
		return 0;
	}

	for(i=0; i<hash->buckets_size; i++) {
			VSRING_FOREACH( cur, &hash->buckets[i] ) {
					ret = eval_func( (VBUCKETHASH_Entry*) cur, context);
					if (ret) {
						if (retval) {
							*retval = ret;
						}
						return (VBUCKETHASH_Entry *) cur;
					}		
			}
	}	

	return 0;
}



/** 
 * @brief iterate over all entries of the hash table and deletes entries that match predicate from the hash, and frees the memory (optionally)
 * @param hash (in) the object.
 * @param check_if (in) predicate function; the function returns 1 then inspected argument element will be deleted; if argument pointer NULL then all entries will be deleted. 
 * @param context (in) data pointer passed to every invocation of check_if
 * @param free_ctx (in) allocation interface used to free data of entry; if 0 then no memory will be released.
 * @param offset_of_link (in) offset of VBUCKETHASH_Entry in embedded structure.
 */
V_INLINE void VBUCKETHASH_deleteif( VBUCKETHASH *hash, VBUCKETHASH_VISITOR check_if, void *context, VCONTEXT *free_ctx, int offset_of_link)
{
	VSRING  *cur,*prev, *eof,*del;
	size_t i = 0;


	for(i = 0; i < hash->buckets_size; i ++) {

		eof = prev = &hash->buckets[i];
		cur = prev->next;

		while(cur != eof) {

			if (!check_if || check_if( (VBUCKETHASH_Entry*) cur, context))  {
			   hash->elmcount--;
			   cur = cur->next;
			   del = VSRING_unlink_after(prev);
			   if (free_ctx) {
					V_FREE( free_ctx, V_MEMBEROF(del,offset_of_link) );
			   }
			} else {
				cur = cur->next;
				prev = cur;
			}
		}
	}
}


/** 
 * @brief iterate over all entries of the hash table and delete them.
 * @param hash			 (in) the object 
 * @param free_ctx		 (in) allocation interface used to free data of entry.
 * @param offset_of_link (in) offset of VBUCKETHASH_Entry in embedded structure.
 */
V_INLINE void VBUCKETHASH_deleteall( VBUCKETHASH *hash, VBUCKETHASH_VISITOR_V on_delete, void *context, VCONTEXT *free_ctx, int offset_of_link)
{
	VBUCKETHASH_Entry *cur;

	VBUCKETHASH_DELETEALL(cur,hash);
		if (on_delete) {
			on_delete(cur,context);
		}
		if (free_ctx) {
			V_FREE( free_ctx, V_MEMBEROF(cur,offset_of_link) );
		}
	VBUCKETHASH_DELETEALL_END
}

/**
 * @brief check consistency of bucket hash structure.
 */
V_INLINE int VBUCKETHASH_check(VBUCKETHASH *hash)
{
	size_t	i,nbuck = hash->buckets_size;
	size_t  count_of_elem = 0;
	VBUCKETHASH_Entry  *cur;
	
	for(i=0; i < nbuck; i++) {
		if (!VSRING_check(hash->buckets + i)) {
		   return 0;
		}
	}

	VBUCKETHASH_FOREACH(cur,hash)
		count_of_elem++;
	VBUCKETHASH_FOREACH_END
	
	if (count_of_elem != hash->elmcount) {
		return 0;
	}

	return 1;
}


#ifdef  __cplusplus
}
#endif

#endif

