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


#include <util/vslist.h>
#include <stdio.h>
#include "vtest.h"

typedef struct
{
  VSLIST_entry base;
  int num;
}
  INT_DLIST_ENTRY;

static int compare_entry(INT_DLIST_ENTRY *a1, INT_DLIST_ENTRY *a2)
{
	
	if (a1->num < a2->num) {
		return -1;
	}

	if (a1->num > a2->num) {
		return 1;
	}
	return 0;
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
		
		tmp = arr[0];
		arr[0] = arr[pos];
		arr[pos] = tmp;

	}
	return arr;

}

void VSLIST_test()
{
	VSLIST list;
	INT_DLIST_ENTRY *tmp;
	VSLIST_entry *prev, *pos;
	int i, *arr;

	VSLIST_init( &list );

	for(i=0;i<10;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = (i * 2);
		VSLIST_push_back(&list, (VSLIST_entry *) tmp);
	}

	VASSERT( VSLIST_check( &list ) );

	i = 0;
	VSLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 2;
	}
	VASSERT( i == 20 );

    pos = VSLIST_get_first( &list );
	for(i=0;i<10;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = (i * 2) + 1;

  	    VSLIST_insert_after(&list, pos, (VSLIST_entry *) tmp);
		pos = VSLIST_get_next( pos );
		pos = VSLIST_get_next( pos );
	}

	VASSERT( VSLIST_check( &list ) );

	i = 0;
	VSLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i++;
	}
	VASSERT( i == 20 );
	VASSERT( list.elmcount == 20)

	i = 0;
	prev = 0;
	VSLIST_FOREACH(  pos, &list ) {
		if (i) {

			free( VSLIST_unlink_after( &list, prev ) );
			prev = pos = prev->next;
			if (!pos) {
				break;
			}

		} else {
			prev = pos;
		}
		
		i+= 1;
	}

	VASSERT( i == 10 );
	VASSERT( list.elmcount == 10)

	VASSERT( VSLIST_check( &list ) );

	i = 0;
	VSLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 2;
	}
	VASSERT( i == 20);

	while(!VSLIST_isempty( &list)) {
			free( VSLIST_unlink_after( &list, 0));
	}

	VASSERT( VSLIST_check( &list ) );
	VASSERT( VSLIST_isempty( &list ) );


	arr = shuffle(100);
	for(i=0; i<100; i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = arr[i];

		VSLIST_insert_sorted(&list, (VSLIST_COMPARE) compare_entry,(VSLIST_entry *) tmp);
	}
	free(arr);

	VASSERT( VSLIST_check( &list ) );

	i = 0;
	VSLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 1;
	}
	VASSERT( i == 100);
		
	while(!VSLIST_isempty( &list)) {
			free( VSLIST_unlink_after( &list, 0));
	}

	for(i=0;i<100;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = i;
		VSLIST_push_back(&list, (VSLIST_entry *) tmp);
	}
	VASSERT( VSLIST_check( &list ) );

	i = 0;
	VSLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 1;
	}
	VASSERT( i == 100);

	while(!VSLIST_isempty( &list)) {
			free( VSLIST_unlink_after( &list, 0));
	}

	for(i=0;i<100;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = i;
		VSLIST_push_front(&list, (VSLIST_entry *) tmp);
	}

	VASSERT( VSLIST_check( &list ) );

	i = 99;
	VSLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i -= 1;
	}
	VASSERT( i == -1);

	while(!VSLIST_isempty( &list)) {
			free( VSLIST_unlink_after( &list, 0));
	}

	VASSERT( VSLIST_check( &list ) );
		
}
