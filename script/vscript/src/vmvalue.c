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
#include "vm.h"
#include "vmconst.h"
#include <memory.h>
#include <util/vutil.h>
#include <util/vhashfunction.h>
#include "ast.h"

V_EXPORT VCONTEXT *VM_CTX(void *vm)
{
	return ((VSCRIPTVM *) vm)->ctx;
}

VM_OBJ_HEADER *VM_OBJ_HEADER_new_type(VCONTEXT *ctx, int var_type)
{
	switch( var_type ) {
		case S_VAR_HASH:
			return (VM_OBJ_HEADER *) VM_VALUE_HASH_init( ctx, 10 );
		case S_VAR_ARRAY:
			return (VM_OBJ_HEADER *) VM_VALUE_ARRAY_init( ctx, 10 );
	
		default:
			return 0;
	}
}

VM_OBJ_HEADER *VM_OBJ_HEADER_new_base_type(VCONTEXT *ctx, VM_BASE_TYPE_DEF var_type)
{
			// create value type
	switch( var_type ) {
		case VM_STRING:
			return (VM_OBJ_HEADER *) VM_VALUE_STRING_init(ctx, 10 );
		case VM_LONG:
			return (VM_OBJ_HEADER *) VM_VALUE_LONG_init(ctx, 0 );
		case VM_DOUBLE:
			return (VM_OBJ_HEADER *) VM_VALUE_DOUBLE_init(ctx, 0 );
		case VM_HASH:
			return (VM_OBJ_HEADER *) VM_VALUE_HASH_init( ctx, 10 );
		case VM_ARRAY:
			return (VM_OBJ_HEADER *) VM_VALUE_ARRAY_init(ctx, 10 );
	
		default:
			return 0;
	}
}

const char * VM_SCALAR_get_const_string(VSCRIPTVM *vm, VM_CONSTANT_VALUE *val)
{
	return (const char *) vm->string_area + VM_CONSTANT_string_offset(val);
}

size_t VM_SCALAR_get_const_string_length(struct tagVSCRIPTVM *vm, VM_CONSTANT_VALUE *val)
{
	return * ((size_t *) (vm->string_area + VM_CONSTANT_string_offset(val) - sizeof(size_t)) );
}

VM_OBJ_HEADER *VM_OBJ_HEADER_const2scalar( VSCRIPTVM *vm, void *value)
{
	VM_BASE_TYPE  var_type = VM_TYPE_get_base_type(value);
	if (var_type & VM_CONSTANT) {
		switch( var_type ) {
			case VM_LONG | VM_CONSTANT:
				return (VM_OBJ_HEADER *) VM_VALUE_LONG_init(vm->ctx,  
							VM_CONSTANT_long(value));				
			case VM_DOUBLE | VM_CONSTANT:
				return (VM_OBJ_HEADER *) VM_VALUE_DOUBLE_init(vm->ctx,
							(long) VM_CONSTANT_double(value));				
			
			case VM_STRING | VM_CONSTANT:
				return (VM_OBJ_HEADER *) VM_VALUE_STRING_copy(vm->ctx,
								VM_SCALAR_get_const_string(vm, value ), -1);
			default:
				return 0;
		}
	}
	return value;
}

VM_OBJ_HEADER *VM_OBJ_HEADER_copy_scalar(VSCRIPTVM *vm, void *value)
{
	VM_BASE_TYPE  var_type = VM_TYPE_get_base_type(value);

	switch( var_type ) {
		case VM_LONG | VM_CONSTANT:
			return (VM_OBJ_HEADER *) VM_VALUE_LONG_init(vm->ctx,  
						VM_CONSTANT_long(value));				
		case VM_DOUBLE | VM_CONSTANT:
			return (VM_OBJ_HEADER *) VM_VALUE_DOUBLE_init(vm->ctx,
						(long) VM_CONSTANT_double(value));				
		
		case VM_STRING | VM_CONSTANT:
			return (VM_OBJ_HEADER *) VM_VALUE_STRING_copy(vm->ctx,
							VM_SCALAR_get_const_string(vm, value ),
							VM_SCALAR_get_const_string_length(vm, value) );

		case VM_LONG:
			return (VM_OBJ_HEADER *) VM_VALUE_LONG_init(vm->ctx, VM_VALUE_LONG_value(value) );
		case VM_DOUBLE:
			return (VM_OBJ_HEADER *) VM_VALUE_DOUBLE_init(vm->ctx, VM_VALUE_DOUBLE_value(value) );
		case VM_STRING:
			return (VM_OBJ_HEADER *) VM_VALUE_STRING_copy(vm->ctx, VM_VALUE_STRING_value(value) , VM_VALUE_STRING_strlen(value) );
		default:
			return 0;
	}

}


