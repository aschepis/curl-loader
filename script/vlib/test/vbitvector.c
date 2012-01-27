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

#include <util/vbitvector.h>
#include <stdio.h>
#include "vtest.h"

extern VCONTEXT * test_alloc;


void VBITVECTOR_test()
{
	VBITVECTOR bits;
	int      i,bitvalue;
	V_UINT32 ii,cnt;


	/* size is V_UIN32 aligned*/
	VASSERT(  VBITVECTOR_getsize(21) == 4);

	VASSERT( !VBITVECTOR_init( test_alloc, &bits, 21) );

	for(i=0;i<20;i++) {
	   VASSERT( ! VBITVECTOR_testbit(&bits,i) );
	}

	VBITVECTOR_setbit(&bits,0);
	VBITVECTOR_setbit(&bits,10);
	VBITVECTOR_setbit(&bits,20);

	for(i=0;i<20;i++) {
		switch(i) {
		case 0:
		case 10:
		case 20:
		   VASSERT( VBITVECTOR_testbit(&bits,i) );
		   break;
		default:
		   VASSERT( ! VBITVECTOR_testbit(&bits,i) );
		   break;
		}
	}

	VBITVECTOR_clearbit(&bits,0);
	VBITVECTOR_clearbit(&bits,10);
	VBITVECTOR_clearbit(&bits,20);

	for(i=0;i<20;i++) {
	   VASSERT( !VBITVECTOR_testbit(&bits,i) );
	}


	VBITVECTOR_setbit(&bits,0);
	VBITVECTOR_setbit(&bits,10);
	VBITVECTOR_setbit(&bits,20);

	VASSERT( !VBITVECTOR_resize( &bits, 33) );
	VASSERT(  VBITVECTOR_getsize(33) == 8);

	for(i=0;i<33;i++) {
		switch(i) {
		case 0:
		case 10:
		case 20:
		   VASSERT( VBITVECTOR_testbit(&bits,i) );
			   break;
		default:
		   VASSERT( ! VBITVECTOR_testbit(&bits,i) );
		   break;
		}
	}

	cnt = 0;
	VBITVECTOR_FOREACH_SET_BIT(ii,&bits)
		cnt += 1;
		switch(ii) {
		case 0:
		case 10:
		case 20:
			break;
		default:
		   VASSERT( 0 );
		   break;
		}	
	VBITVECTOR_FOREACH_END
	VASSERT(cnt==3);


	cnt = 0;
	VBITVECTOR_FOREACH_BIT(ii,bitvalue,&bits)
		cnt += 1;
		switch(ii) {
		case 0:
		case 10:
		case 20:
			VASSERT( bitvalue == 1 );
			break;
		default:
		   VASSERT( bitvalue == 0 );
		   break;
		}	

	VBITVECTOR_FOREACH_END
	VASSERT(cnt==33)

	cnt = 0;
	VBITVECTOR_FOREACH_ZERO_BIT(ii,&bits)
		cnt += 1;
		switch(ii) {
		case 0:
		case 10:
		case 20:
			VASSERT( 0 );
			break;
		default:		   
		   break;
		}	

	VBITVECTOR_FOREACH_END
	VASSERT(cnt==31); /* TODO: should be 30 ? */

	VBITVECTOR_free( &bits );

}

