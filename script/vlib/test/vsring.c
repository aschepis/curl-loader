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

#include <util/vsring.h>
#include <stdio.h>
#include "vtest.h"

typedef struct
{
  VSRING base;
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

void VSRING_test()
{
	VSRING list;
	INT_DLIST_ENTRY *tmp;
	VSRING *prev, *pos;
	int i, *arr;


	VSRING_init( &list );

	for(i=9;i>=0;i--) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = (i * 2);
		VSRING_push_front(&list, (VSRING *) tmp);
	}

	VASSERT( VSRING_check( &list ) );

	i = 0;
	VSRING_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 2;
	}
	VASSERT( i == 20 );


    pos = VSRING_get_first( &list );
	for(i=0;i<10;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = (i * 2) + 1;

  	    VSRING_insert_after( pos, (VSRING *) tmp);
		pos = VSRING_get_next( &list, pos );
		pos = VSRING_get_next( &list, pos );
	}

	VASSERT( VSRING_check( &list ) );

	i = 0;
	VSRING_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i++;
	}
	VASSERT( i == 20 );


	i = 0;
	prev=  &list;
	VSRING_FOREACH(pos, &list ) {
		if (i & 1) {
			free( VSRING_unlink_after( prev ) );
			VASSERT( VSRING_check( &list ) );
			pos = prev;
		}
		prev = pos;
		i+= 1;
	}
	VASSERT( i == 20 );
	
	VASSERT( VSRING_check( &list ) );

	i = 0;
	VSRING_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 2;
	}
	VASSERT( i == 20);
	
	VASSERT( VSRING_check( &list ) );

	while(!VSRING_isempty( &list)) {
			free( VSRING_unlink_after( &list));
	}

	VASSERT( VSRING_check( &list ) );
	VASSERT( VSRING_isempty( &list ) );


	arr = shuffle(100);
	for(i=0; i<100; i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = arr[i];

		VSRING_insert_sorted(&list, (VSRING_COMPARE) compare_entry,(VSRING *) tmp);
	}
	free(arr);

	VASSERT( VSRING_check( &list ) );

	i = 0;
	VSRING_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 1;
	}
	VASSERT( i == 100);
		
	while(!VSRING_isempty( &list)) {
			free( VSRING_unlink_after( &list ));
	}

	for(i=0;i<100;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = i;
		VSRING_push_front(&list, (VSRING *) tmp);
	}
	VASSERT( VSRING_check( &list ) );

	i = 99;
	VSRING_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i -= 1;
	}
	VASSERT( i == -1);

	while(!VSRING_isempty( &list)) {
			free( VSRING_unlink_after( &list ));
	}

	for(i=0;i<100;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = i;
		VSRING_push_front(&list, (VSRING *) tmp);
	}

	VASSERT( VSRING_check( &list ) );

	i = 99;
	VSRING_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i -= 1;
	}
	VASSERT( i == -1);

	while(!VSRING_isempty( &list)) {
			free( VSRING_unlink_after( &list ));
	}	
}