void VM_OBJECT_destroy(VCONTEXT *ctx, VM_OBJ_HEADER *hdr) {
	switch( hdr->type.base_type ) {
	 
	 case VM_STRING:
		VM_VALUE_STRING_free( ctx, (VM_VALUE_STRING *) hdr );
		break;

	 case VM_LONG:
	 case VM_DOUBLE:
		V_FREE( ctx, hdr );
		break;
 
	 case VM_HASH:
		VM_VALUE_HASH_free( ctx, (VM_VALUE_HASH *) hdr );
		break;

	 case VM_ARRAY:
		VM_VALUE_ARRAY_free( ctx, (VM_VALUE_ARRAY *) hdr );
		break;
	 
	 default:
		 ;
	}	
}

V_EXPORT int VM_VALUE_STRING_add_cstr(VCONTEXT *ctx, VM_VALUE_STRING * val, const char *str, size_t str_len)
{
	int size;
	char *newval;

	if ( (val->len + str_len + 1 ) < val->size) {

add:
		memcpy( val->val + str_len, str, str_len + 1 );
		val->len += str_len;
		return 0;
	}
	// grow the current object - this should be some per vm policy ? 
	size = (val->size + str_len + 1) + 20;

	newval = (char *) V_REALLOC( ctx, val->val, size );
	if (!newval) {
		return -1;
	}

	val->val = newval;
	val->size = size;
	goto add;

	return 0;
}

V_EXPORT int VM_VALUE_STRING_add(VCONTEXT *ctx, VM_VALUE_STRING * val, VM_VALUE_STRING * add )
{
	return VM_VALUE_STRING_add_cstr(ctx, val, add->val, add->len);
}

/* *** */
typedef enum {
	VM_VALUE_HASH_FIND,
	VM_VALUE_HASH_INSERT,
	VM_VALUE_HASH_DELETE,
}
	VM_VALUE_HASH_OP;


static int VM_VALUE_HASH_IMP_init(VCONTEXT *ctx,VM_VALUE_HASH_IMP *imp, size_t bcount)
{

	bcount = VUTIL_round_to_power_of_n( bcount );
	imp->buckets = V_MALLOC(ctx, sizeof(VM_VALUE_HASH_BUCKET) * bcount);
	if (imp->buckets == 0) {
		return -1;
	}
	memset(imp->buckets, 0, sizeof(VM_VALUE_HASH_BUCKET *) * bcount);
	imp->buckets_count = bcount;
	imp->elmcount = 0;
	return 0;
}

void VM_VALUE_HASH_IMP_free( VCONTEXT *ctx, VM_VALUE_HASH_IMP *imp)
{
	size_t i, j;

	for(i = 0; i<imp->buckets_count; i++) {
		VM_VALUE_HASH_BUCKET *cur, *next;

		cur = imp->buckets[ i ];

		while( cur ) {
			for(j = 0; j < VM_VALUE_HASH_ENTRIES_PER_BUCKET; j++ ) {
				if (cur->entry[ j ].hash) {
					VM_OBJ_HEADER_release( ctx, cur->entry[ j ].key );
					VM_OBJ_HEADER_release( ctx, cur->entry[ j ].value );
				}
			}

			next = cur->next;
			V_FREE( ctx, cur );
			cur = next;
		}
	} 

	V_FREE( ctx, imp->buckets );
}

static int compare_keys(VSCRIPTVM *vm, VM_OBJ_HEADER *arg, const char *rhs, int len_rhs)
{
	char tmp_buf_lhs[100];
	const char *ret_lhs;
	int len_lhs;

	ret_lhs = VM_SCALAR_to_string(vm, arg, tmp_buf_lhs, &len_lhs);

	if (len_lhs != len_rhs) {
		return 0;
	}

	return strcmp(ret_lhs, rhs) == 0;	
}

			

