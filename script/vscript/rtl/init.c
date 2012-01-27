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
#include <rtl.h>




static VSCRIPTXLIB_METHOD_CALLBACK_TABLE init_callback[] = {

//general functions 

	{ 1,  "print",  "s:s",		std_print },

	{ 2,  "length", "s:s",		std_length },

	{ 22,  "exit",  "s:s",		std_exit },	

	//function on hashes
	{ 3,  "keys",   "@:%",		std_keys },

	{ 3,  "values",	"@:%",		std_values },

//function on arrays
	{ 3,  "push",	"s:@,!",	std_push },

	{ 3,  "pop",	"!:@",		std_pop },

	{ 3,  "std_join", "s:@",	std_join },

//function on strings
	{ 4,  "index",  "s:s,s",		std_index },
	
	{ 5,  "rindex", "s:s,s",		std_rindex },
	
	{ 6,  "substr", "s:s,s,s",	std_substr },

	{ 7,  "substr", "s:s,s",	std_substr },

//debug functions
	{ 8,  "stack_trace", "s:",    std_strace },
	
	{ 9,  "trace", "s:s",			std_trace },	
	
//functions on numbers
	{ 10,	"abs",	 "s:s",			std_abs },

	{ 11,	"atan2", "s:s",			std_atan2 },

	{ 12,	"cos",	 "s:s",			std_cos },

	{ 13,	"exp",	 "s:s",			std_exp },

	{ 14,	"log",	 "s:s",			std_log },	

	{ 15,	"sin",	 "s:s",			std_sin },	

	{ 16,	"sqrt",	 "s:s",			std_sqrt },	

	{ 17,	"oct",	 "s:s",			std_oct },	
	
	{ 18,	"hex",	 "s:s",			std_hex },	
	
	{ 19,	"int",	 "s:s",			std_int },	
	
	{ 20,	"rand",	 "s:s",			std_rand },	
	
	{ 21,	"srand", "s:s",			std_srand },	
	

	{ -1, 0, 0, 0 }
};


V_EXPORT int RTL_register(VSCRIPTXLIB *ctx)
{
	VSCRIPTXLIB_add_xmethod_callback_table(ctx, init_callback );
	return 0;

}

