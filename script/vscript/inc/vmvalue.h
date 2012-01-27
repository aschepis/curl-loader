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

#ifndef __VMVALUE_H_
#define __VMVALUE_H_

#include <util/vbasedefs.h>
#include <util/vbuckethash.h>
#include <string.h>

struct tagVSCRIPTVM;


V_EXPORT VCONTEXT *VM_CTX(void *vm);

/* *** enumeration of base types  *** */

typedef enum {
  VM_STRING  = 1,
  VM_LONG    = 2,
  VM_DOUBLE  = 3, 
  
  VM_HASH    = 16,
  VM_ARRAY   = 32,

  VM_CONSTANT = 128

} VM_BASE_TYPE_DEF;

#define VM_IS_CONSTANT(type) ((type) & VM_CONSTANT)
#define VM_IS_SCALAR(type)   ((type) & 3)
#define VM_IS_NUMERIC(type)  (VM_IS_SCALAR(type) && VM_IS_SCALAR(type) <= 2)


/* *** base type data *** */

typedef unsigned char VM_BASE_TYPE;

typedef  union {
  void *metadata;
  VM_BASE_TYPE  base_type;
} 
  VM_TYPE;

#define VM_TYPE_get_base_type(x) ((VM_TYPE *) (x))->base_type


/* *** base type of all types *** */

typedef struct tagVM_OBJ_HEADER {
  VM_TYPE type;
  int ref_count;
}
  VM_OBJ_HEADER;


#define VM_OBJ_HEADER_type(v) ((VM_OBJ_HEADER *)v)->type

VM_OBJ_HEADER *VM_OBJ_HEADER_copy_scalar(struct tagVSCRIPTVM *vm, void  *value);

VM_OBJ_HEADER *VM_OBJ_HEADER_new_base_type(VCONTEXT *ctx, VM_BASE_TYPE_DEF var_type);

VM_OBJ_HEADER *VM_OBJ_HEADER_new_type(VCONTEXT *ctx, int var_type);

V_INLINE void VM_OBJ_HEADER_add_ref(void *v);

V_INLINE int VM_OBJ_HEADER_release(VCONTEXT *ctx,  void *v);


/* *** long type header definitions *** */

typedef struct tagVM_VALUE_LONG {
  VM_OBJ_HEADER base;
  long val;
}
  VM_VALUE_LONG;

#define VM_VALUE_LONG_value(value) ((VM_VALUE_LONG *) value)->val

V_INLINE void VM_VALUE_LONG_init_stc( VM_VALUE_LONG *stc, long val );

V_INLINE VM_VALUE_LONG * VM_VALUE_LONG_init( VCONTEXT *ctx, long val );

V_EXPORT VM_OBJ_HEADER  * VM_VALUE_LONG_return( void *vm, long val );

/* *** double type header definitions *** */

typedef struct tagVM_VALUE_DOUBLE {
  VM_OBJ_HEADER base;
  double val;
}
  VM_VALUE_DOUBLE;

#define VM_VALUE_DOUBLE_value(value) ((VM_VALUE_DOUBLE *) value)->val

V_INLINE void VM_VALUE_DOUBLE_init_stc( VM_VALUE_DOUBLE *ret, double val );

V_INLINE VM_VALUE_DOUBLE * VM_VALUE_DOUBLE_init( VCONTEXT *ctx, double val );

V_EXPORT VM_OBJ_HEADER  * VM_VALUE_DOUBLE_return( void *vm, double val );

/* *** string type header definitions *** */

typedef struct tagVM_VALUE_STRING {
  VM_OBJ_HEADER base;
  size_t size;
  size_t len; //strlen
  char *val;
}
  VM_VALUE_STRING;
  
#define VM_VALUE_STRING_value(value) ((VM_VALUE_STRING *) value)->val
#define VM_VALUE_STRING_strlen(value) ((VM_VALUE_STRING *) value)->len
  
V_INLINE VM_VALUE_STRING * VM_VALUE_STRING_init(VCONTEXT *ctx, size_t len); 

V_INLINE VM_VALUE_STRING * VM_VALUE_STRING_init_add(VCONTEXT *ctx, const char *strl, size_t strl_len, const char *strr, size_t strr_len);

V_INLINE VM_VALUE_STRING * VM_VALUE_STRING_copy(VCONTEXT *ctx, const char *txt, size_t len );

