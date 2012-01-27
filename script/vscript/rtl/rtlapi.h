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
#ifndef __RTLAPI_H__
#define __RTLAPI_H__

/*
 * print to standard output.
 */
int std_push(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_pop(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_print(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_length(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_keys(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_values(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_join(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_index(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_rindex(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_substr(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params); 

int std_strace(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_trace(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_abs(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_atan2(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_cos(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_exp(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_log(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_sin(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_sqrt(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_oct(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);
	
int std_hex(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);
	
int std_int(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);
	
int std_rand(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);
	
int std_srand(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

int std_exit(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params);

#endif


