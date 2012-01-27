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

#ifndef _V_BUFFER_H_
#define _V_BUFFER_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>
#include <stdlib.h>
#include <string.h>

#define VBUFFER_RESIZE  (0x1)

/**
 * Fixed size data buffer for protocol data processing.
  
 * Protocol data header data is stripped or added, while the data item moves
 * through different protocol layers. 
 
 * If we want to prepend protocol headers, then we have to reserve some empty space at the start of data buffer;
 * so the start of data are is moved to the left as we add a header, and is moved to the right as we remove a header.
 * (Idea is used in handling of frame/packet data in struct sk_buff in Linux TCP/IP stack - but they probably didn't invent it either)

 * The buffer looks as follows: 

 * [---head room---{====data area====}---tail room---]

 * Head room := data from start of buffer up to data. this area does not contain any data.  
 * Data area := the data that is contained in this buffer; (current protocol data unit).
 * Tail room := room from end of data until end of buffer. this area does not contain any data.
 *
 */
typedef struct 
{
	VCONTEXT *ctx;
	int 	flags;
	
	V_UINT8 *buffer;		/** start of data block */
	size_t  elmmaxcount;	/** maximum number of elements */

	V_UINT8 *dataptr;		/** pointer to data area */
	size_t  elmcount;		/** size of data area */

}
	VBUFFER;


/**
 * @brief construct a buffer from given data area
 *
 * @param buf 
 * @param data
 * @param data_size
 * @param head_area_size
 */
V_INLINE int VBUFFER_init_from_data( VBUFFER *buf, unsigned char *data, size_t data_size, size_t head_area_size  )
{
	if (head_area_size >= data_size) {
		return -1;
	}

	buf->ctx  = 0;

	buf->buffer   = data;
	buf->elmmaxcount = data_size;

	buf->dataptr  = data + head_area_size;
	buf->elmcount = 0;
		
	buf->flags = 0;

	return 0;
}

/**
 * @brief construct a buffer and allocate memory buffer of given size.
 *
 * @param ctx
 * @param buf
 * @param size
 * @param head_area_size
 * @param is_dynamic 
 */
V_INLINE int VBUFFER_init( VCONTEXT *ctx, VBUFFER *buf, size_t size, size_t head_area_size, int is_dynamic )
{
	if (!ctx) {
		ctx = VCONTEXT_get_default_ctx();
	}

	buf->ctx   = ctx;

	if (head_area_size >= size) {
		return -1;
	}

	if ((buf->buffer = V_MALLOC( ctx, size )) == 0) {
		return -1;
	}
	buf->elmmaxcount = size;
	
	buf->dataptr  = buf->buffer + head_area_size;
	buf->elmcount = 0;

	
	buf->flags = (is_dynamic ? VBUFFER_RESIZE : 0);
	return 0;
}



/**
 * @brief Free all memory held by buffer (destructor).
 * @param buf (in) the object
 */
V_INLINE void VBUFFER_free( VBUFFER *buf )
{
	if (buf->buffer) {
		
		if (buf->ctx) {
			V_FREE( buf->ctx, buf->buffer );
		}
		buf->buffer = 0;
	}
}

/** @brief discard all data contained in buffer and set new head area size.
 *
 * @param buf (in)
 * @param head_area_size (in)
 */
V_INLINE int VBUFFER_reset_at( VBUFFER *buf, size_t head_area_size )
{
	if (head_area_size >= buf->elmmaxcount) {
		return -1;
	}
	buf->dataptr = buf->buffer + head_area_size;
	buf->elmcount = 0;

	return 0;
}

/*---*/

/** @brief returns data pointer
 */
V_INLINE V_UINT8 *VBUFFER_get_data_ptr( VBUFFER *buf)
{
	return buf->dataptr;
}

/** @brief returns pointer to the first byte after the end of the data area
 */
V_INLINE V_UINT8 *VBUFFER_get_data_eof( VBUFFER *buf)
{
	return buf->dataptr + buf->elmcount;
}

/** @brief returns number of bytes in data area
 */
V_INLINE size_t VBUFFER_get_data_size( VBUFFER *buf)
{
	return buf->elmcount;
}


/** @brief return number of bytes in this buffer (i.e. the sum of head area, data area, and tail area)
 */
