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

#include <util/vhashfunction.h>


/*
 * some application (like compilers and runtimes) would want to speed up 
 * hashing of strings - the method is to limit the number of characters passed to hash.
 * the assumption is that input strings are not just some random bunch of byte and are sufficiently different.
 * of interest for hashing. so this is an option that must be given at compile time.
 */
#ifdef _LIMIT_STRING_LENGTH_

#define LIMIT_STR_INIT  int limit_str_cnt = 0;
#define LIMIT_STR_CHECK	 && limit_str_cnt++ < 20

#else

#define LIMIT_STR_INIT  
#define LIMIT_STR_CHECK	 

#endif

V_EXPORT VBUCKETHASH_VALUE	VHASHFUNCTION_sample_hash_func(void * keydata, size_t key_length)
{
	unsigned char * key = (unsigned char *) keydata;
	VBUCKETHASH_VALUE hash_val = 0;
		
	if (key_length != VHASH_STRING) {
		while (key_length > 0) {
			hash_val = (hash_val<<5) + hash_val + *key++;	
			key_length --;
		}
		return hash_val;
	} else {
		LIMIT_STR_INIT

		while (*key  LIMIT_STR_CHECK) {
			hash_val = (hash_val<<5) + hash_val + *key++;
		}
		return hash_val;
	}

}

V_EXPORT VBUCKETHASH_VALUE	VHASHFUNCTION_PJW(void * keydata, size_t key_length)
{
	unsigned char * key = (unsigned char *) keydata;
	VBUCKETHASH_VALUE g = 0, hash_val = 0;

	if (key_length != VHASH_STRING) {
		while (key_length > 0) {
			hash_val = (hash_val<<4) + hash_val + *key++;	
			g = hash_val & 0xF0000000;
			if (g) {
				hash_val = hash_val ^ ( g >> 24);
				hash_val = hash_val ^ g;
			}
			key_length --;
		}
		return hash_val;
	} else {
		LIMIT_STR_INIT

		while (*key LIMIT_STR_CHECK) {
			hash_val = (hash_val<<4) + hash_val + *key++;	
			g = hash_val & 0xF0000000;
			if (g) {
				hash_val = hash_val ^ ( g >> 24);
				hash_val = hash_val ^ g;
			}
		}
		return hash_val;
	}
}

V_EXPORT VBUCKETHASH_VALUE VHASHFUNCTION_rotating(void * keydata, size_t key_length)
{
  VBUCKETHASH_VALUE hash;
  unsigned char * key = (unsigned char *) keydata;
  size_t i;

  if (key_length != VHASH_STRING) {
		for (hash = key_length, i=0; i<key_length; ++i) {
			hash = (hash<<4)^(hash>>28) ^ key[i];
		}
		return hash;
  } else {
		LIMIT_STR_INIT

		for (hash = ((size_t)*key) << 16; *key  LIMIT_STR_CHECK; ++key) {
			hash = (hash<<4)^(hash>>28) ^ (*key);
		}
		return hash;
  }
}

V_EXPORT VBUCKETHASH_VALUE VHASHFUNCTION_shift_and_xor(void * keydata, size_t key_length)
{
  VBUCKETHASH_VALUE hash;
  unsigned char * key = (unsigned char *) keydata;
  size_t i;

  if (key_length != VHASH_STRING) {
		for (hash = key_length, i=0; i<key_length; ++i) {
			hash ^= (hash<<5) + (hash>>2)  + key[i];
		}
		return hash;
  } else {
		LIMIT_STR_INIT

		for (hash = ((size_t)*key) << 16; *key LIMIT_STR_CHECK; ++key) {
			hash ^= (hash<<5) + (hash>>2)  + (*key);
		}
		return hash;
  }
}


V_EXPORT VBUCKETHASH_VALUE VHASHFUNCTION_Fowler_Noll_Vo(void * keydata, size_t key_length)
{
  VBUCKETHASH_VALUE hash = 2166136261U;
  unsigned char * key = (unsigned char *) keydata;
  size_t i;

  if (key_length != VHASH_STRING) {
		for (hash = 0, i=0; i<key_length; ++i) {
			hash = ( hash * 16777619U ) ^ key[i];
		}
		return hash;
  } else {
		LIMIT_STR_INIT

		for (hash = 0; *key LIMIT_STR_CHECK; ++key) {
			hash = ( hash * 16777619U ) ^ (*key);
		}
		return hash;
  }
}

V_EXPORT VBUCKETHASH_VALUE VHASHFUNCTION_Bob_Jenkins_one_at_a_time(void * keydata, size_t key_length)
{
  VBUCKETHASH_VALUE h = 2166136261U;
  unsigned char * key = (unsigned char *) keydata;
  size_t i;


  if (key_length != VHASH_STRING) {
	
	for ( i = 0; i < key_length; i++ ) {
		h += key[i];
		h += ( h << 10 );
		h ^= ( h >> 6 );
	}

	h += ( h << 3 );
	h ^= ( h >> 11 );
	h += ( h << 15 );

	return h;

  } else {
	LIMIT_STR_INIT

	for ( ; *key LIMIT_STR_CHECK; ++key ) {
		h += *key;
		h += ( h << 10 );
		h ^= ( h >> 6 );
	}

	h += ( h << 3 );
	h ^= ( h >> 11 );
	h += ( h << 15 );

	return h;
  }
}

