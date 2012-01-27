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
#include "vtestrunner.h"
#include <string.h>
#include <malloc.h>

#ifdef WIN32
#include <windows.h>
#endif

VTEST_RUNNER_IMPL * vtest_impl;
static char *g_suite_name;

#ifdef WIN32

typedef BOOL (WINAPI *PIsDebuggerPresent)(void);

BOOL DynIsDebuggerPresent(void)
{
	PIsDebuggerPresent isDeb;
	CHAR K32Path[ MAX_PATH ];
	HINSTANCE hK32;
	BOOL Rc;

	Rc = FALSE;
	GetSystemDirectory( K32Path, MAX_PATH );
	strcat(K32Path,"\\");
	strcat( K32Path, "KERNEL32.DLL" );

	hK32 = LoadLibrary( K32Path );
	if( hK32 != NULL ) {
		isDeb = (PIsDebuggerPresent) GetProcAddress(hK32,"IsDebuggerPresent");
		if(isDeb != NULL) {
			Rc = isDeb();
		}
		FreeLibrary( hK32 );
	}

	return Rc;
}
#endif

void VFAIL(const char *cond, const char *file,int line)
{
	vtest_impl->results( vtest_impl->scope_fail, 
						 vtest_impl->suite->name, 
						 vtest_impl->test ? vtest_impl->test->name : 0, 
						 cond, file, line);

	vtest_impl->current_test_state = 0;

#ifdef WIN32
	if (DynIsDebuggerPresent()) {
		DebugBreak();	
	}
#endif

}


static int parse_suite_name(const char *line, char **suite_name,int *len)
{
	const char *tmp = line;
	char *next;
	
	*suite_name = (char *) line;
	next = strchr(tmp,'/');

	if (next) {
		*len = next - tmp;
	} else {
		*len = strlen(line);
	}

	return 1;

}

static int parse_next_test_name(const char *line, char **test_name, int *len, char **last_pos)
{
	const char *next;

	if (*last_pos == 0) {
		*last_pos = strchr(line,'/');
		if (*last_pos == 0) {
			return 0;
		}
		*last_pos += 1;
	} else {
		if (**last_pos != ',') {
			return 0;
		}
		*last_pos = *last_pos + 1;
	}

	if (**last_pos == '\0') {
		return 0;
	}

	next = strchr( *last_pos, ',');

	*test_name = *last_pos;
	if (next) {
		*len = next - *last_pos;
	} else {
		*len = strlen(*last_pos);
		next = *last_pos + *len;
	}
	*last_pos = (char *) next;
	return 1;
}


/* do we run the given suite, given the current command line */
static int VTEST_is_run_suite(const char *suite_name, const char *argv[],int argc)
{
	int i;
	char *tmp_suite_name;
	int suite_name_len;
	
	if (!argv || !argc) {
		return 1;
	}

	for(i = 0; i < argc; i++) {
		
		if (parse_suite_name(argv[i],&tmp_suite_name, &suite_name_len)) {
			if (strlen(suite_name) == (size_t) suite_name_len) {
				if (memcmp(suite_name, tmp_suite_name, suite_name_len) == 0) {
					return 1;
				}			
			}
		}

	}

	return 0;
}

/* do we run the given test, given the current command line */
static int VTEST_is_run_test(const char *suite_name, const char *test_name, const char *argv[],int argc)
{
	char *last_pos, *tstname, *tmp_suite_name;
	int tstlen, i, is_suite, suite_name_len, test_count;

	if (!argv || !argc) {
		return 1;
	}

	for(i = 0; i < argc; i++) {

		is_suite = 0;
		if (parse_suite_name(argv[i],&tmp_suite_name, &suite_name_len)) {
			if (strlen(suite_name) == (size_t) suite_name_len) {
				if (memcmp(suite_name, tmp_suite_name, suite_name_len) == 0) {
					is_suite = 1;
				}			
			}
		}

		if (!is_suite) {
			continue;
		}

		for( test_count = 0, last_pos = 0; parse_next_test_name(argv[i],&tstname,&tstlen,&last_pos); test_count += 1 ) {
			if (strlen(test_name) == (size_t) tstlen) {
				if (memcmp(test_name, tstname, tstlen) == 0) {
					return 1;
				}
			}
		}

		if (!test_count) {
			return 1;
		}
	
	}
	return 0;
}