V_INLINE size_t VBUFFER_buffer_size( VBUFFER *buf )
{
	return buf->elmmaxcount;
}

/* @brief returns number of bytes from start of buffer to start of data (head room)
 */
V_INLINE size_t VBUFFER_headroom( VBUFFER *buf )
{
	return buf->dataptr - buf->buffer;
}


/* @brief returns number of bytes from end of data buffer to end of buffer (tail room)
 */
V_INLINE size_t VBUFFER_tailroom( VBUFFER *buf )
{
	return buf->elmmaxcount - (VBUFFER_headroom(buf) + buf->elmcount);
}

/*---*/

/** @brief changes size of head area 
 *  The head area size is changed; data area is copied to the new location;
 *  This operation fails if the buffer is not large enough to hold all data, given the new head are size.
 */
V_INLINE int VBUFFER_set_headroom( VBUFFER *buf, size_t new_head_area_size) 
{
	size_t st = 0;
	
	if (buf->dataptr != buf->buffer) {
		st = VBUFFER_headroom(buf);

		if ((new_head_area_size + buf->elmcount) >= buf->elmmaxcount) {
			return -1;
		}

		if (buf->elmcount) {
			memmove( buf->buffer + new_head_area_size, buf->dataptr, buf->elmcount );
		}
		buf->dataptr = buf->buffer + new_head_area_size;
	}	
	return st;
}

/** @brief resizes a buffer; given that buffer is allocated by this object (VBUFFER_init), and not passed from outside (VBUFFER_init_from_data). 
 * The current buffer, given its current headroom have to fit into the new buffer size.
 */
V_INLINE int VBUFFER_resize( VBUFFER *buf, size_t size )
{
	V_UINT8 *data;
	size_t fsize;

	if (!buf->ctx) {
		return -1;
	}
	
	if (size < buf->elmcount) {
		return -1;
	}

	if ((VBUFFER_headroom( buf ) + buf->elmcount) > size) {
		return -1;
	}

	fsize = VBUFFER_headroom( buf );

	data = V_REALLOC( buf->ctx, buf->buffer, size);
	if (!data) {
		return -1;
	}

	buf->buffer = data;
	buf->dataptr = data + fsize;
	
	return 0;
}


/*
 * @brief changes size reserved by buffer area. 
 * If the new buffer is not large enough to hold the data at the given head area size, then we
 * resize the buffer if possible.
 */
V_INLINE int VBUFFER_data_set_size( VBUFFER *buf, size_t size )
{
	size_t fsize;

	fsize = VBUFFER_headroom( buf );

	if ( (fsize + size) > buf->elmmaxcount) {

		if (buf->flags & VBUFFER_RESIZE) {
			if (VBUFFER_resize(buf, fsize + size)) {
				return -1;
			}
		} else {
			return -1;
		}
	}

	buf->elmcount = size;
	return 0;
}

/*---*/


/*---*/

/**
 * @brief add some data to the end of the buffer
 * If the new buffer is not large enough to hold the data at the given head area size, then we
 * resize the buffer if possible.
 */
V_INLINE int VBUFFER_add_data( VBUFFER *buf, void *data, size_t data_size )
{
	size_t eof = VBUFFER_headroom(buf) + buf->elmcount;

	if ((eof + data_size) > buf->elmmaxcount) {
		
		if (buf->flags & VBUFFER_RESIZE) {
			if (VBUFFER_resize(buf, eof + data_size)) {
				return -1;
			}
		} else {
			return -1;
		}
	}

	
	memcpy( buf->buffer + eof, data, data_size );
	buf->elmcount += data_size;

	return 0;
}



/*---*/


/** @brief check consistency of buffer
 */
V_INLINE int VBUFFER_check( VBUFFER *buf)
{
	if (!buf->dataptr || !buf->buffer) {
		return 0;
	}

	if (buf->dataptr < buf->buffer) {
		return 0;
	}

	if (buf->dataptr > (buf->buffer + buf->elmmaxcount))  {
		return 0;
	}

	if ((buf->dataptr + buf->elmcount) > (buf->buffer + buf->elmmaxcount)) {
		return 0;
	}

	if (buf->elmcount > buf->elmmaxcount) {
		return 0;
	}

	return 1;
}


#ifdef  __cplusplus
}
#endif

#endif



