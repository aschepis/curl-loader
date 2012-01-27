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


#include <util/vdlist.h>
#include <stdio.h>
#include "vtest.h"

typedef struct
{
  VDLIST_entry base;
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

void VDLIST_test()
{
	VDLIST list;
	INT_DLIST_ENTRY *tmp;
	VDLIST_entry *pos, *next;
	int i, *arr;


	VDLIST_init( &list );

	for(i=0;i<10;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = (i * 2);
		VDLIST_push_back(&list, (VDLIST_entry *) tmp);
	}

	VASSERT( VDLIST_check( &list ) );

	i = 0;
	VDLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 2;
	}
	VASSERT( i == 20 );

	i = 18;
	VDLIST_FOREACH_REVERSE( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i -= 2;
	}
	VASSERT( i == -2 );

    pos = VDLIST_get_first( &list );
	for(i=0;i<10;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = (i * 2) + 1;

  	    VDLIST_insert_after(&list, pos, (VDLIST_entry *) tmp);
		pos = VDLIST_get_next( pos );
		pos = VDLIST_get_next( pos );
	}

	VASSERT( VDLIST_check( &list ) );

	i = 0;
	VDLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i++;
	}
	VASSERT( i == 20 );

	i = 19;
	VDLIST_FOREACH_REVERSE( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i--;
	}
	VASSERT( i == -1 );

	i = 0;
	VDLIST_FOREACH_SAVE( pos, next, &list ) {
		if (i & 1) {
			free( VDLIST_unlink( &list, pos ) );
		}
		i+= 1;
	}
	VASSERT( i == 20 );
	VASSERT( list.elmcount == 10)

	VASSERT( VDLIST_check( &list ) );

	i = 0;
	VDLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 2;
	}
	VASSERT( i == 20);

	VDLIST_FOREACH_SAVE( pos, next,  &list ) {
			free( VDLIST_unlink( &list, pos ));
	}

	VASSERT( VDLIST_check( &list ) );
	VASSERT( VDLIST_isempty( &list ) );


	arr = shuffle(100);
	for(i=0; i<100; i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = arr[i];

		VDLIST_insert_sorted(&list, (VDLIST_COMPARE) compare_entry,(VDLIST_entry *) tmp);
	}
	free(arr);

	VASSERT( VDLIST_check( &list ) );

	i = 0;
	VDLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 1;
	}
	VASSERT( i == 100);
		
	VDLIST_FOREACH_SAVE( pos, next,  &list ) {
			free( VDLIST_unlink( &list, pos ));
	}

	for(i=0;i<100;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = i;
		VDLIST_push_back(&list, (VDLIST_entry *) tmp);
	}
	VASSERT( VDLIST_check( &list ) );

	i = 0;
	VDLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i += 1;
	}
	VASSERT( i == 100);

	VDLIST_FOREACH_SAVE( pos, next,  &list ) {
			free( VDLIST_unlink( &list, pos ));
	}

	for(i=0;i<100;i++) {
		tmp = (INT_DLIST_ENTRY *) malloc(sizeof(INT_DLIST_ENTRY));
		tmp->num = i;
		VDLIST_push_front(&list, (VDLIST_entry *) tmp);
	}

	VASSERT( VDLIST_check( &list ) );

	i = 99;
	VDLIST_FOREACH( pos, &list ) {

		VASSERT( ((INT_DLIST_ENTRY *) pos)->num == i );
		i -= 1;
	}
	VASSERT( i == -1);

	VDLIST_FOREACH_SAVE( pos, next,  &list ) {
			free( VDLIST_unlink( &list, pos ));
	}

		
}
