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

#include <util/vio.h>
#include <ctype.h>

int VIOFILE_read(struct tagVIO *vio, void *buf, size_t len)
{
	return fread(buf,1, len, ((VIOFILE *) vio)->fp);
}

int VIOFILE_write(struct tagVIO *vio, void *buf, size_t len)
{
	return fwrite(buf,1, len, ((VIOFILE *) vio)->fp);
}

int VIOFILE_ioctl(struct tagVIO *vio, int cmd, void *prm)
{
	V_UNUSED(vio);
	V_UNUSED(cmd);
	V_UNUSED(prm);

	return -1;
}

int VIOFILE_close(struct tagVIO * vio)
{
	return fclose( ((VIOFILE *) vio)->fp );
}


int VIOFILE_open( VIOFILE *vio, const char *file, const char *mode)
{
	vio->base.read  = VIOFILE_read;
	vio->base.write = VIOFILE_write;
	vio->base.ioctl = VIOFILE_ioctl;
	vio->base.close = VIOFILE_close;

	if ( (((VIOFILE *)vio)->fp = fopen( file, mode)) == 0) {
		return -1;
	}
	return 0;
}


V_EXPORT int VIOMEM_read(struct tagVIO *arg, void *buf, size_t len)
{
	VIOMEM *v = (VIOMEM *) arg;
	return VCIRCBUF_popn( &v->buff, buf, len );
}

V_EXPORT int VIOMEM_write(struct tagVIO *arg, void *buf, size_t len)
{
	VIOMEM *v = (VIOMEM *) arg;
	return VCIRCBUF_pushn( &v->buff, buf, len );
}

V_EXPORT int VIOMEM_ioctl(struct tagVIO *arg, int cmd, void *prm)
{
	V_UNUSED(arg);
	V_UNUSED(cmd);
	V_UNUSED(prm);

	return -1;
}

V_EXPORT int VIOMEM_close(struct tagVIO * vio)
{
	VCIRCBUF_free( & ((VIOMEM *) vio)->buff );
	return 0;
}

/**
 * create io buffer to fixed size block
 */
V_EXPORT int VIOMEM_init_fixed( VIOMEM *vio, void *buf, size_t size)
{
	vio->base.read  = VIOMEM_read;
	vio->base.write = VIOMEM_write;
	vio->base.ioctl = VIOMEM_ioctl;
	vio->base.close = VIOMEM_close;

	return VCIRCBUF_init_fixed( & ((VIOMEM *) vio)->buff, 1, buf, size, 0, 0, 0 );
}


V_EXPORT int VIOMEM_init( VCONTEXT *ctx, VIOMEM *vio, size_t init_buf_size, int is_fixed )
{
	vio->base.read  = VIOMEM_read;
	vio->base.write = VIOMEM_write;
	vio->base.ioctl = VIOMEM_ioctl;
	vio->base.close = VIOMEM_close;

	return VCIRCBUF_init( ctx, & ((VIOMEM *) vio)->buff, 1, init_buf_size, is_fixed, 0, 0, 0 );
}


V_EXPORT int VIOBT_read(struct tagVIO *arg, void *buf, size_t len)
{
	VIOBT *v = (VIOBT *) arg;
	return v->next->read( arg, buf, len );
}

V_EXPORT int VIOBT_write(struct tagVIO *arg, void *buf, size_t len)
{
	VIOBT *v = (VIOBT *) arg;
	return v->next->write( arg, buf, len );
}

V_EXPORT int VIOBT_ioctl(struct tagVIO *arg, int cmd, void *prm)
{
	VIOBT *v = (VIOBT *) arg;
	return v->next->ioctl( arg, cmd, prm );
}

V_EXPORT int VIOBT_close(struct tagVIO * arg)
{
	VIOBT *v = (VIOBT *) arg;
	return v->next->close( arg );
}


V_EXPORT int VIOBT_init( VCONTEXT *ctx, VIOBT *bt, VIO *impl, size_t init_buf_size, int is_fixed )
{
	V_UNUSED(impl);

	bt->base.read  = VIOBT_read;
	bt->base.write = VIOBT_write;
	bt->base.ioctl = VIOBT_ioctl;
	bt->base.close = VIOBT_close;

	bt->readpos = 0;
	bt->mark = (size_t) -1;
	
	return VCIRCBUF_init( ctx, &bt->buff, 1, init_buf_size, is_fixed, 0, 0, 0 );
}

int VIOBT_get_more( VIOBT *bt )
{
	if (! VCIRCBUF_isfull( &bt->buff)) {
		
		if (bt->buff.start > bt->buff.end) {
			

		} else {

		}

	} else  {
		/* grow the buffer */

	}

	return 0;
}

V_EXPORT int VIOBT_getchar( VIOBT *bt )
{	
	V_UINT32 next,nextstart,ret;
	
	next = bt->readpos + 1;
	if (next >= bt->buff.elmmaxcount) {
		next = bt->buff.elmmaxcount;
	}

	if (next != bt->buff.end) {

		nextstart = bt->buff.start;
		if (nextstart != bt->mark) {
			nextstart += 1;
			if (nextstart >= bt->buff.elmmaxcount) {
				nextstart = 0;
			}
			bt->buff.start = nextstart;
		}

		ret = bt->buff.buffer[ next ];
		bt->readpos = next;
		return ret;
	} 
	   
	if (VIOBT_get_more( bt )) {
		return -1;
	}
	return VIOBT_getchar( bt );

}