VM_OBJ_HEADER *VM_VALUE_HASH_IMP_do_hash(VSCRIPTVM *vm, VM_VALUE_HASH_OP op, 
											VM_VALUE_HASH_IMP *imp, 
											VM_OBJ_HEADER *key, VM_OBJ_HEADER *value)
{
	unsigned int hash_val; 
	int idx, i, found;
	VM_VALUE_HASH_BUCKET *buck;
	int empty_idx = -1;
	VM_VALUE_HASH_BUCKET *empty_entry;
	char tmp_buf_rhs[100];
	const char *ret_rhs;
	int  len_rhs;

	ret_rhs = VM_SCALAR_to_string(vm, key, tmp_buf_rhs, &len_rhs);
	
	hash_val = VHASHFUNCTION_Bob_Jenkins_one_at_a_time((void *) ret_rhs, VHASH_STRING);

	idx = hash_val & (imp->buckets_count - 1);
	buck = imp->buckets[idx];

	found = 0;
	if (op == VM_VALUE_HASH_INSERT) {
		for(;buck; buck = buck->next) {

			for(i=0;i<VM_VALUE_HASH_ENTRIES_PER_BUCKET;i++) {
				if (buck->entry[i].hash == hash_val) {
					if (compare_keys(vm, buck->entry[i].key, ret_rhs, len_rhs)) {
						found = 1;
						goto next;
					}
				}
			}
			buck = buck->next;
		}
	} else {
		for(;buck; buck = buck->next) {

			for(i=0;i<VM_VALUE_HASH_ENTRIES_PER_BUCKET;i++) {
				if (buck->entry[i].hash == hash_val) {
					if (compare_keys(vm, buck->entry[i].key, ret_rhs, len_rhs)) {
						found = 1;
						goto next;
					}
				}
				if (empty_idx == -1 && buck->entry[i].hash == 0  ) {
					empty_idx = i;
					empty_entry = buck;				
				}
			}
		}	
	}
next:

	switch(op) {
	case VM_VALUE_HASH_FIND:
		if (found) {
			return buck->entry[i].value;
		}	
		break;

	case VM_VALUE_HASH_INSERT:

		if (found) {
			// replace old value with new value.
			VM_OBJ_HEADER_release( vm->ctx, buck->entry[i].value);
			buck->entry[i].value = value;
			return 0;
		}

		if (empty_idx == -1) {
			// insert new bucket entry etc. etc.
			empty_entry = 
				(VM_VALUE_HASH_BUCKET *) 
					V_MALLOC(vm->ctx, 
						sizeof(VM_VALUE_HASH_BUCKET) + VM_VALUE_HASH_ENTRIES_PER_BUCKET * sizeof(VM_VALUE_HASH_ENTRY));

			if (!empty_entry) {
				return (VM_OBJ_HEADER *) -1;
			}

			empty_entry->next = 0;

			for(i =0; i < VM_VALUE_HASH_ENTRIES_PER_BUCKET; i++) {
				empty_entry->entry[i].hash = 0;	
			}

			empty_entry->next = imp->buckets[ idx ];
			imp->buckets[ idx ] = empty_entry;

			empty_idx = 0;
		}

		VM_OBJ_HEADER_add_ref(key);		
		VM_OBJ_HEADER_add_ref(value);

		empty_entry->entry[empty_idx].hash = hash_val;
		empty_entry->entry[empty_idx].key = key;
		empty_entry->entry[empty_idx].value = value;
		imp->elmcount++;

		break;

	case VM_VALUE_HASH_DELETE:
		if (found) {
			buck->entry[i].hash = 0;
			VM_OBJ_HEADER_release( vm->ctx, buck->entry[i].key);
			VM_OBJ_HEADER_release( vm->ctx, buck->entry[i].value);
		}
		break;

	}
	return 0;
}

/* *** */


VM_VALUE_HASH *VM_VALUE_HASH_init( VCONTEXT *ctx, int buckets)
{
   VM_VALUE_HASH * ret;

   ret = (VM_VALUE_HASH *) V_MALLOC( ctx , sizeof(VM_VALUE_HASH));
   if (!ret) {
	 return 0;
   }
   if (VM_VALUE_HASH_IMP_init(ctx, &ret->imp, buckets) ) {
	   V_FREE( ctx, ret );
	   return 0;
   }
   ret->base.ref_count = 1;
   ret->base.type.base_type = VM_HASH;
   return ret;

}

void VM_VALUE_HASH_free( VCONTEXT *ctx, VM_VALUE_HASH *hash)
{
	VM_VALUE_HASH_IMP_free( ctx, &hash->imp );
}


VM_OBJ_HEADER * VM_VALUE_HASH_get( VSCRIPTVM *vm, VM_VALUE_HASH *arr, VM_OBJ_HEADER *index )
{
	return VM_VALUE_HASH_IMP_do_hash(vm, VM_VALUE_HASH_FIND, &arr->imp, index, 0 );
}

int VM_VALUE_HASH_set( VSCRIPTVM *vm,  VM_VALUE_HASH *arr, VM_OBJ_HEADER *index, VM_OBJ_HEADER *rhs)
{
    if (VM_VALUE_HASH_IMP_do_hash(vm, VM_VALUE_HASH_INSERT, &arr->imp, index, rhs )) {
		return -1;
	}
	return 0;
}

int VM_VALUE_HASH_unset( VSCRIPTVM *vm,  VM_VALUE_HASH *arr, VM_OBJ_HEADER *index)
{
    if (VM_VALUE_HASH_IMP_do_hash(vm, VM_VALUE_HASH_DELETE, &arr->imp, index, 0 )) {
		return -1;
	}
	return 0;
}


