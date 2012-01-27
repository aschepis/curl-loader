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

#ifndef _VCLOSEDHASH_H_
#define _VCLOSEDHASH_H_


#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>

#define CLOSEDHASH_EMPTY   0
#define CLOSEDHASH_DELETED 1
#define CLOSEDHASH_RESERVED_HASH 2

typedef V_UINT32 VCLOSEDHASH_VALUE;

typedef struct  {
	VCLOSEDHASH_VALUE hashvalue;
	V_UINT8	key[];
} VCLOSEDHASH_HEADER;


typedef VCLOSEDHASH_VALUE	(*VCLOSEDHASH_HASH_FN)	 (void * key, size_t key_length);
typedef int					(*VCLOSEDHASH_COMPARE_FN)(void * key1, void * key2, size_t key_length);
typedef size_t				(*VCLOSEDHASH_REHASH_FN) (void * key, size_t key_length, size_t num_probes);
	


V_INLINE size_t VCLOSEDHASH_linear_rehash(void * key2, size_t key_length, size_t num_probes)
{
	V_UNUSED(key2);
	V_UNUSED(key_length);
	V_UNUSED(num_probes);
	
	return 1;
}

V_INLINE size_t VCLOSEDHASH_quadratic_rehash(void * key2, size_t key_length, size_t num_probes){
	V_UNUSED(key2);
	V_UNUSED(key_length);
	V_UNUSED(num_probes);

	return num_probes;
}


/**
 * @brief Hash table that is implemented as closed hash table.
 *
 * Following text is quoted from From http://en.wikipedia.org/wiki/Hash_table
 *
 * Open addressing hash tables store the records directly within the array. 
 * This approach is also called closed hashing. A hash collision is resolved by 
 * probing, or searching through alternate locations in the array (the probe sequence) 
 * until either the target record is found, or an unused array slot is found, which
 * indicates that there is no such key in the table. Well known probe sequences 
 * include:
 *
 * linear probing - in which the interval between probes is fixed--often at 1.
 *
 * quadratic probing - in which the interval between probes increases linearly (hence, the indices are described by a quadratic function).
 *
 * double hashing - in which the interval between probes is fixed for each record but is computed 
 *   by another hash function.
 *
 * The main tradeoffs between these methods are that linear probing has the best 
 * cache performance but is most sensitive to clustering, while double hashing has 
 * poor cache performance but exhibits virtually no clustering; quadratic probing 
 * falls in-between in both areas. Double hashing can also require more computation 
 * than other forms of probing. Some open addressing methods, such as last-come-first-served hashing and cuckoo hashing move existing keys around in the array to make room for the new key. This gives better maximum search times than the methods based on probing.
 *
 * A critical influence on performance of an open addressing hash table is the load factor; 
 * that is, the proportion of the slots in the array that are used. As the load factor 
 * increases towards 100%, the number of probes that may be required to find or insert 
 * a given key rises dramatically. Once the table becomes full, probing algorithms may 
 * even fail to terminate. Even with good hash functions, load factors are normally 
 * limited to 80%. A poor hash function can exhibit poor performance even at very low 
 * load factors by generating significant clustering. What causes hash functions to 
 * cluster is not well understood, and it is easy to unintentionally write a hash 
 * function which causes severe clustering.
 
 */
typedef struct  {
	VCONTEXT *ctx;
	VRESIZE_policy	sizepolicy;

	VCLOSEDHASH_HASH_FN    hash;
	VCLOSEDHASH_COMPARE_FN compare;
	VCLOSEDHASH_REHASH_FN  rehash;
	
	size_t      keysize;
	size_t		datasize;
	size_t		elmsize;

	size_t      elmmaxcount;
	size_t      elmcount;
	size_t		resize_threshold;

	V_UINT8  	*data;

} VCLOSEDHASH;


typedef void	(*VCLOSEDHASH_VISITOR_V)	(VCLOSEDHASH *hash, void *key, size_t key_size, void *value, size_t value_size, void *context);
typedef int		(*VCLOSEDHASH_VISITOR)		(VCLOSEDHASH *hash, void *key, size_t key_size, void *value, size_t value_size, void *context);


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
V_EXPORT int  VCLOSEDHASH_set_resize_trigger( int multiplier, int divisor );

/**
 * @brief get current load factor of hash table that will trigger resize of hash table
 *
 * @param multiplier (out) multiplier of rational number
 * @param divisor	 (out) divisor of rational number
 */
V_EXPORT void VCLOSEDHASH_get_resize_trigger( int *multiplier, int *divisor );

V_EXPORT int VCLOSEDHASH_init_uniqemap(

					VCONTEXT	*ctx,
					VCLOSEDHASH	*hash,

					size_t		 keysize,
					size_t		 datasize,
					V_UINT32	 numelements,

					VCLOSEDHASH_HASH_FN		hash_func,
					VCLOSEDHASH_COMPARE_FN	compare,
					VCLOSEDHASH_REHASH_FN	rehash
					);


