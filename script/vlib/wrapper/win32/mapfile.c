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
#include <windows.h>


typedef struct tagMAPFILE_IMP {
	HANDLE file;
	HANDLE mapping;
	void  *addr;
	V_UINT32 length_low;
	V_UINT32 length_high;
}
	MAPFILE_IMP;



static struct { 
	unsigned int flag; 
	unsigned int mode;
	unsigned int map_mode;
} open_mode[] = {
	VOS_MAPPING_READ, PAGE_READONLY, FILE_MAP_READ,
	(VOS_MAPPING_READ|VOS_MAPPING_WRITE), PAGE_READWRITE, FILE_MAP_WRITE,
	(VOS_MAPPING_READ|VOS_MAPPING_EXEC),  PAGE_EXECUTE_READ, FILE_MAP_READ,
	(VOS_MAPPING_READ|VOS_MAPPING_WRITE|VOS_MAPPING_EXEC), PAGE_EXECUTE_READWRITE, FILE_MAP_WRITE,
	0,0,0
};	



V_EXPORT VOS_MAPFILE VOS_MAPFILE_open(const char *file_name, unsigned int open_flag, const char *share_name)
{
	MAPFILE_IMP *ret;
	SECURITY_ATTRIBUTES attr;
	int i;
	unsigned int create_view_mode = 0;
	unsigned int map_view_mode = 0;


	attr.nLength = sizeof(SECURITY_ATTRIBUTES);
	attr.lpSecurityDescriptor = 0;
	attr.bInheritHandle = open_flag & VOS_MAPPING_INHERIT;

	open_flag &= ~VOS_MAPPING_INHERIT;

	for(i=0;open_mode[i].flag != 0;i++) {

		if (open_mode[i].flag == open_flag) {
			create_view_mode = open_mode[i].mode;
			map_view_mode = open_mode[i].map_mode;
			break;
		}
	}
	if (open_mode[i].flag == 0) {
		// illegal flag combination
		return 0;
	}

	ret = (MAPFILE_IMP *) malloc( sizeof(MAPFILE_IMP) );
	if (!ret) {
		return 0;
	}
	memset(ret,0,sizeof(MAPFILE_IMP));



	ret->file = CreateFile(file_name, 
					  open_flag & VOS_MAPPING_WRITE ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
					  0, // share mode
					  &attr,
					  OPEN_EXISTING,
					  0, 
					  0);  // template file

	if (ret->file == INVALID_HANDLE_VALUE) {
		goto err;
	}

	ret->length_low = GetFileSize( ret->file, &ret->length_high );

	ret->mapping = CreateFileMapping(ret->file, 
						   &attr,
						   create_view_mode,
						   ret->length_high,
						   ret->length_low,
						   share_name);

	if (!ret->mapping) {
		goto err;

	}

	ret->addr =  MapViewOfFile( ret->mapping,
								map_view_mode,
								0,
								0,
								0);
	if (!ret->addr) {
		goto err;
	}



	return (VOS_MAPFILE) ret;

err:
	VOS_MAPFILE_close( (VOS_MAPFILE) ret );
	
	return 0;
}

#if 0
V_EXPORT VOS_MAPFILE VOS_MAPFILE_shared_mem( unsigned int open_flag, const char *share_name, V_UINT64 size)
{
	return 0;
}
#endif

V_EXPORT void VOS_MAPFILE_close(VOS_MAPFILE *mapfile)
{
	MAPFILE_IMP * imp = (MAPFILE_IMP *) mapfile;

	if (imp->addr) {
		UnmapViewOfFile( imp->addr );
	}
	if (imp->mapping) {
		CloseHandle( imp->mapping );
	}	
	if (imp->file) {
		CloseHandle(imp->file);
	}
	free(imp);
}

V_EXPORT int VOS_MAPFILE_flush(VOS_MAPFILE *mapfile)
{
	MAPFILE_IMP * imp = (MAPFILE_IMP *) mapfile;

	FlushViewOfFile( imp->addr, 0 );

	return 0;
}


V_EXPORT void *VOS_MAPFILE_get_ptr(VOS_MAPFILE *mapfile)
{
	MAPFILE_IMP * imp = (MAPFILE_IMP *) mapfile;

	return imp->addr;
}

V_EXPORT V_UINT64 VOS_MAPFILE_get_length(VOS_MAPFILE *mapfile)
{
	MAPFILE_IMP * imp = (MAPFILE_IMP *) mapfile;
	V_UINT64 res;

	res = imp->length_low;
	res |= ( ((V_UINT64) imp->length_high) << 32);

	return res;
}

