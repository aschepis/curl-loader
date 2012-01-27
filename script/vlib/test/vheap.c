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

#include <util/vheap.h>
#include <stdio.h>
#include <string.h>
#include "vtest.h"

extern VCONTEXT * test_alloc;

typedef struct HEntry {
	int Key;
	char Name[6];
} HEntry;

int HEntry_compare(void *lh, void *rh, size_t entry)
{
	HEntry *le = (HEntry *) lh;
	HEntry *re = (HEntry *) rh;

	V_UNUSED(entry);

	if (le->Key < re->Key) {
		return -1;
	}
	if (le->Key > re->Key) {
		return 1;
	}
	return 0;
}

void check_heap(void *data, size_t elsize, void *context)
{
	HEntry *pentry = (HEntry *) data;
	int *num = (int *) context;

	V_UNUSED(elsize);

	VASSERT( *num < pentry->Key);
	*num = pentry->Key;
}

static void *shuffle(int n)
{
	int *arr,i,tmp,pos;
	
	arr = (int *) malloc(n * sizeof(int));
	for(i = 0; i < n; i++) {
		arr[i] = i;
	}

	for(i = 0; i < n; i++) {

		pos = (rand() % (n - 1)) + 1;

		VASSERT_RET(pos > 0 && pos < n, 0);
		tmp = arr[0];
		arr[0] = arr[pos];
		arr[pos] = tmp;

	}
	return arr;
}

#ifdef SHOW_RESULTS
void printheap(VHEAP *heap)
{

	size_t i;

	for(i=0;i<heap->elmcount;i++) {
		HEntry *entry = (HEntry *) (heap->buffer + i * heap->elmsize);
		printf(" %03d ", entry->Key);
	}
	printf("\n");
}
#endif


#define TEST_SIZE 200

void VHEAP_test()
{
	VHEAP heap;
	HEntry entry,*pentry;
	int *tmp,i;


	VASSERT( !VHEAP_init( test_alloc, &heap, 10, sizeof(HEntry),  HEntry_compare) );

	tmp = shuffle(TEST_SIZE);
#ifdef SHOW_RESULTS
	printheap(&heap);
#endif

	for(i=0;i<TEST_SIZE;i++) {
			entry.Key = tmp[i];
			sprintf(entry.Name,"N:%03d",tmp[i]);

			VHEAP_push(  &heap, (unsigned char *) &entry, sizeof(entry) );

#ifdef SHOW_RESULTS
			printheap(&heap);
#endif
			
			VASSERT(  VHEAP_check(&heap) );

	}
	free(tmp);

#ifdef SHOW_RESULTS
	printf("pop!\n");
#endif


	VASSERT( VHEAP_check(&heap) );
	VASSERT( VHEAP_size(&heap) == TEST_SIZE );

	i = -1;
	while( (pentry = (HEntry *) VHEAP_top( &heap )) != 0 ) {

#ifdef SHOW_RESULTS
		printheap(&heap);
		printf("->%d\n",pentry->Key);
#endif
		VASSERT(pentry->Key > i);


		sprintf(entry.Name,"N:%03d",pentry->Key);
		VASSERT( pentry->Key >= i);
		VASSERT( strcmp(pentry->Name,entry.Name) == 0);
		i = pentry->Key;
		
		VHEAP_pop(  &heap);
	}

	VASSERT(VHEAP_check(&heap));
	VHEAP_free(&heap);
}

