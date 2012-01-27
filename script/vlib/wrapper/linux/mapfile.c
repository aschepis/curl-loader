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
#include <sys/types.h>
//#define _USE_LARGFILE64
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

typedef struct tagMAPFILE_IMP {
	int  file;
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
	{ VOS_MAPPING_READ, O_RDONLY, PROT_READ },
	{ (VOS_MAPPING_READ|VOS_MAPPING_WRITE), O_RDWR, PROT_READ | PROT_WRITE },
	{ (VOS_MAPPING_READ|VOS_MAPPING_EXEC),  O_RDONLY, PROT_EXEC },
	{ (VOS_MAPPING_READ|VOS_MAPPING_WRITE|VOS_MAPPING_EXEC), O_RDWR, PROT_READ | PROT_WRITE | PROT_EXEC },
	{ 0,0,0 }
};	


int _fstat(int fd, struct stat *);

V_EXPORT VOS_MAPFILE VOS_MAPFILE_open(const char *file_name, unsigned int open_flag, const char *share_name)
{
	MAPFILE_IMP *ret;
	int i;
	int file_mode, map_mode;
	struct stat buf;

	V_UNUSED(share_name);

	for(i=0;open_mode[i].flag != 0;i++) {

		if (open_mode[i].flag == open_flag) {
			file_mode = open_mode[i].mode;
			map_mode = open_mode[i].map_mode;
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

#if linux
	stat( file_name, &buf );
#endif	


	ret->file = open( file_name, file_mode);

	if (ret->file == -1) {
		goto err;
	}

#if _WIN32
	_fstat( ret->file, &buf );
#endif	

	ret->length_low = buf.st_size;


	ret->addr =  mmap( 0, ret->length_low, map_mode, MAP_PRIVATE, ret->file, 0);
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
		munmap( imp->addr, imp->length_low );
	}
	if (imp->file != -1){
		close(imp->file);
	}
	free(imp);
}

V_EXPORT int VOS_MAPFILE_flush(VOS_MAPFILE *mapfile)
{
	MAPFILE_IMP * imp = (MAPFILE_IMP *) mapfile;

	msync( imp->addr, 0, MS_SYNC);

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

