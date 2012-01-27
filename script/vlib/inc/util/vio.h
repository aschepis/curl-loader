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

#ifndef _VIO_H_
#define _VIO_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <util/vcircbuf.h>
#include <util/vstra.h>

struct tagVIO;

typedef int (*VIO_RW_FUNC) (struct tagVIO *, void *buf, size_t len);
typedef int (*VIO_IOCTL_FUNC) (struct tagVIO *, int cmd, void *prm);
typedef int (*VIO_CLOSE_FUNC) (struct tagVIO *);  

/*
	Current requirement of IO interface
		- good for parsing
		- good for object serializaiton
		- networking ? KuKu? 
*/


/**
 * IO base interface
 */
typedef struct  {
	VIO_RW_FUNC		read;
	VIO_RW_FUNC		write;
	VIO_IOCTL_FUNC  ioctl;
	VIO_CLOSE_FUNC	close;
} VIO;


/**
 * File IO interface
 */
typedef struct  {
	VIO base;
	FILE *fp;
} VIOFILE;

int VIOFILE_open( VIOFILE *vio, const char *file, const char *mode);


/**
 * IO interface to memory buffer
 */
typedef struct  {
	VIO base;
	VCIRCBUF buff;
} VIOMEM;

V_EXPORT int VIOMEM_init_fixed( VIOMEM *vio, void *buf, size_t size);
V_EXPORT int VIOMEM_init( VCONTEXT *ctx, VIOMEM *vio, size_t init_buf_size, int is_fixed );


/**
 * Access IO interface functions
 */
#define VIO_read( vio, buf, len)	((VIO *) (vio))->read( (vio), (buf), (len))
#define VIO_write( vio, buf, len)	((VIO *) (vio))->write( (vio), (buf), (len))
#define VIO_ioctl( vio, cmd, prm)	((VIO *) (vio))->ioctl( (vio), (cmd), (prm))
#define VIO_close( viod )			((VIO *) (vio))->close( (vio) )


typedef struct  {
	VIO			base;
	VIO			*next;
	VCIRCBUF	buff;
	size_t		readpos;
	size_t		mark;

}	VIOBT;

V_EXPORT int VIOBT_init( VCONTEXT *ctx, VIOBT *bt, VIO *impl, size_t init_buf_size, int is_fixed );

V_EXPORT int VIOBT_getchar( VIOBT *bt );

V_EXPORT int VIOBT_ungetchar( VIOBT *bt );

V_INLINE int VIOBT_get_mark( VIOBT *bt )
{
	return bt->mark = bt->readpos;
}

V_INLINE void VIOBT_unget( VIOBT *bt, int mark )
{
	bt->buff.start = mark;
	bt->mark = (size_t) -1;

}

V_EXPORT int VIOBT_parse_skip_spaces(VIOBT *in);

V_EXPORT int VIOBT_parse_keyword(VIOBT *in, const char *str);

V_EXPORT int VIOBT_parse_keywords(VIOBT *in, const char *keywordArray[]);

V_EXPORT int VIOBT_parse_string_constant(VIOBT *in, VSTRA *ret);

V_EXPORT int VIOBT_parse_id(VIOBT *in, VSTRA *ret);

V_EXPORT int VIOBT_parse_double(VIOBT *in, double *ret);

#ifdef  __cplusplus
}
#endif

#endif
