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
#include <util/vsparsearray.h>
#include <util/vutil.h>
#include <assert.h>

V_EXPORT int VSPARSEARRAY_init(VCONTEXT *ctx, VSPARSEARRAY *arr, size_t elmsize, size_t elmmaxcount, size_t elmpernode)
{
	size_t i;

	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

#ifndef VSPARSEARRAY_NOT_POWER_OF_TWO_SIZED_NODES
	elmpernode = VUTIL_round_to_power_of_n( elmpernode );
	if (elmpernode >= elmmaxcount) {
		elmpernode >>= 1;
	}
#endif

	arr->ctx = ctx;
	arr->sizepolicy = VRESIZE_init_multiply(2);

	arr->elmsize = elmsize;
	arr->elmcount = 0;

#if 0
	arr->maxindex = (size_t) -1; 
	arr->minindex = (size_t) -1;
#endif

	arr->elmmaxcount = elmmaxcount;
	arr->node_count = elmmaxcount / elmpernode + (elmmaxcount % elmpernode ? 1 : 0);
	arr->elmmaxcount_node = elmpernode;
#ifndef VSPARSEARRAY_NOT_POWER_OF_TWO_SIZED_NODES
	arr->elmmaxcount_node_log2 = VUTIL_log_base_2_of_n( elmpernode );
#endif

	arr->node = (VSPARSEARRAYNODE **) V_MALLOC( ctx,
						sizeof(VSPARSEARRAYNODE *) * arr->node_count);
	if (!arr->node) {
		return -1;
	}
	for( i=0; i<arr->node_count;i++) {
		arr->node[ i ] = 0;
	}

	VBITVCECTOR_init_from_mem( &arr->imp, arr->elmmaxcount_node, 0, 0 );
		
	return 0;
}

V_EXPORT void VSPARSEARRAY_free( VSPARSEARRAY *arr )
{
	size_t i;

	if (arr->ctx) {
		for(i = 0; i < arr->node_count; i++) {
			if (arr->node[i]) {
				V_FREE( arr->ctx, arr->node[i] );
			}
			arr->node[i] = 0;
		}
		arr->elmcount = 0;
	}
}

static VSPARSEARRAYNODE *new_node( VSPARSEARRAY *arr )
{
	VSPARSEARRAYNODE *ret;
	size_t bitmap_size = arr->imp.size; /*(arr->elmmaxcount_node >> 3 ) + 1 ; */
	size_t data_size = arr->elmsize;


	ret = (VSPARSEARRAYNODE *) 
		V_MALLOC( arr->ctx, sizeof(VSPARSEARRAYNODE) + 
									bitmap_size + data_size );
	if (!ret) {
		return 0;
	}
	ret->elmcount = 1;
	ret->elmmaxcount = 1;
	
	ret->data = ret->bitmask + bitmap_size;
	memset(ret->bitmask, 0, bitmap_size);

	return ret;
}

static VSPARSEARRAYNODE *grow_node( VSPARSEARRAY *arr, size_t idx)
{
	VSPARSEARRAYNODE *node = arr->node[ idx ], *new_node;
	size_t bitmap_size = arr->imp.size; /*(arr->elmmaxcount_node >> 3 ) + 1 ; */
	size_t data_size;
	size_t sz;
	
	sz = VRESIZE_request(arr->sizepolicy, arr, node->elmmaxcount+1);
	if (!sz) {
		return 0;
	}

	if ( idx == (arr->elmmaxcount - 1)) {
		size_t max = arr->elmmaxcount & (arr->elmmaxcount_node - 1);
		if (sz > max) {
			sz = max;
		}
	} else {
		if (sz > arr->elmmaxcount_node) {
			sz = arr->elmmaxcount_node;
		}
	}

	data_size = arr->elmsize * sz;

	new_node = (VSPARSEARRAYNODE * ) V_REALLOC( arr->ctx, node, 
					sizeof(VSPARSEARRAYNODE) + bitmap_size + data_size );
		
	if (!new_node) {
		return 0;
	}
	new_node->elmmaxcount = sz;
	new_node->data = new_node->bitmask + bitmap_size;
	
	arr->node[ idx ] = new_node;
	
	return new_node;
}


size_t VSPARSEARRAY_node_idx( VSPARSEARRAY *arr, size_t idx)
{
#ifndef VSPARSEARRAY_NOT_POWER_OF_TWO_SIZED_NODES
	return idx >> (arr->elmmaxcount_node_log2 - 1);
#else
	return idx / arr->elmmaxcount_node;
#endif
}

size_t VSPARSEARRAY_in_node_idx( VSPARSEARRAY *arr, size_t idx)
{

#ifndef VSPARSEARRAY_NOT_POWER_OF_TWO_SIZED_NODES
	return idx  &  (arr->elmmaxcount_node - 1);
#else
	return idx  %  arr->elmmaxcount_node;
#endif
}