/**
 * @brief Object destructor; frees a hash table.
 * All elements currently contained in hash table will remain in memory.
 * @param hash (in) the object
 */

V_INLINE void	VCLOSEDHASH_free(VCLOSEDHASH		*hash)
{
	if (hash->ctx) {
		if (hash->data) {
			V_FREE(hash->ctx, hash->data);
			hash->data = 0;
		}
	}
}



/** 
 * @brief resize the hash table

 * Change number of buckets in hash table. 
 * The function pass over each element in the hash table and insert it into the resized table.
 
 * @param new number of buckets
 */
V_EXPORT int VCLOSEDHASH_resize(VCLOSEDHASH	*hash, size_t buckets);

/**
 * @brief returns number of objects that are currently held by this collection.
 * @param  arr (in) the object
 * @return 
 */
V_INLINE size_t VCLOSEDHASH_size( VCLOSEDHASH *hash) 
{
	return hash->elmcount;
}

/** 
 * @brief returns maximum number of elements that can currently be held by this collection.
 * @param  arr (in) the object
 * @return 
 */
V_INLINE size_t VCLOSEDHASH_maxsize( VCLOSEDHASH *hash )
{
	return hash->elmmaxcount;
}

/**
 * @brief find entry with given key in hash table; 

 * @param hash		(in) the object
 * @param key		(in) pointer to key
 * @param key_size	(in) size of key 
 * @param size		(out) optional if not null - returns size of entry.
 
 * @return pointer to entry if found; NULL if not found
 */
V_EXPORT void * VCLOSEDHASH_find(VCLOSEDHASH *hash, void *key, size_t key_size, size_t *size);

/**
 * @brief insert new entry in hash table
 * @param hash (in) the object
 
 * @param key  (in) key 
 * @param size (in) size of key.

 * @param data (in)	data 
 * @param data_size (in) size of data.
 
 */
V_EXPORT int	VCLOSEDHASH_insert(VCLOSEDHASH *hash, void *key, size_t key_size, void *data, size_t data_size);

V_EXPORT int	VCLOSEDHASH_delete(VCLOSEDHASH *hash, void *key, size_t key_size, VCLOSEDHASH_VISITOR_V delete_callback, void *context);


