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
#include <util/vstra.h>
#include <util/vutil.h>
#include <ctype.h>

size_t g_null_string_buf_size = 20;

V_EXPORT size_t VSTRA_get_size_of_empty_string(void)
{
	return g_null_string_buf_size;
}

V_EXPORT void   VSTRA_set_size_of_empty_string(size_t sz)
{
	g_null_string_buf_size = sz;
}


V_STATIC_INLINE int VSTRA_construct( VSTRA_INIT_OPTION opt, VCONTEXT *ctx, VSTRA *str, const char *cstr, size_t sz, VRESIZE_policy sizepolicy)
{

	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	if (!ctx) {
		return -1;
	}

	str->ctx = ctx;
	str->sizepolicy = sizepolicy;

	if (opt == VNEW_STRING) {
		if (VSTRA_initn(ctx, str, sz)) {
			return -1;
		}
		str->sizepolicy = sizepolicy;
	} else {
		if (str->buflen <= sz) {
			if (VSTRA_resize(str, sz)) {
				return -1;
			}
		}
	}

	{
		char *ret = strncpy(str->buffer, cstr, sz) + sz;
		*ret = '\0';
	}

	str->size = sz;

	return 0;
}

static int VSTRA_sncat_nocheck(VSTRA *dst, const char *src, size_t len )
{
	char *ret;

	if (dst->buflen <= len) {
		if (VSTRA_resize(dst, len)) {
			return -1;
		}
	}

	ret = strncpy(dst->buffer + dst->size, src, len) + len;
	*ret = '\0';

	dst->size = len;

	return 0;
}


V_EXPORT int VSTRA_initn( VCONTEXT *ctx, VSTRA *str, size_t sz)
{
	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	if (!ctx) {
		return -1;
	}

	str->ctx = ctx;
	str->sizepolicy = VRESIZE_init_multiply(2);
	
	str->buffer = 0;
	str->buflen = sz;
	str->size = 0;

	if (VSTRA_resize( str, sz )) {
		return -1;
	}

	str->buffer[0] = '\0';
	return 0;
}


V_EXPORT int VSTRA_resize( VSTRA *str, size_t reserved)
{
	char *ptr;

	if (!str->ctx) {
		if (reserved == str->buflen) {
			return 0;
		}
		return -1;
	}

	if (!reserved) {
		reserved = g_null_string_buf_size;
	} else {

		reserved  = VRESIZE_request(str->sizepolicy, str, reserved);
		if (!reserved) {
			return -1;
		}

	}

	ptr = (char *) V_REALLOC(str->ctx, str->buffer, reserved);
	if (!ptr) {
		return -1;
	}
	str->buffer = ptr;
	if ( str->size >= reserved) {
		str->buffer[ reserved-1 ] = '\0';
	}
	str->size = str->size < reserved - 1 ? str->size : reserved - 1;
	str->buflen = reserved;
	return 0;
}


V_EXPORT int VSTRA_sinit( VSTRA *dest, VCONTEXT *ctx, const char *cstr, size_t sz)
{
	size_t slen = strlen(cstr);
	return VSTRA_construct( VNEW_STRING, ctx, dest, cstr, slen < sz ? slen : sz, VRESIZE_init_multiply(2) );
}

V_EXPORT int VSTRA_sinitn( VSTRA *dest, VCONTEXT *ctx, const char *cstr)
{
	return VSTRA_construct( VNEW_STRING, ctx, dest, cstr, strlen(cstr), VRESIZE_init_multiply(2) );
}


V_EXPORT int VSTRA_cpy( VSTRA_INIT_OPTION opt, VSTRA *dest, VSTRA *src)
{
	if (opt == VNEW_STRING) {
	  return VSTRA_construct( opt, src->ctx, dest, src->buffer, src->size, src->sizepolicy);
	}
	VSTRA_set_zero(dest);
	return VSTRA_sncat_nocheck( dest, src->buffer, src->size);
	
}

V_EXPORT int VSTRA_scpy( VSTRA *dest, const char *cstr)
{
	VSTRA_set_zero(dest);
	return VSTRA_sncat_nocheck(  dest, cstr, strlen(cstr));
}

V_EXPORT int VSTRA_cat ( VSTRA *dst, VSTRA *src )
{
	return VSTRA_sncat_nocheck(  dst, src->buffer, src->size);
}

V_EXPORT int VSTRA_ncat ( VSTRA *dst, VSTRA *src, size_t len )
{
	return VSTRA_sncat_nocheck(  dst, src->buffer, src->size < len ? src->size : len );
} 

V_EXPORT int VSTRA_scat(VSTRA *dst, const char *src )
{
	return VSTRA_sncat_nocheck(  dst, src, strlen(src) + 1 );
}

V_EXPORT int VSTRA_sncat ( VSTRA *dst, const char *src, size_t len )
{
	size_t slen = strlen(src);

	return VSTRA_sncat_nocheck( dst, src, slen < len ? slen : len );
}

/* mid/left - report error on out of bound index or adapt to them ? */
V_EXPORT int VSTRA_left( VSTRA_INIT_OPTION opt, VSTRA *ret, VSTRA *str, size_t len )
{
	return VSTRA_construct( opt, str->ctx, ret,str->buffer, str->size < len ? str->size : len, str->sizepolicy );
}

V_EXPORT int VSTRA_right( VSTRA_INIT_OPTION opt, VSTRA *ret, VSTRA *str, size_t len )
{
	size_t offset;
	
	if (len < str->size) { 
		offset = str->size - len;
	} else {
		len = str->size;
		offset = 0;
	}

	return VSTRA_construct( opt, str->ctx, ret, str->buffer + offset, len, str->sizepolicy );
}

V_EXPORT int VSTRA_mid( VSTRA_INIT_OPTION opt, VSTRA *ret, VSTRA *str, size_t pos, size_t len )
{
	size_t offset;
	
	if (pos < str->size) { 
		offset = pos;
		len =  (str->size - offset) < len ? (str->size - offset) : len;
	} else {
		offset = 0;
		len = 0;
	}

	return VSTRA_construct( opt, str->ctx, ret, str->buffer + pos, len, str->sizepolicy );
}

V_EXPORT int VSTRA_fgetline( VSTRA *ret, FILE *fp, int eoflinechar)
{
	int ch;
	int rt = 0;

	VSTRA_set_zero(ret);

	if ( (ch = fgetc(fp)) != -1 ) {

		if (ch != eoflinechar) {
			do {
				rt = VSTRA_sncat(ret, (char *) &ch, 1);
			} while( (ch = fgetc(fp)) != -1  && ch != eoflinechar);
		}
	} else {
		rt = -1;
	}

	return rt;
}

V_EXPORT int VSTRA_fgets( VSTRA *ret, FILE *fp)
{
	int ch;
	int rt = 0;

	VSTRA_set_zero(ret);

	while( (ch = fgetc(fp)) != -1  && isspace(ch)) {
	}

	if (ch != -1) {
		while( (ch = fgetc(fp)) != -1  && !isspace(ch)) {
			rt = VSTRA_sncat(ret, (char *) &ch, 1);
		}
	}

	return rt;
}

#define READ_SIZE 1024

V_EXPORT int VSTRA_readfile( VSTRA *ret, FILE *fp)
{
	VSTRA buf;
	int  read;
	
	VSTRA_init_stack(&buf,READ_SIZE);

	VSTRA_set_zero(ret);

	while((read = fread(VSTRA_str(&buf),READ_SIZE,1,fp)) > 0) {
		if (VSTRA_sncat(ret, VSTRA_cstr(&buf), read)) {
			return -1;
		}
	}
	return 0;
}
