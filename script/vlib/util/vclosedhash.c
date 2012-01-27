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
#include <util/vclosedhash.h>
#include <util/vutil.h>
#include <util/vhashfunction.h>

static int g_multiplier = 2;
static int g_divisor = 3;

#define ADJUST_HASH(hash,buckets) ((hash) & ((buckets) - 1))


V_EXPORT int VCLOSEDHASH_set_resize_trigger( int multiplier, int divisor )
{
	if (g_multiplier > divisor) {
		return -1;
	}
	
	g_multiplier = multiplier;
	g_divisor = divisor;

	return 0;
}


V_EXPORT void VCLOSEDHASH_get_resize_trigger( int *multiplier, int *divisor )
{
	if (multiplier) {
		*multiplier = g_multiplier;
	}
	if (divisor) {
		*divisor = g_divisor;
	}
}


V_EXPORT int VCLOSEDHASH_init_uniqemap(

					VCONTEXT	*ctx,
					VCLOSEDHASH	*hash,

					size_t		 keysize,
					size_t		 datasize,
					V_UINT32	 numelements,

					VCLOSEDHASH_HASH_FN		hash_func,
					VCLOSEDHASH_COMPARE_FN	compare,
					VCLOSEDHASH_REHASH_FN	rehash
					)
{
	VCLOSEDHASH_HEADER * hdr;
	size_t i;
	size_t elmsize  = keysize + datasize;

	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	numelements		= VUTIL_round_to_power_of_n( numelements );

	hash->ctx	    = ctx;
	hash->sizepolicy= VRESIZE_init_multiply(2);
	hash->resize_threshold = (numelements * g_multiplier) / g_divisor;
		
	hash->elmsize	= elmsize;
	hash->keysize   = keysize;
	hash->datasize  = datasize;
	
	hash->elmmaxcount = numelements;
	hash->elmcount = 0;

	if (!compare) {
		return -1;
	}
	if (!hash_func) {
		hash_func = VHASHFUNCTION_Bob_Jenkins_one_at_a_time;
	}
	if (!rehash) {
		rehash = VCLOSEDHASH_linear_rehash;
	}

	hash->compare   = compare;
	hash->hash		= hash_func;
	hash->rehash	= rehash;

	hash->data		= V_MALLOC( ctx, (elmsize + sizeof(VCLOSEDHASH_HEADER)) * numelements );
	if (hash->data == 0) {
		return -1;
	}

	for(i =0; i < numelements; i++) {
		hdr = (VCLOSEDHASH_HEADER *) (hash->data + (elmsize + sizeof(VCLOSEDHASH_HEADER)) * i);
		hdr->hashvalue = CLOSEDHASH_EMPTY;
	}
	return 0;
}


static VCLOSEDHASH_HEADER *VCLOSEDHASH_find_internal(VCLOSEDHASH *hash, void *key, size_t keysize,
													 VCLOSEDHASH_HEADER **find_pos, VCLOSEDHASH_VALUE *hash_value)
{
	VCLOSEDHASH_VALUE val, cval; 
	VCLOSEDHASH_HEADER *hdr;
	size_t pos;
	size_t elmmaxcount;
	size_t elmsize_hdr;


	elmmaxcount = hash->elmmaxcount;
	elmsize_hdr = hash->elmsize + sizeof( VCLOSEDHASH_HEADER );

	if ( !hash_value || !(*hash_value) ) {

		val = hash->hash(key, keysize) & ~CLOSEDHASH_RESERVED_HASH;
		if (val < CLOSEDHASH_RESERVED_HASH) {
			val = CLOSEDHASH_RESERVED_HASH;
		}

	} else {

		val = *hash_value;
	}

	if (hash_value) {
		*hash_value = val;
	}

	cval =  ADJUST_HASH(val, elmmaxcount);

	for(pos=0; pos < elmmaxcount; ) {

		hdr = (VCLOSEDHASH_HEADER *) (hash->data + (elmsize_hdr * cval));

		if (hdr->hashvalue <= CLOSEDHASH_DELETED) {

			if (find_pos) {
			   *find_pos = hdr;
			}
			return 0;
		}

		if (hdr->hashvalue == val) {
			if (hash->compare( &hdr->key, key, keysize) == 0) {
				return hdr;
			}
		}

		cval = ADJUST_HASH( cval + hash->rehash( key, keysize, ++pos), elmmaxcount);
	
	}

	if (find_pos) {
	   *find_pos = 0;
	}

	return 0;
}