V_EXPORT int VSPARSEARRAY_set(VSPARSEARRAY *arr, size_t idx, void *data, size_t elmsize)
{
	VSPARSEARRAYNODE *node;
	size_t pos, numbits, bitpos;
	V_UINT8 *entry;

	if (elmsize != arr->elmsize) {
		return -1;
	}

	if (idx >= arr->elmmaxcount) {
		return -1;
	}


	pos = VSPARSEARRAY_node_idx(arr, idx);
	bitpos = VSPARSEARRAY_in_node_idx(arr, idx);

	node = arr->node[ pos ];
	if (!node) {
		node = arr->node[ pos ] = new_node( arr );
		if (!node) {
			return -1;
		}

		numbits = 1;
		VBITS_setbit( node->bitmask, bitpos );
		arr->elmcount ++;

	} else {
		numbits = VBITS_countbits(node->bitmask, bitpos + 1);
		if (! VBITS_testbit( node->bitmask, bitpos )) {
			VBITS_setbit( node->bitmask, bitpos );

			/* missing bit will be set */
			numbits += 1;
			
			/* grow if necessary */
			node->elmcount ++;
			if (node->elmcount >= node->elmmaxcount) {

				node = grow_node( arr, pos );
				if (!node) {
					return -1;
				}
			}
	
			/* make root for new element */
			if (numbits < node->elmcount) {
				assert(numbits != 0);
				memmove( node->data + arr->elmsize *  numbits, 
						 node->data + arr->elmsize * (numbits - 1), 						 
						 (node->elmcount - numbits) * arr->elmsize);
			}
			arr->elmcount ++;

#if 0
			if (arr->maxindex == -1) {
				arr->maxindex = arr->minindex = idx;
			} else {
				if (idx > arr->maxindex) {
					arr->minindex = idx;
				}
				if (idx < arr->minindex) {
					arr->minindex = idx;
				}
			}
#endif
		} 
	}
	assert( numbits <= node->elmmaxcount );

	entry = (V_UINT8 *) node->data + (numbits - 1) * arr->elmsize;
	memcpy( entry, data, elmsize );


	return 0;
}


V_EXPORT int  VSPARSEARRAY_unset(VSPARSEARRAY *arr, size_t idx)
{
	VSPARSEARRAYNODE *node;
	size_t numbits, bitpos, pos;

	if (idx >= arr->elmmaxcount) {
		return -1;
	}


	pos = VSPARSEARRAY_node_idx(arr, idx);

	node = arr->node[ pos ];
	if (! node) {
		return -1;
	}

	bitpos = VSPARSEARRAY_in_node_idx(arr, idx);

	/* if bit is set then nothin to do. */
	if (! VBITS_testbit( node->bitmask, bitpos )) {
		return -1;
	}

	/* number of bits till this one */
	numbits = VBITS_countbits(node->bitmask, bitpos);

	/* remove hole hole of element */
	if (numbits < node->elmcount) {
			memmove( node->data + arr->elmsize * (numbits - 1), 
					 node->data + arr->elmsize *  numbits, 					 
					 (node->elmcount - numbits) * arr->elmsize);
	}
	
	
	arr->elmcount --;

#if 0
	switch(arr->elmcount == 0) {
		case 0:
			arr->maxindex = arr->minindex = (size_t) -1;
			break;
		case 1:
			arr->maxindex = arr->minindex = idx;
			break;
		default:
			if (arr->maxindex == idx){ 

			}
			if (arr->minindex == idx) {

			}
	}
#endif

	node->elmcount --;
	if (!node->elmcount) {
		V_FREE( arr->ctx, node );
		arr->node[ pos ] = 0;

	}
	return 0;
}

V_EXPORT int VSPARSEARRAY_isset( VSPARSEARRAY *arr, size_t idx )
{
	VSPARSEARRAYNODE *node;
	size_t pos, bitpos;

	if (idx >= arr->elmmaxcount) {
		return 0;
	}

	pos = VSPARSEARRAY_node_idx(arr, idx);

	node = arr->node[ pos ];
	if (! node) {
		return 0;
	}

	bitpos = VSPARSEARRAY_in_node_idx(arr, idx);

	/* if bit is set then nothin to do. */
	if (! VBITS_testbit( node->bitmask, bitpos )) {
		return 0;
	}
	return 1;
}

V_EXPORT int VSPARSEARRAY_get(VSPARSEARRAY *arr, size_t idx, void **data, size_t *elmsize)
{
	VSPARSEARRAYNODE *node;
	size_t numbits, bitpos, pos;


	pos = VSPARSEARRAY_node_idx(arr, idx);

	node = arr->node[ pos ];
	if (! node) {
		return -1;
	}

	bitpos = VSPARSEARRAY_in_node_idx(arr, idx);

	/* if bit is set then nothin to do. */
	if (! VBITS_testbit( node->bitmask, bitpos  )) {
		return -1;
	}

	/* number of bits till this one */
	numbits = VBITS_countbits(node->bitmask, bitpos + 1);
	if (!numbits) {
		return -1;
	}
	if (numbits >= arr->elmmaxcount) {
		return -1;
	}

	/* get element at this position */
	if (data) {
		*data = node->data + (numbits - 1) * arr->elmsize;
	}

	if (elmsize) {
		*elmsize = arr->elmsize;
	}

	return 0;
}


V_EXPORT int  VSPARSEARRAY_check( VSPARSEARRAY *arr)
{
	size_t i;
	size_t num = 0;
	VSPARSEARRAYNODE *node;
	size_t numbits;

	if (arr->elmmaxcount_node > arr->elmmaxcount) {
		return 0;
	}

	if (arr->elmcount > arr->elmmaxcount) {
		return 0;
	}

	if ((arr->elmmaxcount / arr->elmmaxcount_node) != arr->node_count) {
		return 0;
	}

#ifndef VSPARSEARRAY_NOT_POWER_OF_TWO_SIZED_NODES
	if ( ((size_t) 1 << (arr->elmmaxcount_node_log2 - 1)) != arr->elmmaxcount_node) {
		return 0;
	}
#endif
	
	for( i=0; i<arr->node_count;i++) {
		node = arr->node[ i ];
		if (!node) {
			continue;
		}

		if (node->elmcount > node->elmmaxcount) {
			return 0;
		}

		numbits = VBITS_countbits(node->bitmask, arr->elmmaxcount_node);
		if (numbits != node->elmcount) {
			return 0;
		}

		num += node->elmcount; 
	}	

	if (num != arr->elmcount) {
		return 0;
	}

	return 1;
}

