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

#include <util/vtree.h>
#include "vtest.h"

typedef struct {
	VTREENODE node;
	int value;
} VTREENODE_INT;


static int preorder_1[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
static int postorder_1[] = { 2, 3, 4, 1, 7, 6, 5, 8, 10, 9 };

static int preorder_2[] = { 1, 2, 3, 4, 8, 9, 10 };
static int postorder_2[] = { 2, 3, 4, 1, 8, 10, 9 };


static int preorder_3[] = { 1, 3,  8, 9, 10 };
static int postorder_3[] = {  3, 1, 8, 10, 9 };

/*
 *	TODO:
	 - more thorough test of unlink
	 - test iteration over root (zero case)
	 - castings to iteration type in FOREACH macros (global thing!).
 */

void VTREE_test()
{
/* test tree
	r
	|
	1-5-8-9
	| |   |  
	| 6   10
	| |
	| 7
	|
	2-3-4

*/
	VCONTEXT ctx;
	VTREENODE_INT root,*tmp1,*tmp2,*tmp3,*tmp4,  *tmp_next, *tmp_nexta;
	VTREENODE *cur;
	int n;

	VCONTEXT_init_null(&ctx);
	
	VTREE_init_root(&root.node);

	VTREE_FOREACH_PREORDER(cur,&root.node)
		VASSERT( 0 );
	VTREE_FOREACH_PREORDER_END

	VTREE_FOREACH_POSTORDER(cur,&root.node)
		VASSERT( 0 );
	VTREE_FOREACH_POSTORDER_END

    VASSERT( VTREE_check_tree(&root.node) );
   
    /*first level */
	tmp1 = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp1->value = 5;
	VTREE_insert_child(&root.node, (VTREENODE *) tmp1, VTREE_INSERT_FIRST, V_TRUE);

	tmp2 = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp2->value = 8;
	VTREE_insert_child(&root.node, (VTREENODE *) tmp2, VTREE_INSERT_LAST, V_TRUE);

	tmp3 = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp3->value = 1;
	VTREE_insert_child(&root.node, (VTREENODE *) tmp3, VTREE_INSERT_FIRST, V_TRUE);

	tmp4 = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp4->value = 9;
	VTREE_insert_child(&root.node, (VTREENODE *) tmp4, VTREE_INSERT_LAST, V_TRUE);

	/*level 2 from tmp1*/
	tmp_next = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_next->value = 3;
	VTREE_insert_child((VTREENODE *) tmp3, (VTREENODE *) tmp_next, VTREE_INSERT_LAST, V_TRUE);

	tmp_nexta = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_nexta->value = 4;
	VTREE_insert_right_sibling((VTREENODE *) tmp_next, (VTREENODE *) tmp_nexta, V_TRUE);

	tmp_nexta = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_nexta->value = 2;
	VTREE_insert_left_sibling((VTREENODE *) tmp_next, (VTREENODE *) tmp_nexta, V_TRUE);

	/*level 2 from tmp2 */
	tmp_next = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_next->value = 6;
	VTREE_insert_child((VTREENODE *) tmp1, (VTREENODE *) tmp_next, VTREE_INSERT_FIRST, V_TRUE);

	tmp_nexta = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_nexta->value = 7;
	VTREE_insert_child((VTREENODE *)tmp_next, (VTREENODE *) tmp_nexta, VTREE_INSERT_LAST, V_TRUE);

	/* level 2 from tmp4 */
	tmp_next = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_next->value = 10;
	VTREE_insert_child((VTREENODE *) tmp4, (VTREENODE *) tmp_next, VTREE_INSERT_FIRST, V_TRUE);

    VASSERT( VTREE_check_tree(&root.node) );

	/* test iteration */
	n = 0;
	VTREE_FOREACH_PREORDER(cur,&root.node)

		VASSERT( n < 10 && ((VTREENODE_INT *)cur)->value == preorder_1[ n++ ] );

	VTREE_FOREACH_PREORDER_END
	VASSERT( n == 10 );

	/* test iteration */
	n = 0;
	VTREE_FOREACH_POSTORDER(cur, &root.node)

		VASSERT( n < 10 && ((VTREENODE_INT *)cur)->value == postorder_1[ n++ ] );

	VTREE_FOREACH_POSTORDER_END
	VASSERT( n == 10 );

	/* delete some nodes */


	VTREE_unlink_node((VTREENODE *) tmp1);

    VASSERT( VTREE_check_tree(&root.node) );

/* test tree
	r
	|
	1-8-9
	|   |  
	|   10
	| 
	|
	2-3-4

*/

	n = 0;
	VTREE_FOREACH_PREORDER(cur,&root.node)

		VASSERT( n < 10 && ((VTREENODE_INT *)cur)->value == preorder_2[ n++ ] );

	VTREE_FOREACH_PREORDER_END
	VASSERT( n == 7 );

	/* test iteration */
	n = 0;
	VTREE_FOREACH_POSTORDER(cur,&root.node)

		VASSERT( n < 10 && ((VTREENODE_INT *)cur)->value == postorder_2[ n ++ ] );

	VTREE_FOREACH_POSTORDER_END
	VASSERT( n == 7 );


	/* delete some nodes */
	VTREE_unlink_node( (VTREENODE *) VTREE_rightmost_sibling( (VTREENODE *) VTREE_first_child( (VTREENODE *) tmp3) ) );
	VTREE_unlink_node( (VTREENODE *) VTREE_first_child( (VTREENODE *) tmp3) );

    VASSERT( VTREE_check_tree(&root.node) );

/* test tree
	r
	|
	1-8-9
	|   |  
	|   10
	| 
	|
	3

*/

	n = 0;
	VTREE_FOREACH_PREORDER(cur,&root.node)

		VASSERT( n < 10 && ((VTREENODE_INT *)cur)->value == preorder_3[ n++ ] );

	VTREE_FOREACH_PREORDER_END
	VASSERT( n == 5 );

	/* test iteration */
	n = 0;
	VTREE_FOREACH_POSTORDER(cur,&root.node)

		VASSERT( n < 10 && ((VTREENODE_INT *)cur)->value == postorder_3[ n++ ] );

	VTREE_FOREACH_POSTORDER_END
	VASSERT( n == 5 );

}

static int preorder_u1[] = { 1, 2, 5, 3, 4, 8, 9, 10, 11, 12, 13, 14 };
static int postorder_u1[] = { 5, 2, 3, 4, 1, 8, 10, 12, 13, 14, 11, 9   };

static int preorder_u2[] = { 2, 5, 3, 4, 8, 9, 10, 12, 13, 14 };
static int postorder_u2[] = { 5, 2, 3, 4, 8, 10, 12, 13, 14, 9   };


void VTREE_test_unlink()
{
/* test tree
	r
	|
	1-8-9
	|   |  
	|   10--11
	|	     |
	|	     12-13-14
	| 
	|
	2-3-4
	|
	5

*/

	VCONTEXT ctx;
	VTREENODE_INT root,*tmp2,*tmp3,*tmp4,  *tmp_next, *tmp_nexta, *tmp_next2;
	VTREENODE *cur;
	int n;

	VCONTEXT_init_null(&ctx);
	
	VTREE_init_root(&root.node);

    VASSERT( VTREE_check_tree(&root.node) );
   
    /*first level */

	tmp2 = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp2->value = 8;
	VTREE_insert_child(&root.node, (VTREENODE *) tmp2, VTREE_INSERT_LAST, V_TRUE);

	tmp3 = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp3->value = 1;
	VTREE_insert_child(&root.node, (VTREENODE *) tmp3, VTREE_INSERT_FIRST, V_TRUE);

	tmp4 = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp4->value = 9;
	VTREE_insert_child(&root.node, (VTREENODE *) tmp4, VTREE_INSERT_LAST, V_TRUE);

	/*level 2 from tmp1*/
	tmp_next = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_next->value = 3;
	VTREE_insert_child((VTREENODE *) tmp3, (VTREENODE *) tmp_next, VTREE_INSERT_LAST, V_TRUE);


	tmp_nexta = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_nexta->value = 4;
	VTREE_insert_right_sibling((VTREENODE *) tmp_next, (VTREENODE *) tmp_nexta, V_TRUE);

	tmp_nexta = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_nexta->value = 2;
	VTREE_insert_left_sibling((VTREENODE *) tmp_next, (VTREENODE *) tmp_nexta, V_TRUE);

		tmp_next2 = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
		tmp_next2->value = 5;
		VTREE_insert_child((VTREENODE *) tmp_nexta, (VTREENODE *) tmp_next2, VTREE_INSERT_LAST, V_TRUE);

	/* level 2 from tmp4 */
	tmp_next = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_next->value = 10;
	VTREE_insert_child((VTREENODE *) tmp4, (VTREENODE *) tmp_next, VTREE_INSERT_LAST, V_TRUE);

	tmp_next = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_next->value = 11;
	VTREE_insert_child((VTREENODE *) tmp4, (VTREENODE *) tmp_next, VTREE_INSERT_LAST, V_TRUE);

	/* level 3 from tmp_next */
	tmp_nexta = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_nexta->value = 12;
	VTREE_insert_child((VTREENODE *) tmp_next, (VTREENODE *) tmp_nexta, VTREE_INSERT_LAST, V_TRUE);

	tmp_nexta = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_nexta->value = 13;
	VTREE_insert_child((VTREENODE *) tmp_next, (VTREENODE *) tmp_nexta, VTREE_INSERT_LAST, V_TRUE);

	tmp_nexta = ctx.malloc(&ctx, sizeof(VTREENODE_INT));
	tmp_nexta->value = 14;
	VTREE_insert_child((VTREENODE *) tmp_next, (VTREENODE *) tmp_nexta, VTREE_INSERT_LAST, V_TRUE);

    VASSERT( VTREE_check_tree(&root.node) );


	/* test iteration */
	n = 0;
	VTREE_FOREACH_PREORDER(cur,&root.node)

		VASSERT( n < 12 && ((VTREENODE_INT *)cur)->value == preorder_u1[ n++ ] );

	VTREE_FOREACH_PREORDER_END
	VASSERT( n == 12 );

	/* test iteration */
	n = 0;
	VTREE_FOREACH_POSTORDER(cur, &root.node)

		VASSERT( n < 12 && ((VTREENODE_INT *)cur)->value == postorder_u1[ n++ ] );

	VTREE_FOREACH_POSTORDER_END
	VASSERT( n == 12 );

	VTREE_unlink_node( (VTREENODE *) tmp3 );
	VTREE_unlink_node( (VTREENODE *) tmp_next );

    VASSERT( VTREE_check_tree(&root.node) );


/* test tree
	r
	|
	2-3-4-8-9
	|	    |
	5		10-12-13-14

*/

	n = 0;
	VTREE_FOREACH_PREORDER(cur,&root.node)

		VASSERT( n < 10 && ((VTREENODE_INT *)cur)->value == preorder_u2[ n++ ] );

	VTREE_FOREACH_PREORDER_END
	VASSERT( n == 10 );

	/* test iteration */
	n = 0;
	VTREE_FOREACH_POSTORDER(cur, &root.node)

		VASSERT( n < 10 && ((VTREENODE_INT *)cur)->value == postorder_u2[ n++ ] );

	VTREE_FOREACH_POSTORDER_END
	VASSERT( n == 10 );

}