V_INLINE VM_VALUE_STRING * VM_VALUE_STRING_copy_n(VCONTEXT *ctx, const char *txt, size_t len );

V_INLINE void VM_VALUE_STRING_free(VCONTEXT *ctx, VM_VALUE_STRING * val);

VM_OBJ_HEADER *VM_OBJ_HEADER_const2scalar( struct tagVSCRIPTVM *vm, void *value);

V_EXPORT int VM_VALUE_STRING_add(VCONTEXT *ctx, VM_VALUE_STRING * val, VM_VALUE_STRING * add );

V_EXPORT int VM_VALUE_STRING_add_cstr(VCONTEXT *ctx, VM_VALUE_STRING * val, const char *str, size_t str_len);



V_EXPORT VM_OBJ_HEADER  * VM_VALUE_STRING_return( void *vm, const char * val );

V_EXPORT VM_OBJ_HEADER  * VM_VALUE_STRING_return_n( void *vm, const char * val, int len );

/* *** scalar accessor *** */

V_EXPORT long VM_SCALAR_to_long(struct tagVSCRIPTVM *vm,void *value);

V_EXPORT double VM_SCALAR_to_double(struct tagVSCRIPTVM *vm, void *value);

V_EXPORT const char * VM_SCALAR_to_string(struct tagVSCRIPTVM *vm, void *value, char *tmp_buf, int *string_length);

/* *** array type header definitions *** */

typedef struct tagVM_VALUE_ARRAY {
  VM_OBJ_HEADER base;
  int size;
  int count; // number of elements
  VM_OBJ_HEADER **val;
}
  VM_VALUE_ARRAY;


V_INLINE VM_VALUE_ARRAY *VM_VALUE_ARRAY_init( VCONTEXT *ctx, int size);

V_INLINE void VM_VALUE_ARRAY_free( VCONTEXT *ctx, VM_VALUE_ARRAY * arr);

V_INLINE VM_OBJ_HEADER * VM_VALUE_ARRAY_get( VM_VALUE_ARRAY *arr, int idx );
V_INLINE int VM_VALUE_ARRAY_set( VCONTEXT *ctx,  VM_VALUE_ARRAY *arr, int idx, VM_OBJ_HEADER *rhs);
V_INLINE int VM_VALUE_ARRAY_unset( VCONTEXT *ctx,  VM_VALUE_ARRAY *arr, int idx);


/* *** hash type header definitions *** */


typedef struct tagVM_VALUE_HASH_ENTRY {
	unsigned int hash;
	VM_OBJ_HEADER *key;
	VM_OBJ_HEADER *value;
} 
  VM_VALUE_HASH_ENTRY;

#define VM_VALUE_HASH_ENTRIES_PER_BUCKET 10

typedef struct tagVM_VALUE_HASH_BUCKET {
	struct tagVM_VALUE_HASH_BUCKET *next;
	VM_VALUE_HASH_ENTRY entry[]; 
} 
  VM_VALUE_HASH_BUCKET;

typedef struct VM_VALUE_HASH_IMP {
	size_t elmcount;
	size_t buckets_count;
	VM_VALUE_HASH_BUCKET **buckets;
} 
    VM_VALUE_HASH_IMP;

	
typedef struct tagVM_VALUE_HASH {
	VM_OBJ_HEADER base;
	VM_VALUE_HASH_IMP imp;
} 
	VM_VALUE_HASH;

VM_VALUE_HASH *VM_VALUE_HASH_init( VCONTEXT *ctx, int buckets);


void VM_VALUE_HASH_free( VCONTEXT *ctx, VM_VALUE_HASH *hash);


VM_OBJ_HEADER * VM_VALUE_HASH_get( struct tagVSCRIPTVM *vm, VM_VALUE_HASH *arr, VM_OBJ_HEADER *index );

int VM_VALUE_HASH_set( struct tagVSCRIPTVM *vm, VM_VALUE_HASH *arr, VM_OBJ_HEADER *index, VM_OBJ_HEADER *rhs);

int VM_VALUE_HASH_unset( struct tagVSCRIPTVM *vm, VM_VALUE_HASH *arr, VM_OBJ_HEADER *index);


/* *** xmethod parameter structure *** */

typedef struct tagVSCRIPT_XMETHOD_FRAME {
	VM_OBJ_HEADER *retval;// pointer to return value.
	int	param_count;	  // number of parameters
	VM_OBJ_HEADER **param; // function parameters
} VSCRIPT_XMETHOD_FRAME;

