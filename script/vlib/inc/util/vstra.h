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

#ifndef _VSTRA_H_
#define _VSTRA_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>

typedef enum {
  VCOPY_STRING,
  VNEW_STRING,

} VSTRA_INIT_OPTION;

/**
 * @brief String class for single byte characters.
 */
typedef struct  {

  VCONTEXT *ctx;
  VRESIZE_policy sizepolicy;

  size_t    buflen;
  size_t    size;
  char     *buffer;

} VSTRA;



/**
 * @brief get initial size of empty string, when initial size is not supplied (global value for all instances)
 */
V_EXPORT size_t VSTRA_get_size_of_empty_string(void);

/**
 * @brief set initial size of empty string, when initial size is not supplied (global value for all instances)
 */
V_EXPORT void   VSTRA_set_size_of_empty_string(size_t sz);


/**
 * @brief object construct, initialise a new empty string of given maximum size, memory allocated from allocation interface.
 * @param ctx (in)		allocator interface. (if null we are using the default allocator)
 * @param str (in)		the object
 * @param sz  (in)		the initial size (in bytes) of the new object; 0 means that we assume the default size.
 * @param sizepolicy (in) size  
 * @return 0 if ok. non zero if failure.
 */
V_EXPORT int VSTRA_initn( VCONTEXT *ctx, VSTRA *str, size_t sz);

/**
 * @brief object construct, initialise a new empty string of given maximum size, memory allocated from allocation interface.
 * @param ctx (in)		allocator interface. (if null we are using the default allocator)
 * @param str (in)		the object
 * @param sz  (in)		the initial size (in bytes) of the 
 * @param sizepolicy (in) policy for resizing of object;
 * @return 0 if ok. non zero if failure.
 */
V_INLINE int VSTRA_init( VCONTEXT *ctx, VSTRA *str)
{
	return VSTRA_initn( ctx, str, 0);
}

V_INLINE  int VSTRA_init_fixed( VSTRA * str, void *ptr, size_t bufsize, int str_is_empty)
{
	if (!ptr || bufsize <= 0) {
		return -1;
	}

	str->ctx = 0;
	str->sizepolicy.type = VRESIZE_fixed;

	str->buffer = ptr;
	str->buflen = bufsize;

	if (str_is_empty) {
		str->size = 0;
		str->buffer[0] = '\0';
	} else {
		str->size = strlen(str->buffer);
	}
	return 0;
}

/**
 * @brief set resize policy
 
 * The resize policy determines how much space is allocated in advance, given
 * that the container has to be resized.

 * Function has no effect iff the object is a fixed sized object (constructors VSTRA_init_fixed VSTRA_init_stack)

 * @brief arr the object
 * @brief sizepolicy amount 
 */
V_INLINE void  VSTRA_set_sizepolicy( VSTRA *str, VRESIZE_policy sizepolicy)
{
	str->sizepolicy = sizepolicy;
}


#define VSTRA_init_stack( str, sz) \
do { \
	(str)->ctx = 0; \
	(str)->sizepolicy.type = VRESIZE_fixed; \
	(str)->buffer = (char *) alloca((sz)); \
	(str)->buflen = (sz); \
	(str)->size = 0; \
	(str)->buffer[0] = '\0'; \
} while(0);

/* init new string by copying it from C string */

V_EXPORT int VSTRA_sinit( VSTRA *dest, VCONTEXT *ctx, const char *cstr, size_t sz);

V_EXPORT int VSTRA_sinitn( VSTRA *dest, VCONTEXT *ctx, const char *cstr);


/* destructor */

V_INLINE void VSTRA_free(  VSTRA *str)
{
	if (str->ctx) {
		if (str->buffer) {
			V_FREE(str->ctx, str->buffer);
			str->buffer = 0;
		}
	}
}

/* change of size */

V_EXPORT int VSTRA_resize( VSTRA *str, size_t reserved);




/* modify an existing object */

V_INLINE void VSTRA_set_zero( VSTRA *str) 
{
	str->buffer[0] = '\0';
	str->size = 0;
}


V_EXPORT int VSTRA_cpy( VSTRA_INIT_OPTION opt, VSTRA *dest, VSTRA *src);

V_EXPORT int VSTRA_ncpy( VSTRA_INIT_OPTION opt, VSTRA *dest, VSTRA *src, size_t sz);

V_EXPORT int VSTRA_scpy( VSTRA *dest, const char *cstr);

V_EXPORT int VSTRA_scpyn( VSTRA *dest, const char *cstr, size_t sz);


/* add string to object */


V_EXPORT int VSTRA_cat ( VSTRA *dst, VSTRA *src );

V_EXPORT int VSTRA_ncat ( VSTRA *dst, VSTRA *src, size_t len );

V_EXPORT int VSTRA_scat(VSTRA *dst, const char *src );

V_EXPORT int VSTRA_sncat ( VSTRA *dst, const char *src, size_t len );


/* vb stuff */


V_EXPORT int VSTRA_left( VSTRA_INIT_OPTION opt, VSTRA *ret, VSTRA *str, size_t len );

V_EXPORT int VSTRA_right( VSTRA_INIT_OPTION opt, VSTRA *ret, VSTRA *str, size_t len );

V_EXPORT int VSTRA_mid( VSTRA_INIT_OPTION opt, VSTRA *ret, VSTRA *str, size_t pos, size_t len );


/* read string from file */


V_EXPORT int VSTRA_fgetline( VSTRA *ret, FILE *fp, int eoflinechar);

V_EXPORT int VSTRA_fgets( VSTRA *ret, FILE *fp);

V_EXPORT int VSTRA_readfile( VSTRA *ret, FILE *fp);

/* accessor functions */


V_INLINE const char *VSTRA_cstr( VSTRA *str)
{
	return str->buffer;
}

V_INLINE char *VSTRA_str( VSTRA *str)
{
	return str->buffer;
}



V_INLINE size_t VSTRA_length( VSTRA *str) 
{
	return str->size;
}

V_INLINE size_t VSTRA_maxsize( VSTRA *str )
{
	return str->buflen;
}


#ifdef  __cplusplus
}
#endif

#endif

