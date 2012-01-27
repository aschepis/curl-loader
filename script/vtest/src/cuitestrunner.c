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
#include "vtestcui.h"
#include <stdio.h>
#include <string.h>

void CUI_report_suite_start (const char *suite_name)
{
	fprintf(stdout,"Suite %s\n", suite_name);
}

void CUI_report_test_start (const char *suite_name, const char *test_name, int iteration, int maxiteration)
{
	if (maxiteration == 1) {
		fprintf(stdout,"  Test: %s/%s started\n", suite_name, test_name);
	} else {
		fprintf(stdout,"  Test: %s/%s started. iteration #%d out of #%d\n", suite_name, test_name, iteration, maxiteration);
	}
}

void CUI_report_results(	VTEST_STATUS status, 
						const char *suite_name, const char *test_name,
						const char *fail_cond, const char *fail_file,int fail_line)
{
	(void) (test_name);

	switch(status) {
		case VTEST_TEST_OK:
			fprintf(stdout,"    Status: Ok\n");
			break;
		case VTEST_TEST_FAILED:
			fprintf(stdout,"    Status: Failed - %s at %s:%d\n", 
							fail_cond, fail_file, fail_line);
			break;
		case VTEST_SUITE_SETUP_FAILED:
			fprintf(stdout,"Suite %s setup failed - %s at %s:%d\n",
							suite_name, fail_cond, fail_file, fail_line);
			break;

		case VTEST_SUITE_TEARDOWN_FAILED:
			fprintf(stdout,"Suite %s teardown failed - %s at %s:%d\n",
							suite_name, fail_cond, fail_file, fail_line);
			break;

		default: {
				 }
	}
}

void CUI_report_wrapup	   (int suitesinitfailed, int suitesteardownfailed, 
							int tests_passed, int tests_failed, int testnotrun)
{
	fprintf(stdout,"\nTest summary\n"
				   "  Tests passed:			%d\n"
				   "  Tests failed:			%d\n"
				   "  Tests not run:		%d (due to suite setup failure)\n"
   				   "  Suites setup failed:		%d\n"
				   "  Suites teardown failed:	%d\n",

					tests_passed, tests_failed, testnotrun,
					suitesinitfailed, suitesteardownfailed);

	if (!suitesinitfailed && !suitesteardownfailed && 
		!tests_failed  && !testnotrun) {

		fprintf(stdout,"\nALL TESTS PASSED\n");

	} else {

		fprintf(stdout,"\n***YOU HAVE TESTS TO FIX***\n");

	}

}

void VTEST_CUI_test_runner(VTEST_TEST_SUITE *suite)
{
  VTEST_RUNNER_IMPL impl;

  impl.suite_start			= CUI_report_suite_start;
  impl.test_start			= CUI_report_test_start;
  impl.results				= CUI_report_results;
  impl.wrapup				= CUI_report_wrapup;

  VTEST_test_runner( suite, &impl );

}

void VTEST_CUI_test_runner_cmdline(VTEST_TEST_SUITE *suite, int argc, const char *argv[])
{
  VTEST_RUNNER_IMPL impl;

  impl.suite_start			= CUI_report_suite_start;
  impl.test_start			= CUI_report_test_start;
  impl.results				= CUI_report_results;
  impl.wrapup				= CUI_report_wrapup;

  VTEST_test_runner_cmdline( suite, &impl, argc, argv);

}

void VTEST_CUI_list_suite(VTEST_TEST_SUITE *suite)
{
	for(;suite; suite = suite->next_suite) {
		printf("%s ",suite->name);
	}
}

int VTEST_CUI_list_tests_of_suite(VTEST_TEST_SUITE *suite,const char *suite_name)
{
	int ret = 0;
	VTEST_TEST *test;
			

	for(;suite; suite = suite->next_suite) {
		if (strcmp(suite->name,suite_name) == 0) {
			ret = 1;

			for(test = suite->test_cases; test->name != 0; test++) {
				
				printf("%s ",test->name);
			}
		}
	}

	return ret;
}

