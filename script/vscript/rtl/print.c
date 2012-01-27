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

#include <vscript.h>
#include "rtlapi.h"
#include <stdio.h>
#include <vm.h>


/*
 * print to standard output.
 */
int std_print(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	int i;

	V_UNUSED(method_id);

	for( i = 0; i < params->param_count; i++) {

		char sSize[100];
		int  slen;
		const char *ret;
		int   is_trace_on;
		
		is_trace_on = ((VSCRIPTVM *) ctx)->trace_flag;
		
		ret = VM_SCALAR_to_string(ctx, params->param[i], sSize, &slen);
		
		if (is_trace_on) {
			fputs( "\n>", stdout);
		}
		
		fputs( ret , stdout);

		if (is_trace_on) {
			fputs( "\n", stdout);
		}
	}
	
	params->retval = VM_VALUE_LONG_return(ctx, 1);

	return 0;
}
