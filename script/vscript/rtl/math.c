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
#include <math.h>
#include <stdlib.h>

int std_abs(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	VM_BASE_TYPE  var_type;

	V_UNUSED(method_id);
	
	var_type = VM_TYPE_get_base_type( params->param[0] );
	if (VM_IS_SCALAR(var_type) == VM_LONG) {
		
		long val = VM_SCALAR_to_long(ctx, params->param[0]);
		if (val < 0) {
			val = - val;
		}
		params->retval = VM_VALUE_DOUBLE_return(ctx,val);
	
	} else {

		double val = VM_SCALAR_to_double(ctx, params->param[0]);
		
		if (val < 0) {
			val = -val;
		}
		
		params->retval = VM_VALUE_DOUBLE_return( ctx, val );
	}
	return 0;
}

int std_atan2(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	V_UNUSED(method_id);
	V_UNUSED(ctx);
	V_UNUSED(params);
    
	return 0;
}

int std_cos(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	double val = VM_SCALAR_to_double(ctx, params->param[0]);

	double ret = cos( val );

	V_UNUSED(method_id);

	params->retval = VM_VALUE_DOUBLE_return( ctx, ret );

	return 0;
}

int std_exp(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	double val = VM_SCALAR_to_double(ctx, params->param[0]);

	double ret = exp( val );

	V_UNUSED(method_id);

	params->retval =  VM_VALUE_DOUBLE_return( ctx, ret );

	return 0;
}

int std_log(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	double val = VM_SCALAR_to_double(ctx, params->param[0]);

	double ret = log( val );

	
	V_UNUSED(method_id);
		
	params->retval =VM_VALUE_DOUBLE_return( ctx, ret );

	return 0;
}

int std_sin(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	double val = VM_SCALAR_to_double(ctx, params->param[0]);

	double ret = sin( val );

	V_UNUSED(method_id);
		
	
	params->retval = VM_VALUE_DOUBLE_return( ctx, ret );

	return 0;
}

int std_sqrt(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	double val = VM_SCALAR_to_double(ctx, params->param[0]);

	double ret = sqrt( val );

	V_UNUSED(method_id);

	params->retval = VM_VALUE_DOUBLE_return( ctx, ret );

	return 0;
}

int std_oct(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	char sSize[100];
	int  slen;
	const char *ret;
	long lval;
	
	V_UNUSED(method_id);
			
	
	ret = VM_SCALAR_to_string(ctx, params->param[0], sSize, &slen);
	
	lval = strtol( ret, 0, 8 );
	
	params->retval = VM_VALUE_LONG_return( ctx, lval );

	return 0;
}	

int std_hex(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	char sSize[100];
	int  slen;
	const char *ret;
	long lval;


	V_UNUSED(method_id);
	
	ret = VM_SCALAR_to_string(ctx, params->param[0], sSize, &slen);
	
	lval = strtol( ret, 0, 16 );
	
	params->retval = VM_VALUE_LONG_return( ctx, lval );

	return 0;
}	

int std_int(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	V_UNUSED(method_id);
	V_UNUSED(ctx);
	V_UNUSED(params);
 	return 0;
}	

int std_rand(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{

	V_UNUSED(method_id);
	
	if (params->param_count == 0) {
		params->retval = VM_VALUE_LONG_return( ctx, rand() );
	} else {
		double val; 
		double ret;

		val = VM_SCALAR_to_double(ctx, params->param[0]);
		
		if (val == 0) {
			return 1;
		}

		ret = rand(  );

		params->retval = VM_VALUE_DOUBLE_return( ctx, ret / val );
	}
	return 0;
}
	
int std_srand(void *ctx, int method_id, VSCRIPT_XMETHOD_FRAME *params)
{
	long val = VM_SCALAR_to_long(ctx, params->param[0]);

	V_UNUSED(method_id);
	
	srand( val );

	params->retval = VM_VALUE_LONG_return( ctx, 0 );
	
	return 0;
}
