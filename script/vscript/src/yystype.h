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
#ifndef _YYSTYPE_H_
#define _YYSTYPE_H_


#define  YYSTYPE_IS_DECLARED 

struct tagAST_BASE;

typedef union {
	char   *string_value;
	double  double_value;
	long	long_value;
	int		int_value;
	struct tagAST_BASE *ast;	

} YYSTYPE;



#define YYLTYPE_IS_DECLARED

typedef struct YYLTYPE
{
	int file_id;	// offset of file entry object (what is the file that parsed this one here)
	
	int first_line;
	int first_column;
	int last_line;
	int last_column;

} YYLTYPE;

#define YYLTYPE_set_null(x) \
	do { (x).file_id = (x).first_line =  \
		 (x).last_line = (x).first_column =  \
		 (x).last_column = -1; } while(0);

#endif

