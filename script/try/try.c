/*
*     try.c
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

#include <stdio.h>


struct {
  unsigned char a;
} STC_A;


struct {
  unsigned char a;
  unsigned char a1;
} STC_B;

struct {
  unsigned char a;
  unsigned char a1;
  unsigned char a2;
} STC_C;

struct {
  int b;
  unsigned char a[2];
}  STC_D;



struct {
  unsigned short a;
  int b;
}  STC_E;

struct {
  unsigned short a;
  unsigned char b;
}  STC_F;

main()
{

        printf("float size %d double size %d\n",sizeof(float), sizeof(double));

/* ! all general purpose allocations (malloc) are aligned to sizeof(double) !*/
/* ! most stuff that does not include double as struct members would need only four !*/
	
	printf("sizeof double %d\n",sizeof(double));
/*

! alignment of the entire structure is at least as much as the largest 
alignment of members !

http://gcc.gnu.org/ml/gcc-bugs/2000-06/msg00660.html

GCC uses different rules for alignment of fields and structures, based on the
ABI of the target.  In general if there is a wacky way to pad and align
structures that meets the standard (address of the first element of a structure
when suitably converted is the same as the structure address, addresses in a
struct are monotonically increasing and all addresses of union members when
suitable converted are the same, alignment of the entire structure is at least
as much as the largest alignment of members), some GCC port probably implements
it.  Some things that I can think of off the top of my head:

   1)	Whether the type of a bitfield influences the alignment of the whole
	structure (ie, struct { int x:1; } takes 4 or 8 bytes, while
	struct { char x:1; } takes 1 byte).

   2)	RISC platforms generally want native alignment (chars aligned on 1 byte
	boundaries, shorts on 2 byte boundaries, ints on 4 or 8 byte
	boundaries, and the entire structure alginment is the max of the member
	alignments).

   3)	Both the 68k and 386 ABIs cater to the quirks of the original
	implementation to the detrement of future generations of the chip
	(IIRC, the 68k uses 2 byte alignment instead of native alignment, the
	386 does not align doubles to 8 byte boundaries).

   4)	AIX aligns doubles to 32-bits within structures, but if the first field
	in a structure is a double, it will align the entire structure to 64
	bits.
*/

  printf("STC_A %d\nSTC_B %d\nSTC_C %d\nSTC_D %d\nSTC_E %d\nSTC_F %d\n",
		  sizeof(STC_A), sizeof(STC_B), sizeof(STC_C), sizeof(STC_D), sizeof(STC_E), sizeof(STC_F));
  return 0;
}