void VTEST_goto_next_suite(const char *suite_name)
{
	if (suite_name) {
		g_suite_name = strdup(suite_name);
	}
}

static VTEST_TEST_SUITE *find_suite_by_name(VTEST_TEST_SUITE *first_suite, const char *suite_name)
{

	for( ;first_suite; first_suite = first_suite->next_suite ) {

		if (strcmp( first_suite->name, suite_name) == 0) {
			return first_suite;
		}
	}
	return 0;
}

void VTEST_test_runner_cmdline(VTEST_TEST_SUITE *suite, VTEST_RUNNER_IMPL *impl, int argc, const char *argv[])
{
	int testsfailed = 0;
	int testspassed = 0;
	int testnotrun  = 0;
	int suitesinitfailed = 0;
	int suitesteardownfailed = 0;
	VTEST_TEST_SUITE *first_suite = suite;

	int i,suite_ok;

	vtest_impl = impl;

	for(;suite; suite = suite->next_suite) {
		VTEST_TEST *test;

		if (g_suite_name) {
			VTEST_TEST_SUITE *next;

			next = find_suite_by_name(first_suite, g_suite_name);
			free(g_suite_name);
			g_suite_name = 0;
			
			if (next) {
				suite = next;
			}			
		}

		vtest_impl->suite = suite;
		vtest_impl->test = 0;


		if (!VTEST_is_run_suite(suite->name, argv, argc)) {
			continue;
		}

		if (impl->suite_start) {
		  impl->suite_start(suite->name);
		}

		suite_ok = 1;
		if (suite->setUp) {
			
			vtest_impl->current_test_state = 1; 
			vtest_impl->scope_fail = VTEST_SUITE_SETUP_FAILED;

			suite->setUp();

			if (!vtest_impl->current_test_state) {
				suite_ok = 0;	
			}
		}

		if (suite_ok)  {

			vtest_impl->results( VTEST_SUITE_SETUP_OK, suite->name, 0, 0, 0, 0);
		
			for(test = suite->test_cases; test->name != 0; test++) {

			   if (!VTEST_is_run_test(suite->name, test->name, argv, argc)) {
					continue;
			   }


			   vtest_impl->test = test;
				
			   for(i = 0; i < test->repeat /*&& !g_suite_name*/ ; i++) {

				   impl->test_start( suite->name, test->name, i+1, test->repeat);

				   vtest_impl->current_test_state = 1; 
				   vtest_impl->scope_fail = VTEST_TEST_FAILED;

				   test->function();

				   if (vtest_impl->current_test_state) {
					  vtest_impl->results( VTEST_TEST_OK,
										   vtest_impl->suite->name, 
										   vtest_impl->test->name, 
										   0, 0, 0);
				   }
				   if (vtest_impl->current_test_state) {
					   testspassed++;
				   } else {
					   testsfailed++;
				   }
			   }
			   
			}

			if (suite->tearDown) {

  				vtest_impl->current_test_state = 1; 
				vtest_impl->scope_fail = VTEST_SUITE_TEARDOWN_FAILED;
				vtest_impl->test = 0;

				suite->tearDown();

			
				if (vtest_impl->current_test_state) {
					vtest_impl->results( VTEST_SUITE_TEARDOWN_OK, suite->name, 0, 0, 0, 0);
					
				} else {
					suitesteardownfailed++;
				}
			}

		} else {
			/*test setup failed*/
			for(test = suite->test_cases;test->name != 0;test++) {
				testnotrun ++;
			}
			suitesinitfailed ++;
		}
		
	}

	vtest_impl->wrapup( suitesinitfailed, suitesteardownfailed,
					    testspassed, testsfailed, testnotrun);
}


void VTEST_test_runner(VTEST_TEST_SUITE *suite, VTEST_RUNNER_IMPL *impl)
{
	VTEST_test_runner_cmdline(suite, impl,0,0);
}





