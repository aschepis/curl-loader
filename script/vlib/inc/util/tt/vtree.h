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

#ifndef _VTREE_H_
#define _VTREE_H_

#ifdef  __cplusplus
extern "C" {
#endif


#include <util/vbasedefs.h>


#define VTREE_NEXTLEVEL_LAST 
#define VTREE_NEXTLEVEL_LEFT

/**
 * tree. Each node has variable number of child nodes, the node is embeddable (like ring list).
 * This tree is especially suited for parse trees.
 */
typedef struct tagVTREENODE {
	struct tagVTREENODE *parent;
	
	struct tagVTREENODE *nextlevel_first;
#ifdef VTREE_NEXTLEVEL_LAST
	struct tagVTREENODE *nextlevel_last;
#endif
	struct tagVTREENODE *right;
#ifdef VTREE_NEXTLEVEL_LEFT	
	struct tagVTREENODE *left;
#endif
} VTREENODE;

V_INLINE void VTREE_init_root(VTREENODE *tree)
{
	tree->right = tree->parent = tree->nextlevel_first = 0;
#ifdef VTREE_NEXTLEVEL_LEFT
	tree->left = 0;
#endif
#ifdef VTREE_NEXTLEVEL_LAST
	tree->nextlevel_last = 0;
#endif
}

V_INLINE VTREENODE *VTREE_parent(VTREENODE *node)
{
	return node->parent;
}


V_INLINE VTREENODE *VTREE_left_sibling(VTREENODE *node)
{
#ifdef VTREE_NEXTLEVEL_LEFT
	return node->left;
#else
	VTREENODE *tmp;

	if (!node->parent) {
		return 0;
	}
	tmp = node->parent->nextlevel_first;
	if (tmp == node) {
		return 0;
	}
	while(tmp && tmp->right != node) {
		tmp = tmp->right;
	}
	return tmp;
#endif
}

V_INLINE VTREENODE *VTREE_right_sibling(VTREENODE *node)
{
	return node->right;
}

V_INLINE VTREENODE *VTREE_leftmost_sibling(VTREENODE *node)
{
 	VTREENODE *parent = node->parent;
	if (parent) {
		return parent->nextlevel_first;
	}
	return 0;
}

V_INLINE VTREENODE *VTREE_rightmost_sibling(VTREENODE *node)
{
#ifdef VTREE_NEXTLEVEL_LAST
	VTREENODE *parent = node->parent;
	if (parent) {
		return parent->nextlevel_last;
	}
	return 0;
#else
	while(node->right) {
		node = node->right;
	}
#endif
	return node;
}

V_INLINE VTREENODE *VTREE_first_child(VTREENODE *node)
{
	return node->nextlevel_first;
}

V_INLINE VTREENODE *VTREE_last_child(VTREENODE *node)
{
#ifdef VTREE_NEXTLEVEL_LAST
	return node->nextlevel_last;
#else
	return VTREE_rightmost_sibling(node->nextlevel_first);
#endif
}



/* insert function -> new element next level is initialised hereby to zero ? option? new function? */

V_INLINE int VTREE_insert_right_sibling(VTREENODE *current, VTREENODE *newnode, int node_is_leaf)
{
	VTREENODE *parent = current->parent;

	if (!parent) {
		return -1;
	}
	if (node_is_leaf) {
		VTREE_init_root(newnode);
	}

#ifdef VTREE_NEXTLEVEL_LAST
	if (parent->nextlevel_last == current) {
		parent->nextlevel_last = newnode;
	}
#endif

	newnode->parent = parent;
	
#ifdef VTREE_NEXTLEVEL_LEFT
	newnode->left  = current;
	if (current->right) {
		current->right->left = newnode;
	}
#endif
	newnode->right = current->right;
	current->right = newnode;



	return 0;
}


V_INLINE int VTREE_insert_left_sibling(VTREENODE *current, VTREENODE *newnode, int node_is_leaf)
{
	VTREENODE *parent = current->parent;
	if (!parent) {
		return -1;
	}

	if (node_is_leaf) {
		VTREE_init_root(newnode);
	}

	if (parent->nextlevel_first == current) {
		parent->nextlevel_first = newnode;
	}

	newnode->parent = parent;

#ifdef VTREE_NEXTLEVEL_LEFT
	if (current->left) {
		current->left->right = newnode;
	}
#endif
	newnode->right = current;
#ifdef VTREE_NEXTLEVEL_LEFT	
	newnode->left = current->left;
	current->left = newnode;
#endif



	return 0;
}

typedef enum {
  VTREE_INSERT_FIRST,
  VTREE_INSERT_LAST,

} VTREE_INSERT_MODE;

V_INLINE void VTREE_insert_child(VTREENODE *parent, VTREENODE *newnode, VTREE_INSERT_MODE mode, int node_is_leaf )
{	
	/* init new node */
	if (node_is_leaf) {
		newnode->nextlevel_first = 0;
#ifdef VTREE_NEXTLEVEL_LAST
		newnode->nextlevel_last = 0;
#endif
	}
	newnode->parent = parent;
	newnode->right = 0;
#ifdef VTREE_NEXTLEVEL_LEFT
	newnode->left = 0;
#endif	

	
	if (!parent->nextlevel_first) {		

		/* insert as first child */
		newnode->nextlevel_first = parent->nextlevel_first;
		parent->nextlevel_first = newnode;
#ifdef VTREE_NEXTLEVEL_LAST
		newnode->nextlevel_last = parent->nextlevel_last;
		parent->nextlevel_last = newnode;
#endif
		
	} else {

		/* insert as sibling */
		switch(mode) {
			case VTREE_INSERT_FIRST: {
					VTREE_insert_left_sibling(parent->nextlevel_first,newnode, 0);
				}
				break;
			case VTREE_INSERT_LAST:
#ifdef VTREE_NEXTLEVEL_LAST
				VTREE_insert_right_sibling( parent->nextlevel_last,newnode, 0);
#else
				VTREE_insert_right_sibling( VTREE_rightmost_sibling(parent->nextlevel_first), newnode, 0);
#endif

				break;
		}
	}

}


V_INLINE VTREENODE * VTREE_unlink_node(VTREENODE *node)
{
	VTREENODE *tmp,*tmp_next, *tmp_last;

	if (!node->parent) {
		return 0;
	}

	/* wrong */
	tmp = node->parent;
	if (tmp && tmp->nextlevel_first == node) {

		if (!node->nextlevel_first) {
			tmp->nextlevel_first = node->right;
			goto sdel;
		}

		tmp->nextlevel_first = node->nextlevel_first;				
		tmp_next = tmp->nextlevel_first;
		tmp_last = VTREE_rightmost_sibling(tmp_next);

		tmp_last->right = node->right;
#ifdef VTREE_NEXTLEVEL_LEFT
		if (node->right) {
			node->right->left = tmp_last;
		}
#endif
		do {
			tmp_next->parent = tmp;
			if (tmp_next == tmp_last) {
				break;
			}
			tmp_next = tmp_next->right;
		} while(tmp_next);

		return node;
	}

#ifdef VTREE_NEXTLEVEL_LAST
	if (tmp && tmp->nextlevel_last == node) {

		if (!node->nextlevel_last) {
			tmp->nextlevel_last = VTREE_left_sibling(node);
			goto sdel;
		}

		tmp->nextlevel_last = node->nextlevel_last;	
		tmp_next = tmp->nextlevel_last;
		tmp_last = VTREE_leftmost_sibling(tmp_next);

#ifdef VTREE_NEXTLEVEL_LEFT
		tmp_last->left = node->left;
#endif
		VTREE_left_sibling(node)->right = tmp_last;

		do {
			tmp_last->parent = tmp;
			if (tmp_next == tmp_last) {
				break;
			}
			tmp_last = tmp_last->right;
		} while(tmp_last);					

		return node;
	}
#endif

sdel:

#ifdef VTREE_NEXTLEVEL_LEFT
	tmp = node->right;
	if (tmp) {
		tmp->left = node->left;
	}
#endif

	tmp = VTREE_left_sibling(node);
	if (tmp) {
		tmp->right = node->right;
	}

	return node;
}


typedef int  (*VTREE_VISITOR) (VTREENODE *entry, void *context);
typedef void  (*VTREE_VISITOR_V) (VTREENODE *entry, void *context);

/*
 a
 |
 b-d-g
 | |
 c e-f
*/
VTREENODE *VTREE_preorder_next(VTREENODE *current)
{
	if (current->nextlevel_first) {
		current = current->nextlevel_first;
	} else {
		if (current->right) {
			current = current->right;
		} else {
			while(current->parent) {
				current = current->parent;
				if (current->right) {
					current = current->right;
					break;
				}
			}
		}
	}

	return current;
}


#define VTREE_FOREACH_PREORDER(current, tree)\
{\
	VTREENODE *vtree_nextnode##next;\
	current = VTREE_preorder_next((tree));\
	if (current)\
		for( vtree_nextnode##next =  VTREE_preorder_next(current);\
			    (current) && current != (tree); \
				(current) = vtree_nextnode##next, vtree_nextnode##next =  VTREE_preorder_next(current)) {

		
#define	VTREE_FOREACH_PREORDER_END\
	}\
}




/*
 f
 |
 b-d-e
 | |
 a b-c
*/
VTREENODE *VTREE_postorder_next(VTREENODE *current, VTREENODE *prev)
{

	if (current->nextlevel_first && (!prev || current->parent == prev)) {
next_level:
		do {		
			current = current->nextlevel_first;
		}
		while(current->nextlevel_first);
	} else {
		if (current->right) {
			current = current->right;
			if (current->nextlevel_first) {
			  goto next_level;
			}
		} else {
			if (current->parent) {
				current = current->parent;
			}
		}
	}
	return current;
}


/* wrong loop condition */
#define VTREE_FOREACH_POSTORDER(current, tree)\
{\
	VTREENODE *vtree_nextnode##next,*vtree_nextnode##prev;\
	vtree_nextnode##prev = 0; \
	current = VTREE_postorder_next((tree),vtree_nextnode##prev);\
	if (current)\
		for( vtree_nextnode##next =  VTREE_postorder_next(current,vtree_nextnode##prev);\
			    (current) && current != (tree); \
				vtree_nextnode##prev = (current), \
				(current) = vtree_nextnode##next, \
				vtree_nextnode##next =  VTREE_postorder_next(current,vtree_nextnode##prev)) {




#define VTREE_FOREACH_POSTORDER_END\
	}\
}\

/*
 
 preorder(tree)
 begin
     if tree is null, return;

     print(tree.root);
     preorder(tree.left_subtree);
     preorder(tree.right_subtree);
 end 
 */
void VTREE_foreach_preorder(VTREENODE *node, VTREE_VISITOR_V visit, void *context)
{
	VTREENODE *current;

	VTREE_FOREACH_PREORDER(current,node)
		visit(node,context);
	VTREE_FOREACH_POSTORDER_END
}

VTREENODE * VTREE_find_preorder(VTREENODE *node, VTREE_VISITOR visit, void *context)
{
	VTREENODE *current;

	VTREE_FOREACH_PREORDER(current,node)
		if (visit(node,context)) {
			return current;
		}
	VTREE_FOREACH_POSTORDER_END
	return 0;
}



/*

 postorder(tree)
 begin
     if tree is null, return;

     postorder(tree.left_subtree);
     postorder(tree.right_subtree);
     print(tree.root);
 end 
 */
void VTREE_foreach_postorder(VTREENODE *node, VTREE_VISITOR_V visit, void *context)
{
	VTREENODE *current;

	VTREE_FOREACH_POSTORDER(current,node)
		visit(node,context);
	VTREE_FOREACH_POSTORDER_END
}

VTREENODE * VTREE_find_postorder(VTREENODE *node, VTREE_VISITOR visit,void *context)
{
	VTREENODE *current;

	VTREE_FOREACH_POSTORDER(current,node)
		if (visit(node,context)) {
			return current;
		}
	VTREE_FOREACH_POSTORDER_END
	return 0;
}

/**
 * @brief check tree for consistency errors
 * @return 0 if the tree is inconsistent
 */
int VTREE_check_tree(VTREENODE *root)
{
	VTREENODE *firstch = root->nextlevel_first;
#ifdef VTREE_NEXTLEVEL_LAST
	VTREENODE *lastch   = root->nextlevel_last;
#endif
#ifndef VTREE_NEXTLEVEL_LEFT	
	VTREENODE *slow,*fast, *next;
#endif

	if (!firstch 
#ifdef VTREE_NEXTLEVEL_LAST
			|| !lastch
#endif
		) {

#ifdef VTREE_NEXTLEVEL_LAST
		if (firstch || lastch) {
			return 0;
		}
#endif
		return 1;		
	}	
	
	if (firstch->parent != root) {
		return 0;
	}

#ifdef VTREE_NEXTLEVEL_LEFT	
	for(;;firstch = firstch->right) {
		VTREENODE *next;
		
		if (!firstch) {
			return 0;
		}

		if (!VTREE_check_tree(firstch)) {
			return 0;
		}

		next = firstch->right;
		if (!next) {
#ifdef VTREE_NEXTLEVEL_LAST
			if (lastch != firstch) {
				return 0;
			}
#endif
			break;
		}

		if (next->left != firstch) {
			return 0;
		}
	}
#else
	slow=fast=firstch;
	while(1) {
		if (!VTREE_check_tree(slow)) {
			return 0;
		}
		next = fast->right;
		if (!next) {
			break;			
		}
		fast = next;
		if (fast == slow) {
			return 0;
		}
		next = fast->right;
		if (!next) {
			break;			
		}
		fast = next;
		if (fast == slow) {
			return 0;
		}
		slow = slow->right;
	}
#ifdef VTREE_NEXTLEVEL_LAST
	if (lastch != fast) {
		return 0;
	}
#endif

#endif
	return 1;
}

#ifdef  __cplusplus
}
#endif

#endif

