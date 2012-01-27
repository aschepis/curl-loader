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

#ifndef _VCIRCULARBUFFER_H_
#define _VCIRCULARBUFFER_H_

#ifdef  __cplusplus
extern "C" {
#endif

#include <util/vbasedefs.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

struct tagVCIRCBUF;

typedef void (*VCIRCBUF_event) (struct tagVCIRCBUF *);

/** 
 * @brief Constructs an empty circular buffer object (also known as Ring buffer); Each element of the buffer is of the same size.
 * Bounded ciruclar buffer queue structure, implemented as an array.
 * All elements kept in this structure are of the same size.
 *
 * While the term "circular" is figurative, it alludes to the rotation through the buffer of the positions where the next data 
 * will be read and written. When moving through the buffer, the writer moves forward one step each time it writes, and when it 
 * passes the end of the buffer it starts again at the beginning. The reader moves through the buffer in the same way. As long 
 * as the reader is as fast as or faster than the writer (which is usually the case), the buffer acts as a queue of the next 
 * data to be processed. If the writer is faster than the reader, the buffer can fill up.
 * (Quoted from from http://en.wikipedia.org/wiki/Circular_buffer )
 */
typedef struct tagVCIRCBUF
{
  VCONTEXT	*ctx;

  size_t	elmsize;
  size_t	elmmaxcount;
  size_t	elmcount;

  size_t	start, end;
  
  V_UINT8	*buffer;
  int	    is_fixed;

  VCIRCBUF_event on_empty_event;
  VCIRCBUF_event on_full_event;
  VCIRCBUF_event on_new_data_event;

} VCIRCBUF;



/*
 * @brief constructor of empty circular buffer object 
 *
 * Memory for data area of buffer is allocated from allocator interface; if interface is NULL then global allcoator is used.
 * Objects created in this constructor may be resized with VCIRCBUF_resize.
 *
 * @param context (in) allocator interface that is used to get memory for buffer. (if null we are using the default allocator).
 * @param buf (in) the object
 * @param elmsize (in) size of one element; all elements in this structure are of the same size.
 * @param numofelem (in) maximum number of elements contained in this structure.
 * @param is_fixed (in) if false then this buffer will be resized if we push more elements then given by numofelem parameter.
 * @param on_empty_event (in) optional - if given then this callback will be called when buffer reaches empty state.
 * @param on_full_event (in) optional - if given then this callback will be called when buffer has reached full state
 */
V_INLINE int VCIRCBUF_init( VCONTEXT *ctx, VCIRCBUF *buf, size_t elmsize, size_t numofelem,
						    int is_fixed,
							VCIRCBUF_event on_empty_event, 
							VCIRCBUF_event on_full_event,
							VCIRCBUF_event on_new_data_event)
{
  if (!ctx) {
	ctx = VCONTEXT_get_default_ctx();
  }

  if (!ctx) {
	return -1;
  }

  if ((buf->buffer = (V_UINT8 *) V_MALLOC( ctx, elmsize * numofelem )) == 0) {
	return -1;
  }

  buf->ctx = ctx;

  buf->elmsize  = elmsize;
  buf->elmmaxcount = numofelem;
  buf->is_fixed = is_fixed;

  buf->start = 0;
  buf->end = 0;
  buf->elmcount = 0;

  buf->on_empty_event = on_empty_event;
  buf->on_full_event  = on_full_event;
  buf->on_new_data_event = on_new_data_event;

  if (on_empty_event) {
	on_empty_event(buf);
  }

  return 0;
}


/**
 * @brief constructor of circular buffer object
 *
 * Memory for data area of is supplied by the user as argument values.
 * Objects created in this constructor may not be resized with VCIRCBUF_resize.
 *
 * @param context (in) allocator interface that is used to get memory for buffer. (if null we are using the default allocator).
 * @param ptr (in) pointer to data area supplied by the user.
 * @param bytes (in) size of data area supplied by the user. 
 * @param on_empty_event (in) optional - if given then this callback will be called when buffer reaches empty state.
 * @param on_full_event (in) optional - if given then this callback will be called when buffer has reached full state
 */
V_INLINE int VCIRCBUF_init_fixed( VCIRCBUF *buf, size_t elmsize, void *ptr, size_t bytes,
								  VCIRCBUF_event on_empty_event, 
								  VCIRCBUF_event on_full_event,
								  VCIRCBUF_event on_new_data_event)
{
  buf->ctx = 0;
  buf->buffer = ptr;
  buf->is_fixed = V_TRUE;

  buf->elmsize  = elmsize;
  buf->elmmaxcount = bytes / elmsize;

  buf->start = 0;
  buf->end = 0;
  buf->elmcount = 0;


  buf->on_empty_event = on_empty_event;
  buf->on_full_event  = on_full_event;
  buf->on_new_data_event = on_new_data_event;

  return 0;
}


/** 
 * @brief Free all memory held by circular buffer (destructor).
 * @param buf (in) the object
 */
V_INLINE void VCIRCBUF_free( VCIRCBUF *buf)
{
	if (buf->ctx && buf->buffer) {
		V_FREE( buf->ctx, buf->buffer );
		buf->buffer = 0;
	}
}


V_EXPORT int VCIRCBUF_resize( VCIRCBUF *buf, size_t new_numofelem);



/**
 * @brief returns TRUE if circular buffer is full
 */
V_INLINE V_BOOLEAN VCIRCBUF_isfull( VCIRCBUF *buf )
{
  return buf->elmcount == buf->elmmaxcount;
}

/**
 * @brief returns TRUE if circular buffer is empty
 */
V_INLINE V_BOOLEAN VCIRCBUF_isempty( VCIRCBUF *buf )
{
  return buf->elmcount == 0;
}

V_INLINE size_t VCIRCBUF_size( VCIRCBUF *buf )
{
  return buf->elmcount;
}

V_INLINE size_t VCIRCBUF_maxsize( VCIRCBUF *buf )
{
  return buf->elmmaxcount;
}

V_INLINE size_t VCIRCBUF_size_available( VCIRCBUF *buf )
{
  return buf->elmmaxcount - buf->elmcount;
}


/**
 * @brief add one new element into the circualar buffer
 * @param buf  (in) the object
 * @param elm  (in) pointer to element.
 * @param size (in) size of element (must be equal to size of element received in intializer of object)
 */
V_INLINE int VCIRCBUF_push( VCIRCBUF *buf,void *elm, size_t elm_size )
{
	if (VCIRCBUF_isfull(buf)) { 
		if (buf->is_fixed) {
			return -1;
		}
		if (buf->on_full_event) { 
			buf->on_full_event(buf);
		}

		if (VCIRCBUF_resize( buf, buf->elmcount + 1)) {
			return -1;
		}
	}

	if (elm_size != buf->elmsize) {
		return -1;
	}

	memcpy(buf->buffer + buf->end * buf->elmsize, elm, buf->elmsize );
	buf->end = (buf->end + 1) % buf->elmmaxcount;

	buf->elmcount ++;

	if (buf->on_new_data_event) {
		buf->on_new_data_event(buf);
	}

	if (buf->on_full_event && VCIRCBUF_isfull(buf)) { 
		buf->on_full_event(buf);
	}

	return 0;
}

/**
 * @brief returns pointer to reader side element.
 * @param buf (in) the object
 * @param elmsize (out) 
 * @returns pointer to top element (read size) element of circular buffer
 */
V_INLINE void * VCIRCBUF_top( VCIRCBUF *buf,size_t *elm_size  )
{
	if (VCIRCBUF_isempty(buf)) { 
		return 0;
	}
	*elm_size = buf->elmsize;
	return buf->buffer + buf->start * buf->elmsize;
}


/**
 * @brief pop the top element of the circular buffer, optionally copies the top element to user supplied buffer.
 * @param buf (in) the object
 * @param elm (in|out) user supplied buffer to copy top element into (NULL if we don't wish to copy the top element)
 * @param elm_size (in) if elm != NULL then this is the size in bytes of the user supplied buffer (argument elm)
 * @return -1 if buffer is empty, or user supplied buffer was too small.
 */
V_INLINE int VCIRCBUF_pop( VCIRCBUF *buf,void *elm, size_t elm_size  )
{
	if (VCIRCBUF_isempty(buf)) { 
		return -1;
	}

	if (elm) {
		if (elm_size < buf->elmsize) {
			return -1;
		}
		memcpy( elm, buf->buffer + buf->start * buf->elmsize, buf->elmsize);
	} 

	buf->start = (buf->start + 1) % buf->elmmaxcount;
	buf->elmcount --;

	if (buf->on_empty_event && VCIRCBUF_isempty(buf)) { 
		buf->on_empty_event(buf);
	}

	return 0;
}

V_INLINE int VCIRCBUF_popn( VCIRCBUF *cbuf,void *buf, size_t bufsize  )
{
	size_t elm;
	size_t bufelm;

	if (VCIRCBUF_isempty(cbuf)) { 
		return -1;
	}

	bufelm =  bufsize / cbuf->elmsize; 
	elm =  cbuf->elmcount < bufelm ? cbuf->elmcount : bufelm;

	if (cbuf->start < cbuf->end) {
		memcpy( buf, cbuf->buffer + cbuf->start * cbuf->elmsize, elm * cbuf->elmsize); 
		cbuf->start += elm;
	} else {
		size_t n1 = (cbuf->end - cbuf->start) / cbuf->elmsize;

		memcpy(buf, cbuf->buffer + cbuf->start * cbuf->elmsize, n1 * cbuf->elmsize);
		memcpy((char *) buf + n1 * cbuf->elmsize, cbuf->buffer, (elm - n1) * cbuf->elmsize);
		cbuf->start = elm - n1;
	}

	cbuf->elmcount -= elm;

	if (cbuf->on_empty_event && VCIRCBUF_isempty(cbuf)) { 
		cbuf->on_empty_event(buf);
	}

	return elm;
}

V_INLINE int VCIRCBUF_pushn( VCIRCBUF *cbuf,void *buf, size_t bufsize  )
{
	size_t elm;

	elm = bufsize / cbuf->elmsize;
	if (VCIRCBUF_size_available(cbuf) <   elm) {

		if (cbuf->is_fixed) {
			return -1;
		}

		if (cbuf->on_full_event) { 
			cbuf->on_full_event(buf);
		}

		if (VCIRCBUF_resize( cbuf, cbuf->elmcount + VCIRCBUF_size_available(cbuf) - elm)) {
			return -1;
		}
		return -1;
	}

	elm =  cbuf->elmcount < elm ? cbuf->elmcount : elm;

	if (cbuf->start > cbuf->end) {
		memcpy( cbuf->buffer + cbuf->end * cbuf->elmsize, buf, elm * cbuf->elmsize); 
		cbuf->end += elm;
	} else {
		size_t n1 = (cbuf->end - cbuf->end) / cbuf->elmsize;

		memcpy(cbuf->buffer + cbuf->end * cbuf->elmsize, buf, n1 * cbuf->elmsize);
		memcpy(cbuf->buffer,(char *) buf + n1 * cbuf->elmsize, (elm - n1) * cbuf->elmsize);
		cbuf->end = n1;
	}

	cbuf->elmcount += elm;

	if (cbuf->on_full_event && VCIRCBUF_isfull(cbuf)) { 
		cbuf->on_full_event(buf);
	}


	return elm;
}

/** 
 * @brief checks consistency of this circular buffer structure.
 * @parma buf (in) the object.
 */
V_INLINE int VCIRCBUF_check( VCIRCBUF *buf )
{
	return (buf->start + buf->elmcount) % buf->elmmaxcount == buf->end;
}


#ifdef  __cplusplus
}
#endif

#endif




