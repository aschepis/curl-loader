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
#ifndef _VTESTRUNNER_H_
#define _VTESTRUNNER_H_

typedef enum {
  VTEST_STATUS_OK  = 1,
  VTEST_STATUS_FAILED = 0,
}
  VTEST_STATUS;

typedef void (*VTEST_RUNNER_report_test_suite) (const char *suite_name, int is_start);
typedef void (*VTEST_RUNNER_report_test_start) (const char *suite_name, const char *test_name, int iteration);
typedef void (*VTEST_RUNNER_report_test_result)(const char *suite_name, const char *test_name, VTEST_STATUS status, const char *fail_cond, const char *fail_file,int fail_line);
typedef void (*VTEST_RUNNER_report_wrapup)	   (int tests_passed, int tests_failed);

typedef struct tagVTEST_RUNNER_IMPL {
	
	VTEST_RUNNER_report_test_suite  suite_start_finish;
	VTEST_RUNNER_report_test_start  test_start;
	VTEST_RUNNER_report_test_result test_result;
	VTEST_RUNNER_report_wrapup      wrapup;

	int				  current_test_state;
	VTEST_TEST_SUITE *suite;
	VTEST_TEST		 *test;

} VTEST_RUNNER_IMPL;

void VTEST_test_runner(VTEST_TEST_SUITE *suite, VTEST_RUNNER_IMPL *impl);


#endif