#define VCLOSEDHASH_FOREACH( key, key_type, value, value_type, hash) \
{ \
	VCLOSEDHASH_HEADER *VCLOSED_hash_header##key = (VCLOSEDHASH_HEADER *) hash->data; \
	size_t VCLOSED_hash_elmsize##key = (hash)->elmsize + sizeof(VCLOSEDHASH_HEADER); \
	size_t VCLOSED_hash_i##key = 0; \
	for(;VCLOSED_hash_i##key < (hash)->elmmaxcount; \
		 VCLOSED_hash_i##key ++,VCLOSED_hash_header##key = VPTR_ADD(VCLOSED_hash_header##key, VCLOSED_hash_elmsize##key, VCLOSEDHASH_HEADER *) ) { \
		if (VCLOSED_hash_header##key ->hashvalue > CLOSEDHASH_DELETED) { \
			(key) =  (key_type) (VCLOSED_hash_header##key + 1); \
			(value) = (value_type) ( ((V_UINT8 *) (VCLOSED_hash_header##key + 1)) + (hash)->keysize );
		
#define VCLOSEDHASH_FOREACH_END \
		}\
	} \
}

#define VCLOSEDHASH_DELETEALL( key, key_type, value, value_type, hash) \
{ \
	VCLOSEDHASH_HEADER *VCLOSED_hash_header##key = (VCLOSEDHASH_HEADER *) hash->data; \
	size_t VCLOSED_hash_i##key = 0; \
	size_t VCLOSED_hash_elmsize##key = (hash)->elmsize + sizeof(VCLOSEDHASH_HEADER); \
	for(;VCLOSED_hash_i##key < (hash)->elmmaxcount; \
		 VCLOSED_hash_i##key ++,VCLOSED_hash_header##key = VPTR_ADD(VCLOSED_hash_header##key, VCLOSED_hash_elmsize##key, VCLOSEDHASH_HEADER *) ) { \
		if (VCLOSED_hash_header##key ->hashvalue == CLOSEDHASH_DELETED) { \
			VCLOSED_hash_header##key ->hashvalue = CLOSEDHASH_EMPTY; \
		} \
		if (VCLOSED_hash_header##key ->hashvalue > CLOSEDHASH_DELETED) { \
			VCLOSED_hash_header##key ->hashvalue = CLOSEDHASH_EMPTY; \
			(hash)->elmcount -- ; \
			(key) =  (key_type) (VCLOSED_hash_header##key + 1); \
			(value) = (value_type) ( ((V_UINT8 *) (VCLOSED_hash_header##key + 1)) + (hash)->keysize );

#define VCLOSEDHASH_DELETEALL_END \
		}\
	} \
}

/** 
 * @brief iterate over all entries of the hash table and invoke callback with each element.
 * Equivalent of Lisp foldl,mapcar and friends.
 * @param hash 
 * @param eval_func  function invoked for every element in hash table.
 * @param context data pointer passed to every invocation of eval_func
 */
V_INLINE void VCLOSEDHASH_foreach( VCLOSEDHASH *hash, VCLOSEDHASH_VISITOR_V eval_func, void *context )
{
    void *key,*value;
	size_t key_size,value_size;

	if (!eval_func) {
		return;
	}

	key_size = hash->keysize;
	value_size = hash->datasize;

	VCLOSEDHASH_FOREACH(key, void * , value, void *, hash)
		eval_func( hash, key, key_size, value, value_size, context);
	VCLOSEDHASH_FOREACH_END
}


typedef struct {
	
   void *key;
   size_t key_size;
   void *value;
   size_t value_size;

}  VCLOSEDHASH_keyvalue;

/**
 * @brief find an element within hash. callback is invoked for each element of the list; when the callback returns non zero value the iteration stops as we have found what we searched for. 
 * 
 * @param hash (in) the object.
 * @param eval_func (in) callback that is called to visit each set (or cleared) bit.
 * @param context (in) pointer that is passed as context on each invocation of callback.
 * @param rpos (out) optional - key value and data value of element that has matched, ignored if NULL.
 * @param retval (out) optional - the first non zero value returned by eval callback, ignored if NULL.
 * @return 0 if no element hash matched,
 *
 */
V_INLINE int VCLOSEDHASH_findif( VCLOSEDHASH *hash, VCLOSEDHASH_VISITOR eval_func, void *context, VCLOSEDHASH_keyvalue *rpos, int *retval)
{
    void *key,*value;
	size_t key_size,value_size;
	int ret;

	if (!eval_func) {
		return 0;
	}

	key_size = hash->keysize;
	value_size = hash->datasize;

	VCLOSEDHASH_FOREACH(key, void * , value, void *, hash)

		ret = eval_func(hash, key, key_size, value, value_size, context );
		if (ret) {
			if (retval) {
				*retval = ret;
			}
			if (rpos) {
				VCLOSEDHASH_keyvalue rval;

				rval.key = key;
				rval.key_size = key_size;
				rval.value = value;
				rval.value_size = value_size;

				*rpos = rval;
			}
			return 1;
		}		
	VCLOSEDHASH_FOREACH_END

	return 0;
}



/** 
 * @brief iterate over all entries of the hash table and deletes entries that match predicate from the hash, and frees the memory (optionally)
 * @param hash (in) the object.
 * @param check_if (in) predicate function; the function returns 1 then inspected argument element will be deleted; if argument pointer NULL then all entries will be deleted. 
 * @param context (in) data pointer passed to every invocation of check_if
 */
V_INLINE void VCLOSEDHASH_deleteif( VCLOSEDHASH *hash, VCLOSEDHASH_VISITOR check_if, void *context)
{
    void *key,*value;
	size_t key_size,value_size;

	key_size = hash->keysize;
	value_size = hash->datasize;

	VCLOSEDHASH_FOREACH(key, void * , value, void *, hash)

		if (!check_if || 
			check_if( hash, key, key_size, value, value_size, context)) {

			VCLOSED_hash_headerkey->hashvalue = CLOSEDHASH_DELETED;
			hash->elmcount --;

		}
	VCLOSEDHASH_FOREACH_END
		
}

/** 
 * @brief iterate over all entries of the hash table and delete them.
 * @param hash			 (in) the object 
 */
V_INLINE void VCLOSEDHASH_deleteall( VCLOSEDHASH *hash, VCLOSEDHASH_VISITOR_V on_delete, void *context)
{
	void *key,*value;
	size_t key_size,value_size;

	key_size = hash->keysize;
	value_size = hash->datasize;

	VCLOSEDHASH_DELETEALL(key, void * , value, void *, hash)
		if (on_delete) {
			on_delete(hash, key, key_size, value, value_size, context);
		}
	VCLOSEDHASH_DELETEALL_END

	hash->elmcount = 0;
}

/**
 * @brief check consistency of closed hash structure.
 */
V_INLINE int VCLOSEDHASH_check(VCLOSEDHASH *hash)
{
	void *key,*value;
	size_t count_of_elem = 0;

	if (hash->elmcount > hash->elmmaxcount) {
		return 0;
	}

	if ((hash->keysize + hash->datasize) != hash->elmsize) {
		return 0;
	}

	VCLOSEDHASH_FOREACH(key, void * , value, void *, hash)
		count_of_elem++;
	VCLOSEDHASH_FOREACH_END
	
	if (count_of_elem != hash->elmcount) {
		return 0;
	}

	return 1;
}

#ifdef  __cplusplus
}
#endif

#endif

