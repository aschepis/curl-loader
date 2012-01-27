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
#include <string.h>
#include <stdio.h>
#include "vtest.h"

typedef struct tagSTR2INT
{
	VBUCKETHASH_Entry entry;
	char name[6];
	int  value;
}
	STR2INT;

static int hash_compare(VBUCKETHASH_Entry *entry, void * key, size_t key_length)
{
	STR2INT *lhs;

	V_UNUSED(key_length);

	lhs = (STR2INT *) entry;

	if (strcmp(lhs->name,key) == 0) {
		return 0;
	}
	return 1;
}

static void hash_visitor (VBUCKETHASH_Entry *entry, void *data)
{
	STR2INT *lhs = (STR2INT *) entry;
	char tmp[6];
	int *tmpn;

	sprintf(tmp,"N:%03d",lhs->value);

	VASSERT( strcmp(tmp,lhs->name) == 0);

	tmpn = (int *) data;
	*tmpn += 1;
	
}



#define FIRST_S "first"
#define SECOND_S "second"
#define THIRD_S "third"

#define TST_NUM 100

extern VCONTEXT * test_alloc;

void VBUCKETHASH_test()
{
	VBUCKETHASH hash;
	STR2INT *tmp;
	int i,count;
	char stmp[6];
	VBUCKETHASH_Entry *entry;
	VCONTEXT *ctx = test_alloc ? test_alloc : VCONTEXT_get_default_ctx();

	VASSERT( !VBUCKETHASH_init_uniquemap( test_alloc, &hash, 10, 0, hash_compare) );
	VASSERT(  VBUCKETHASH_check(&hash) );
	
	for(i=0;i<TST_NUM;i++) {

		tmp = ctx->malloc( ctx, sizeof(STR2INT) );
		sprintf(tmp->name,"N:%03d",i);
		tmp->value = i;

		VASSERT( !VBUCKETHASH_insert( &hash, &tmp->entry, tmp->name, VHASH_STRING ) );
	}

	VASSERT( VBUCKETHASH_check(&hash) );
	VASSERT( VBUCKETHASH_size(&hash) == TST_NUM );

	count = 0;
	VBUCKETHASH_foreach( &hash, hash_visitor, &count);
	VASSERT( count == TST_NUM );

	for(i=0;i<TST_NUM;i++) {
		sprintf(stmp,"N:%03d",i);
		entry = VBUCKETHASH_find(&hash, &stmp, VHASH_STRING );
		VASSERT(entry);
		VASSERT( ((STR2INT *) entry)->value == i );
	}

	VASSERT( VBUCKETHASH_check(&hash) );
	
	VBUCKETHASH_deleteall( &hash, 0, 0, test_alloc, 0);
	VBUCKETHASH_free( &hash );

}