V_EXPORT VBUCKETHASH_VALUE VHASHFUNCTION_ELF(void * keydata, size_t key_length)
{
  VBUCKETHASH_VALUE h = 2166136261U,g;
  unsigned char * key = (unsigned char *) keydata;
  size_t i;

  if (key_length != VHASH_STRING) {
	
	for ( i = 0; i < key_length; i++ ) {
		h = ( h << 4 ) + key[i];
		g = h & 0xf0000000L;

		if ( g != 0 )
			h ^= g >> 24;
		
		h &= ~g;
	}
	return h;
  
  } else {
	LIMIT_STR_INIT	

	for ( ; *key LIMIT_STR_CHECK;key++ ) {
		h = ( h << 4 ) + *key;
		g = h & 0xf0000000L;

		if ( g != 0 )
			h ^= g >> 24;
		
		h &= ~g;

		return h;
	}

	return h;
  }
}

 #define hashsize(n) ( 1U << (n) )
 #define hashmask(n) ( hashsize ( n ) - 1 )
 
 #define mix(a,b,c) \
 { \
   a -= b; a -= c; a ^= ( c >> 13 ); \
   b -= c; b -= a; b ^= ( a << 8 ); \
   c -= a; c -= b; c ^= ( b >> 13 ); \
   a -= b; a -= c; a ^= ( c >> 12 ); \
   b -= c; b -= a; b ^= ( a << 16 ); \
   c -= a; c -= b; c ^= ( b >> 5 ); \
   a -= b; a -= c; a ^= ( c >> 3 ); \
   b -= c; b -= a; b ^= ( a << 10 ); \
   c -= a; c -= b; c ^= ( b >> 15 ); \
 }
 
#define BJ_INIT_VAL 2166136261U

V_EXPORT VBUCKETHASH_VALUE VHASHFUNCTION_Bob_Jenkins(void * keydata, size_t length)
{
    unsigned char *k = keydata;
    unsigned a, b;
    unsigned c = BJ_INIT_VAL;
    unsigned len = length;
 
    a = b = 0x9e3779b9;

	if (length != VHASH_STRING) {
 
		while ( len >= 12 ) {
		   a += ( k[0] + ( (unsigned)k[1] << 8 ) 
			 + ( (unsigned)k[2] << 16 )
			 + ( (unsigned)k[3] << 24 ) );
		   b += ( k[4] + ( (unsigned)k[5] << 8 ) 
			 + ( (unsigned)k[6] << 16 )
			 + ( (unsigned)k[7] << 24 ) );
		   c += ( k[8] + ( (unsigned)k[9] << 8 ) 
			 + ( (unsigned)k[10] << 16 )
			 + ( (unsigned)k[11] << 24 ) );
 
		   mix ( a, b, c );
 
		   k += 12;
		   len -= 12;
		}
	} else {
		size_t pos_k = 0;
		unsigned char *next_k;
		LIMIT_STR_INIT	

		length = 0;

		while(1) {

			for(pos_k = 0, next_k = (unsigned char *) k;
				*next_k && pos_k<12 LIMIT_STR_CHECK;
				pos_k++, next_k++);

			length += pos_k;
				
			if (pos_k < 12) {
				len = pos_k;
				break;
			}
	
			a += ( k[0] + ( (unsigned)k[1] << 8 ) 
			  + ( (unsigned)k[2] << 16 )
			  + ( (unsigned)k[3] << 24 ) );
			b += ( k[4] + ( (unsigned)k[5] << 8 ) 
			  + ( (unsigned)k[6] << 16 )
			  + ( (unsigned)k[7] << 24 ) );
			c += ( k[8] + ( (unsigned)k[9] << 8 ) 
			  + ( (unsigned)k[10] << 16 )
			  + ( (unsigned)k[11] << 24 ) );

			mix ( a, b, c );

			k += 12;

		}
	}
 
    c += length;
 
    switch ( len ) {
    case 11: c += ( (unsigned)k[10] << 24 );
    case 10: c += ( (unsigned)k[9] << 16 );
    case 9 : c += ( (unsigned)k[8] << 8 );
    /* First byte of c reserved for length */
    case 8 : b += ( (unsigned)k[7] << 24 );
    case 7 : b += ( (unsigned)k[6] << 16 );
    case 6 : b += ( (unsigned)k[5] << 8 );
    case 5 : b += k[4];
    case 4 : a += ( (unsigned)k[3] << 24 );
    case 3 : a += ( (unsigned)k[2] << 16 );
    case 2 : a += ( (unsigned)k[1] << 8 );
    case 1 : a += k[0];
    }
 
    mix ( a, b, c );
 
    return c;
}
