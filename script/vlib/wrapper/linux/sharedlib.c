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

#include <wrapper/vwrapper.h>
#include <dlfcn.h>

V_EXPORT VOS_SOLIB * VOS_SOLIB_load(const char *path )
{
	void *hLib = dlopen( path, RTLD_NOW );
	return (VOS_SOLIB *) hLib;

}


V_EXPORT void VOS_SOLIB_close(VOS_SOLIB *hLib)
{
	dlclose( (void *) hLib );
}


//typedef void (*FUN_PTR) (void);

V_EXPORT FUN_PTR VOS_SOLIB_get_proc_address(VOS_SOLIB *hLib, const char *fname)
{
	return (FUN_PTR) dlsym( (void *) hLib, fname );
}