/* *** */
V_EXPORT long VM_SCALAR_to_long(VSCRIPTVM *vm,void *value)
{
	VM_BASE_TYPE ty;
	
	ty =  VM_TYPE_get_base_type(value);

	switch(ty) {
		case VM_LONG:
			return VM_VALUE_LONG_value(value);				
		
		case VM_DOUBLE:
			return (long) VM_VALUE_DOUBLE_value(value);				
		
		case VM_STRING:
			return atoi( VM_VALUE_STRING_value(value) );

		case VM_LONG | VM_CONSTANT:
			return VM_CONSTANT_long(value);				

		case VM_DOUBLE | VM_CONSTANT:
			return (long) VM_CONSTANT_double(value);				
		
		case VM_STRING | VM_CONSTANT:
			return atoi( VM_SCALAR_get_const_string(vm, value) );				

	}
	
	// shouldnt get here
	return 0;
}

V_EXPORT double VM_SCALAR_to_double(VSCRIPTVM *vm, void *value)
{
	VM_BASE_TYPE ty;

	
	ty =  VM_TYPE_get_base_type(value);
	

	switch(ty) {
		case VM_LONG:
			return (double) VM_VALUE_LONG_value(value);				
		
		case VM_DOUBLE:
			return VM_VALUE_DOUBLE_value(value);				
		
		case VM_STRING:
			return atof( VM_VALUE_STRING_value(value) );

		case VM_LONG | VM_CONSTANT:
			return (double) VM_CONSTANT_long(value);
			
		case VM_DOUBLE | VM_CONSTANT:
			return VM_CONSTANT_double(value);				
		
		case VM_STRING | VM_CONSTANT:
			return atof( VM_SCALAR_get_const_string(vm, value) );				
	}
	
	// shouldnt get here
	return 0;
}
 
V_EXPORT const char * VM_SCALAR_to_string(VSCRIPTVM *vm, void *value, char *tmp_buf, int *string_length)
{
	VM_BASE_TYPE ty;

	
	ty =  VM_TYPE_get_base_type(value);

	switch(ty) {
		case VM_LONG:
			*string_length = sprintf(tmp_buf,"%ld", VM_VALUE_LONG_value(value) );
			return tmp_buf;
		
		case VM_DOUBLE:
			*string_length = sprintf(tmp_buf,"%e", VM_VALUE_DOUBLE_value(value) );
			return tmp_buf;
		
		case VM_STRING:
			*string_length = VM_VALUE_STRING_strlen(value);
			return VM_VALUE_STRING_value(value);

		case VM_LONG | VM_CONSTANT:
			*string_length = sprintf(tmp_buf,"%ld", VM_CONSTANT_long(value) );				
			return tmp_buf;
		
		case VM_DOUBLE | VM_CONSTANT:
			*string_length = sprintf(tmp_buf,"%ld", (long) VM_CONSTANT_double(value) );				
			return tmp_buf;
		
		case VM_STRING | VM_CONSTANT:
			*string_length = VM_SCALAR_get_const_string_length(vm, value);
 			return VM_SCALAR_get_const_string(vm, value);				
	}

	// really shouldnt get here
	return 0;
}	



V_EXPORT VM_OBJ_HEADER  * VM_VALUE_DOUBLE_return( void *vm, double val )
{
  return (VM_OBJ_HEADER *) VM_VALUE_DOUBLE_init( ((VSCRIPTVM *) vm)->ctx, val );
}

V_EXPORT VM_OBJ_HEADER  * VM_VALUE_LONG_return( void *vm, long val )
{
  return (VM_OBJ_HEADER *) VM_VALUE_LONG_init( ((VSCRIPTVM *) vm)->ctx, val );
}

V_EXPORT VM_OBJ_HEADER  * VM_VALUE_STRING_return( void *vm, const char * val )
{
  return (VM_OBJ_HEADER *) VM_VALUE_STRING_copy(((VSCRIPTVM *) vm)->ctx, val, -1 );
}


V_EXPORT VM_OBJ_HEADER  * VM_VALUE_STRING_return_n( void *vm, const char * val, int len )
{
  return (VM_OBJ_HEADER *) VM_VALUE_STRING_copy_n(((VSCRIPTVM *) vm)->ctx, val, len  );

}






/* *** */
#if 0
VM_OBJ_HEADER * VM_VALUE_HASH_get( VM_VALUE_HASH *arr, VM_OBJ_HEADER *index )
{
	return 0;
}

int VM_VALUE_HASH_set( VCONTEXT *ctx,  VM_VALUE_HASH *arr, VM_OBJ_HEADER *index, VM_OBJ_HEADER *rhs)
{
	return 0;
}
#endif