V_EXPORT void * VCLOSEDHASH_find(VCLOSEDHASH *hash, void *key,size_t key_size, size_t *size)
{
	VCLOSEDHASH_HEADER *hdr;

	if (hash->elmcount == 0) {
		return 0;
	}

	hdr = VCLOSEDHASH_find_internal(hash,key,key_size,0,0);
	if (!hdr) {
		return 0;
	}
	
	if (size) {
		*size = hash->keysize;
	}

	return hdr->key + hash->keysize;
}

V_EXPORT int VCLOSEDHASH_insert(VCLOSEDHASH *hash, void *key, size_t key_size, void *data, size_t data_size)
{
	VCLOSEDHASH_HEADER *hdr;
	size_t nextsize;
	VCLOSEDHASH_VALUE hashvalue = 0;

	if (key_size != hash->keysize) {
		return -1;
	}

	if (data_size != hash->datasize) {
		return -1;
	}

	if ((hash->elmcount + 1) >= hash->resize_threshold) {

		nextsize = VRESIZE_request(hash->sizepolicy, hash, hash->elmmaxcount);
		if (nextsize) {
			VCLOSEDHASH_resize(hash, nextsize);
		}
	}

	if (VCLOSEDHASH_find_internal(hash,key,key_size,&hdr,&hashvalue)) {
		return -1;
	}

	if (!hdr) {
		return -1;
	}

	hdr->hashvalue = hashvalue;
	memcpy(hdr->key, key, key_size);
	memcpy( (V_UINT8 *) hdr->key + key_size, data, data_size ); 

	hash->elmcount++;
	return 0;
}

static int VCLOSEDHASH_insert_for_resize(VCLOSEDHASH *hash, VCLOSEDHASH_VALUE hashvalue, void *key, void *data)
{
	VCLOSEDHASH_HEADER *hdr;
	size_t key_size  = hash->keysize;
	size_t data_size = hash->datasize;

	if (VCLOSEDHASH_find_internal(hash,key,key_size,&hdr,&hashvalue)) {
		return -1;
	}
	
	hdr->hashvalue = hashvalue;
	memcpy( hdr->key, key, key_size);
	memcpy( (V_UINT8 *) hdr->key + key_size, data, data_size ); 
	
	return 0;
}

V_EXPORT int VCLOSEDHASH_delete(VCLOSEDHASH *hash, void *key, size_t key_size, VCLOSEDHASH_VISITOR_V delete_callback, void *context)
{
	VCLOSEDHASH_HEADER *hdr;


	if (key_size != hash->keysize) {
		return -1;
	}

	if (!VCLOSEDHASH_find_internal(hash,key,key_size,&hdr,0)) {
		return -1;
	}
	
	if (delete_callback) {
		delete_callback(hash,
						hdr->key,key_size, 
						hdr->key + hash->keysize, hash->datasize,
						context);
	}

	hdr->hashvalue = CLOSEDHASH_DELETED;
    hash->elmcount--;
	return 0;
}


V_EXPORT int VCLOSEDHASH_resize(VCLOSEDHASH	*hash, size_t buckets)
{
    void *key,*value;
	VCLOSEDHASH_HEADER *hdr;
	VCLOSEDHASH new_hash;

	if (buckets < hash->elmcount) {
		return -1;
	}

	if (VCLOSEDHASH_init_uniqemap(

					hash->ctx,
					&new_hash,

					hash->keysize,
					hash->datasize,
					buckets,

					hash->hash,
					hash->compare,
					hash->rehash
					)) {
		return -1;
	}

	VCLOSEDHASH_FOREACH(key, void * , value, void *, hash)
		hdr = ((VCLOSEDHASH_HEADER *) key) - 1;

		/* add current entry into new table whiles reusing hash value */
		if (VCLOSEDHASH_insert_for_resize(&new_hash, hdr->hashvalue, 
			key, value )) {

			VCLOSEDHASH_free(&new_hash);
			return -1;
		}

	VCLOSEDHASH_FOREACH_END

	V_FREE( hash->ctx, hash->data );
	hash->data = new_hash.data;
	hash->resize_threshold = new_hash.resize_threshold;
	hash->elmmaxcount = new_hash.elmmaxcount;
		
	return 0;
}

