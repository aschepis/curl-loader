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

#include <util/vbuckethash.h>
#include <util/vutil.h>
#include <util/vhashfunction.h>



#define ADJUST_HASH(hash,buckets) ((hash) & ((buckets) - 1))

static int g_multiplier = 2;
static int g_divisor = 3;

V_EXPORT int VBUCKETHASH_set_resize_trigger( int multiplier, int divisor )
{
	if (g_multiplier > divisor) {
		return -1;
	}
	
	g_multiplier = multiplier;
	g_divisor = divisor;

	return 0;
}


V_EXPORT void VBUCKETHASH_get_resize_trigger( int *multiplier, int *divisor )
{
	if (multiplier) {
		*multiplier = g_multiplier;
	}
	if (divisor) {
		*divisor = g_divisor;
	}
}


V_EXPORT int VBUCKETHASH_init(

					VCONTEXT			*ctx,
					VBUCKETHASH			*hash,

					size_t				 buckets, 
					int                  ismultimap,

					VBUCKETHASH_FUNCTION hash_func,
					VBUCKETHASH_COMPARE	 compare
#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
					,
					VBUCKETHASH_EXTRACT_KEY extract_key
#endif										
					)
{
	size_t i;

	buckets = VUTIL_round_to_power_of_n( buckets );

	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	if (!ctx) {
		return -1;
	}

	if (!hash_func) {
		hash_func = VHASHFUNCTION_Bob_Jenkins_one_at_a_time;
	}

	if (!compare) {
		return -1;
	}

	hash->elmcount = 0;
	hash->hash_func = hash_func;
	hash->compare_func = compare;

#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
	hash->extract_key = extract_key;
#endif												

	hash->ctx = ctx;
	hash->ismultimap = ismultimap;

	hash->buckets = (VSRING*) V_MALLOC( ctx, sizeof(VSRING) * buckets );
	if (!hash->buckets) {
		return 1;
	}
	hash->buckets_size = buckets;
	hash->resize_threshold = (buckets * g_multiplier) / g_divisor;
	hash->sizepolicy	= VRESIZE_init_multiply(2);

	for(i=0;i<buckets;i++) {
		VSRING_init(  &hash->buckets[i] );
  	}
	return 0;
}

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
					)
{
	return VBUCKETHASH_init(ctx, hash, buckets, 1 ,hash_func,compare_keys
#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
					,
					extract_key
#endif														
		);
}

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
					)
{
	return VBUCKETHASH_init(ctx, hash, buckets, 0 ,hash_func,compare_keys
#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
					,
					extract_key
#endif												
		);
}

V_EXPORT int VBUCKETHASH_resize(VBUCKETHASH	*hash, size_t buckets)
{
	VSRING *pbuckets;
	size_t i;
	VBUCKETHASH_Entry *cur;
	size_t elmcount;
#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
	size_t size;
	void *data;
#endif

	buckets = VUTIL_round_to_power_of_n( buckets );

	pbuckets = V_MALLOC( hash->ctx, sizeof(VSRING) * buckets);
	if (!pbuckets) {
		return -1;
	}
	for(i=0;i<buckets;i++) {
		VSRING_init( pbuckets + i );
	}

	elmcount = hash->elmcount;

	VBUCKETHASH_DELETEALL(cur,hash)
#ifdef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
		data = hash->extract_key( cur, &size );
		i = ADJUST_HASH(  hash->hash_func( data , size ), buckets);
		VSRING_push_front( pbuckets + i, (VSRING *) cur);
#else		
		i = ADJUST_HASH(cur->hash, buckets);
		VSRING_push_front( pbuckets + i, (VSRING *) cur);
#endif
	VBUCKETHASH_DELETEALL_END

	V_FREE( hash->ctx, hash->buckets );
	hash->buckets = pbuckets;
	hash->buckets_size = buckets;
	hash->elmcount = elmcount;
	hash->resize_threshold = (buckets * g_multiplier) / g_divisor;

	return 0;

}