/* *** inlines *** */

void VM_OBJECT_destroy(VCONTEXT *ctx,  VM_OBJ_HEADER *hdr);

V_INLINE void VM_OBJ_HEADER_add_ref(void *v)
{
	VM_OBJ_HEADER *obj = (VM_OBJ_HEADER *) v;

	if (! VM_IS_CONSTANT( obj->type.base_type )) {
		obj->ref_count ++;
	}
}

V_INLINE int VM_OBJ_HEADER_release(VCONTEXT *ctx,  void *v)
{
	VM_OBJ_HEADER * hdr = (VM_OBJ_HEADER *) v;

	if (! VM_IS_CONSTANT( hdr->type.base_type )) {

		if ( -- hdr->ref_count == 0 ) {
			// relese the object
			VM_OBJECT_destroy( ctx, hdr );
			return 1;
		}

	}
	return 0;
}

/* *** */

V_INLINE void VM_VALUE_LONG_init_stc( VM_VALUE_LONG *stc, long val )
{
  stc->base.type.base_type = VM_LONG;
  stc->base.ref_count = 1;
  stc->val = val;
}

V_INLINE VM_VALUE_LONG * VM_VALUE_LONG_init( VCONTEXT *ctx, long val )
{
  VM_VALUE_LONG *ret;
  
  ret = (VM_VALUE_LONG *) V_MALLOC( ctx, sizeof( VM_VALUE_LONG) );
  if (!ret) {
	return 0;
  }

  VM_VALUE_LONG_init_stc(ret, val);

  return ret;
}

/* *** */

V_INLINE void VM_VALUE_DOUBLE_init_stc( VM_VALUE_DOUBLE *ret, double val )
{
  ret->base.type.base_type = VM_DOUBLE;
  ret->base.ref_count = 1;
  ret->val = val;
}

V_INLINE VM_VALUE_DOUBLE * VM_VALUE_DOUBLE_init( VCONTEXT *ctx, double val )
{
  VM_VALUE_DOUBLE *ret;
  
  ret = (VM_VALUE_DOUBLE *) V_MALLOC( ctx, sizeof( VM_VALUE_LONG) );
  if (!ret) {
	return 0;
  }
  VM_VALUE_DOUBLE_init_stc( ret, val );

  return ret;
}

V_EXPORT VM_OBJ_HEADER  * VM_VALUE_DOUBLE_return( void *vm, double val );

/* *** */

V_INLINE VM_VALUE_ARRAY *VM_VALUE_ARRAY_init( VCONTEXT *ctx, int size) 
{ 
  VM_VALUE_ARRAY *ret;

  ret = (VM_VALUE_ARRAY *) V_MALLOC(ctx, sizeof(VM_VALUE_ARRAY));
  if (!ret) {
    return 0;
  }

  ret->val = (VM_OBJ_HEADER **) V_MALLOC( ctx, sizeof(void *) * size );
  if (!ret->val) {
	V_FREE(ctx, ret);
	return 0;
  }

  ret->base.type.base_type = VM_ARRAY;
  ret->base.ref_count = 1;
  ret->size = size;
  ret->count = -1;

  return ret;
}

V_INLINE void VM_VALUE_ARRAY_free( VCONTEXT *ctx, VM_VALUE_ARRAY * arr) 
{
  int i;

  for(i = 0; i < arr->size; i++ ) {
	  if (arr->val[i]) {
		 VM_OBJ_HEADER_release( ctx, arr->val[i] );	
	  }
  }	
  V_FREE( ctx, arr->val );
  V_FREE( ctx, arr );
}

V_INLINE VM_OBJ_HEADER * VM_VALUE_ARRAY_get( VM_VALUE_ARRAY *arr, int idx )
{
	if (idx < arr->size) {
		return arr->val[ idx ];
	}
	return 0;
}


V_INLINE int VM_VALUE_ARRAY_set( VCONTEXT *ctx,  VM_VALUE_ARRAY *arr, int idx, VM_OBJ_HEADER *rhs)
{
	int sz;
	VM_OBJ_HEADER **new_val;

	if (idx < arr->size) {
set_a:
		VM_OBJ_HEADER_add_ref( rhs );

		if (idx > arr->count) {
			arr->count = idx;
		}

		arr->val[ idx ] = rhs;

		return 0;
	} 

	sz = idx + 10; // should be some kind of policy

	new_val = (VM_OBJ_HEADER **) V_REALLOC( ctx, arr->val, sz * sizeof(void *));
	if (!new_val) {
		return -1;
	}

	arr->val = new_val;
	arr->size = sz;
	goto set_a;

	return 0;
	
}


