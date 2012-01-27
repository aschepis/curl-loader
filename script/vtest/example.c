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
#include "vtest.h"

void always_fails()
{ 
}

void always_pass()
{
	VASSERT( 1 == 0 );
}

/*
extern VTEST_TEST_SUITE *get_vtest_TEST()
{
  static VTEST_TEST_SUITE test;
  static VTEST_TEST test_cases[] = {
		{ "kuku", (VTEST_ACTION) always_fails, 1 }, 	
		{ 0, 0, 0 }
	};
  test.test_cases = (VTEST_TEST *) test_cases;
  test.next_suite = 0;

  return &test;
}
*/

VTEST_DEFINE_SUITE( FIRSTTEST, 0, 0, LASTTEST)
	VTEST_TEST( "alwayspass", always_pass)
	VTEST_TEST( "failtest", always_fails)
	VTEST_TEST( "alwayspass", always_pass)
VTEST_END_SUITE

VTEST_DEFINE_LAST_SUITE( LASTTEST, 0, 0)
	VTEST_TEST( "failtest", always_fails)
	VTEST_TEST( "alwayspass", always_pass)
VTEST_END_SUITE