V_INLINE VBUCKETHASH_Entry *VBUCKETHASH_find_in_bucket(	
					
					VSRING			*abucket, 
					VBUCKETHASH_VALUE   	hash, 
					VBUCKETHASH_COMPARE	compare_func,
					void			*key, 
					size_t			key_size )
{
	VSRING	*pos;

	VSRING_FOREACH(pos, abucket) {
		VBUCKETHASH_Entry *entry = (VBUCKETHASH_Entry *) pos;
#ifndef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
		if (entry->hash == hash) 
#endif		
		{
		  
			if (compare_func( entry, key, key_size ) == 0) {
				return entry;
			}
		}
	}
 	return 0;
}

V_EXPORT VBUCKETHASH_Entry *VBUCKETHASH_find( VBUCKETHASH *phash, void *key, size_t key_size )
{
	VBUCKETHASH_VALUE hash = phash->hash_func( key, key_size );
	int		bucket =   ADJUST_HASH(hash, phash->buckets_size);
	VSRING	*abucket = &phash->buckets[ bucket ];   
	
	return VBUCKETHASH_find_in_bucket( abucket, hash, phash->compare_func, key, key_size );
}

V_EXPORT VBUCKETHASH_Entry *VBUCKETHASH_find_next( VBUCKETHASH *phash, VBUCKETHASH_Entry *prev, void *key, size_t key_size )
{
	VBUCKETHASH_Entry *next;

	if (!phash->ismultimap) {
		return 0;
	}

	next = (VBUCKETHASH_Entry *) prev->entry.next;
	if (!next) {
		return 0;
	}
#ifndef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
	if (next->hash != prev->hash) {
		return 0;
	}
#endif

	if (phash->compare_func( next, key, key_size ) != 0) {
		return 0;
	}
	return next;
}

V_EXPORT int VBUCKETHASH_insert( VBUCKETHASH *phash, VBUCKETHASH_Entry *entry, void *key, size_t key_size  )
{
	VBUCKETHASH_VALUE hash;
	int		bucket;
	VSRING	*abucket,*fnd;
	size_t	nextsize;

	if ((phash->elmcount + 1) >= phash->resize_threshold) {

		nextsize = VRESIZE_request(phash->sizepolicy, phash, phash->elmcount+1);
		if (nextsize) {
			VBUCKETHASH_resize(phash, nextsize);
		}
	}

	hash = phash->hash_func( key, key_size );	
#ifndef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
	entry->hash = hash;
#endif
	bucket =   ADJUST_HASH(hash, phash->buckets_size);
	abucket = &phash->buckets[ bucket ];   
	
	
	fnd = (VSRING *) VBUCKETHASH_find_in_bucket( abucket, hash, phash->compare_func, key, key_size );

	/* multimap allows only one entry with the given key */
	if (!phash->ismultimap && fnd) {
		return -1;
	}

	if (fnd) {
		VSRING_insert_after( fnd, &entry->entry );
	} else {
		VSRING_push_front( abucket,  &entry->entry );
	}
	phash->elmcount++;

	return 0;
}

V_EXPORT VBUCKETHASH_Entry * VBUCKETHASH_unlink( VBUCKETHASH *phash, void *key, size_t key_size )
{
	VBUCKETHASH_VALUE hash;
	int		bucket;
	VSRING	*abucket;
	VSRING	*pos,*prev;
	VBUCKETHASH_Entry *ret = 0;


	hash =  phash->hash_func( key, key_size );	
	bucket =   ADJUST_HASH(hash, phash->buckets_size);
	abucket = &phash->buckets[ bucket ];   

	prev = abucket;
	VSRING_FOREACH(pos, abucket) {
		VBUCKETHASH_Entry *entry = (VBUCKETHASH_Entry *) pos;
#ifndef VBUCKETHASH_NO_HASH_VALUE_IN_ENTRY
		if (entry->hash == hash) 
#endif		
		{
		  
			if (phash->compare_func( entry, key, key_size ) == 0) {
				ret = (VBUCKETHASH_Entry *)VSRING_unlink_after(prev);
				phash->elmcount--;	
				return ret;
			}
		}
		prev = pos;
	}

	return 0;
}