V_EXPORT int VIOBT_ungetchar( VIOBT *bt )
{
	V_UINT32 next;
	
	next = bt->readpos;

	if (next == bt->buff.start) {
		return -1;
	}
	
	next -= 1;
	if (next <= 0) {
	  next = bt->buff.elmmaxcount - 1;
	}
	bt->readpos = next;

	return 0;
}

V_EXPORT int VIOBT_parse_skip_spaces(VIOBT *in)
{
	int ch;

	while( (ch = VIOBT_getchar( in )) < 0 ) {
		if (!isalpha(ch)) {
			VIOBT_ungetchar( in );
			return 0;
		}
	}
	return ch;
}		


V_EXPORT int VIOBT_parse_keyword(VIOBT *in, const char *str)
{
	int mark;

	VIOBT_parse_skip_spaces( in );

	mark = VIOBT_get_mark( in );
	for(;*str != '\0'; str++) {
		if (*str != VIOBT_getchar(in)) {
			VIOBT_unget( in, mark);
			return -1;
		}
	}
	return 0;

}


V_EXPORT int VIOBT_parse_keywords(VIOBT *in, const char *keywordArray[])
{
	int i;

	for(i = 0; keywordArray[ i ] != '\0'; i++) {
		if (VIOBT_parse_keyword( in, keywordArray[i] ) ) {
			return i;
		}
	}
	return -1;
}

V_EXPORT int VIOBT_parse_string_constant(VIOBT *in, VSTRA *ret)
{
	int ch;

	VIOBT_parse_skip_spaces( in );
	
	ch = VIOBT_getchar(in);
	if (ch == '"') {
		VSTRA_set_zero( ret );
		
		do {

			ch = VIOBT_getchar(in);
			if (ch == '"') {
				break;
			}

			if (ch == '\\') {
				ch = VIOBT_getchar(in);
				switch(ch) {
				case '"':
				case '\\':
				case '/':
					break;
				case 'b':
					ch = 8;
					break;
				case 'f':
					ch = 12;
					break;
				case 'n':
					ch = 10;
					break;
				case 'r':
					ch = 13;
					break;
				case 't':
					ch = 9;
					break;
				default:
					;
				}

				if (ch == 'u') {

					V_UINT8 by[4];
					int i;

					for(i=0;i<4;i++) {
						ch = VIOBT_getchar(in);
						if (ch == -1) {
							return -1;
						}
						by[i] = (unsigned char) ch;
					}

					VSTRA_sncat(ret, (char *) &by, sizeof(by));
					continue;
				}
			}

			VSTRA_sncat(ret, (char *) &ch, 1);

		} while(1);

	} 

	VIOBT_ungetchar(in);
	return -1;
}


V_EXPORT int VIOBT_parse_id(VIOBT *in, VSTRA *ret)
{
	int ch;
	
	VIOBT_parse_skip_spaces( in );


	ch = VIOBT_getchar(in);
	if (isalpha(ch)) {

		VSTRA_set_zero( ret );

		do {

			ch = VIOBT_getchar(in);
			if (ch == '-' || ch == '_' || isalnum(ch)) {
				VSTRA_sncat(ret, (char *) &ch, 1);
			} else {
				VIOBT_ungetchar(in);
				break;
			}

		} while(1);


		return 1;
	}
	
	VIOBT_ungetchar(in);
	return -1;
}


double make_double(int int_sign, V_UINT32 int_part, V_UINT32 fraction_part, int exponent_sign, V_UINT32 exponent_part)
{
	double ret;

	ret  = (1 / (double) fraction_part);
	ret += int_sign * int_part;
	ret *= 10 ^ (exponent_sign * exponent_part);

	return ret;
}

V_EXPORT int VIOBT_parse_double(VIOBT *in, double *ret)
{
	int ch = 0, mark, int_sign = 1, exponent_sign = 1;
	V_UINT32 int_part = 0, fraction_part = 0, exponent_part = 0;
		
	VIOBT_parse_skip_spaces( in );

	mark = VIOBT_get_mark( in );

	if (VIOBT_getchar(in) == '-') {
		int_sign = -1;
	} else {
		ch = VIOBT_getchar(in);
	}

	if (ch != '0') {

		if (isdigit(ch)) {
		
			do {
				
				ch = VIOBT_getchar(in);
				if (!isdigit(ch)) {
					break;
				}
				int_part = ch + (int_part * 10);

			} while( 1 );

		} else {
			VIOBT_unget( in, mark);
			return -1;
		}
	}

	if (ch == '.') {

		do {
			
			ch = VIOBT_getchar(in);
			if (!isdigit(ch)) {
				break;
			}
			fraction_part = ch + (fraction_part * 10);

		} while( 1 );	}

	if (ch == 'e' || ch == 'E') {

		ch = VIOBT_getchar(in);
		if (ch == '+') {

		} else if (ch == '-') {
			exponent_sign = -1;
		} else {
			
		}

		do {

			ch = VIOBT_getchar(in);
			if (!isdigit(ch)) {
				break;
			}
			exponent_part = ch + (exponent_part * 10);

		} while( 1 );	

	} else {
		VIOBT_unget( in, mark);
		return -1;
	}
	
	
	*ret = make_double(int_sign, int_part, fraction_part, exponent_sign, exponent_part);
	return 0;

}