V_INLINE int VM_VALUE_ARRAY_unset( VCONTEXT *ctx,  VM_VALUE_ARRAY *arr, int idx)
{
	VM_OBJ_HEADER * val = arr->val[ idx ] ;
	if (!val) {
		return -1;
	}
	
	VM_OBJ_HEADER_release( ctx,  val );
	arr->val[ idx ] = 0;

	return 0;
}



/* *** */

V_INLINE VM_VALUE_STRING * VM_VALUE_STRING_init(VCONTEXT *ctx, size_t len)
{ 
  size_t mlen;
  VM_VALUE_STRING * ret;
  
  mlen = len;
  ret = (VM_VALUE_STRING *) V_MALLOC( ctx, sizeof(VM_VALUE_STRING));
  if (!ret) {
	return 0;
  }

  if (!len) {
	  mlen = 10;
  }

  ret->val = (char *) V_MALLOC(ctx, mlen);
  if (!ret->val) {
	  V_FREE(ctx, ret);
	  return 0;
  }

  ret->base.type.base_type = VM_STRING;
  ret->base.ref_count = 1;
  ret->len = mlen;
  ret->size = 0;

  if (len == 0) {
	*ret->val = '\0';
  }

  return ret;
}

V_INLINE VM_VALUE_STRING * VM_VALUE_STRING_init_add(VCONTEXT *ctx, const char *strl, size_t strl_len, const char *strr, size_t strr_len)
{
  size_t all;
  VM_VALUE_STRING * ret = (VM_VALUE_STRING *) V_MALLOC( ctx, sizeof(VM_VALUE_STRING));
  if (!ret) {
	return 0;
  }

  all = strl_len + strr_len;

  ret->val = (char *) V_MALLOC(ctx, all + 1);
  if (!ret->val) {
	  V_FREE(ctx, ret);
	  return 0;
  }

  ret->base.type.base_type = VM_STRING;
  ret->base.ref_count = 1;
  ret->len = all;
  ret->size = 0;

  memcpy( ret->val, strl, strl_len );
  memcpy( ret->val + strl_len, strr, strr_len );
  ret->val[ all ] = '\0';
  return ret;
}


V_INLINE VM_VALUE_STRING * VM_VALUE_STRING_copy(VCONTEXT *ctx, const char *txt, size_t len )
{
  VM_VALUE_STRING * ret;

  if (len == (size_t) -1) {
	len = strlen(txt);
  }

  ret = (VM_VALUE_STRING *) V_MALLOC( ctx, sizeof(VM_VALUE_STRING) );
  if (!ret) { 
	return 0;
  }

  if (!len) {
	  len = 10;
  }

  ret->val = (char *) V_MALLOC( ctx, len );
  if (!ret->val) {
	  V_FREE(ctx, ret);
	  return 0;
  }

  ret->base.type.base_type = VM_STRING;
  ret->base.ref_count = 1;
  ret->len = len;
  ret->size = 0;
  strcpy( ret->val, txt );

  return ret;
}


V_INLINE VM_VALUE_STRING * VM_VALUE_STRING_copy_n(VCONTEXT *ctx, const char *txt, size_t len )
{
  VM_VALUE_STRING * ret;

  if (len == (size_t) -1) {
	len = strlen(txt);
  }

  ret = (VM_VALUE_STRING *) V_MALLOC( ctx, sizeof(VM_VALUE_STRING) );
  if (!ret) { 
	return 0;
  }

  if (!len) {
	  len = 10;
  }

  ret->val = (char *) V_MALLOC( ctx, len );
  if (!ret->val) {
	  V_FREE(ctx, ret);
	  return 0;
  }

  ret->base.type.base_type = VM_STRING;
  ret->base.ref_count = 1;
  ret->len = len;
  ret->size = 0;
  strncpy( ret->val, txt, len );
  ret->val[ len ] = '\0';

  return ret;
}

V_INLINE void VM_VALUE_STRING_free(VCONTEXT *ctx, VM_VALUE_STRING * val) 
{
	V_FREE( ctx, val->val );
	V_FREE( ctx, val );
}


#endif
