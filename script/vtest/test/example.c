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
#include "vtestcui.h"

void always_fails()
{ 
	VASSERT( 1 == 0 );
}

void always_pass()
{
}

void setUpFails()
{
	VASSERT(0);
}

/* define a test suite named FIRSTTEST*/
VTEST_DEFINE_SUITE( FIRSTTEST, 0, 0, SECONDTEST)
	VTEST_TEST( "alwayspass", always_pass)
	VTEST_TEST( "failtest", always_fails)
	VTEST_TEST( "anothertest", always_pass)
VTEST_END_SUITE

/* define a test suite named SECONDTEST*/
VTEST_DEFINE_SUITE( SECONDTEST, setUpFails, 0, LASTTEST)
	VTEST_TEST( "alwayspass", always_pass)
	VTEST_TEST( "failtest", always_fails)
	VTEST_TEST( "anothertest", always_pass)
VTEST_END_SUITE


/* define a test suite named LASTTEST; note that this is the last suite in the chain of suites*/
VTEST_DEFINE_LAST_SUITE( LASTTEST, 0, 0)
	/* repeat execution of the same test three times */
	VTEST_TEST_REPEATED( "failtest", always_fails, 3)
	VTEST_TEST( "alwayspass", always_pass)
VTEST_END_SUITE

main(int argc, char *argv[])
{
	VTEST_CUI_test_runner_cmdline( VTEST_SUITE_GET(FIRSTTEST), argc-1, argv+1 );
}
