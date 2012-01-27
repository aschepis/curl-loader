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
#include <string.h>
#include <stdio.h>
#include "vtest.h"

extern VCONTEXT * test_alloc;


static int hash_compare(void *key1, void * key, size_t key_length)
{
	V_UNUSED(key_length);
	
	if (strcmp(key1,key) == 0) {
		return 0;
	}
	return 1;
}

static void hash_visitor (VCLOSEDHASH *hash, void *key, size_t key_size, void *value, size_t value_size, void *context)
{

	char tmp[6];
	int *tmpn;
	char *skey = (char *) key;
    int  nvalue = * ((int *) value);

	V_UNUSED(hash);
	V_UNUSED(key_size);
	V_UNUSED(value_size);
	


	sprintf(tmp,"N:%03d",nvalue);

	VASSERT( strcmp(tmp,skey) == 0);

	tmpn = (int *) context;
	*tmpn += 1;
	
}



#define FIRST_S "first"
#define SECOND_S "second"
#define THIRD_S "third"

#define TST_NUM 100



void VCLOSEDHASH_test()
{
	VCLOSEDHASH hash;
	int i,count;
	char stmp[6];
	char name[6];
	int  value;


	VASSERT( !VCLOSEDHASH_init_uniqemap( test_alloc, &hash, 
			sizeof(name), sizeof(value), 10,
			0, hash_compare, 0 ) );

	VASSERT(  VCLOSEDHASH_check(&hash) );
	
	for(i=0;i<TST_NUM;i++) {
		
		sprintf(name,"N:%03d",i);
		value = i;

		VASSERT( !VCLOSEDHASH_insert( &hash, &name, sizeof(name), &value, sizeof(value) ) );
	}

	VASSERT( VCLOSEDHASH_check(&hash) );
	VASSERT( VCLOSEDHASH_size(&hash) == TST_NUM );

	count = 0;
	VCLOSEDHASH_foreach( &hash, hash_visitor, &count);
	VASSERT( count == TST_NUM );

	for(i=0;i<TST_NUM;i++) {
		int *pval;

		sprintf(stmp,"N:%03d",i);
		VASSERT( (pval = VCLOSEDHASH_find(&hash, &stmp, sizeof(stmp), 0 )) != 0 );

		VASSERT( *pval == i );
	}

	VASSERT( VCLOSEDHASH_check(&hash) );
	
	VCLOSEDHASH_deleteall( &hash, 0, 0 );

	VASSERT( VCLOSEDHASH_check(&hash) );

	VCLOSEDHASH_free( &hash );

}

