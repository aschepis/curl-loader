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
#ifndef _VTEST_H_
#define _VTEST_H_

#ifdef  __cplusplus
extern "C" {
#endif

typedef void (*VTEST_ACTION) (void);

typedef struct tagVTEST_TEST
{
  const char	*name;
  VTEST_ACTION	function;
  int			repeat; 
} VTEST_TEST;

typedef struct tagVTEST_TEST_SUITE
{
  const char		*name;
  VTEST_ACTION		setUp,tearDown;

  struct tagVTEST_TEST_SUITE 
					*next_suite;
  VTEST_TEST		*test_cases; 
  
} VTEST_TEST_SUITE;


/**
 * Start declaration of a test suite that is the last one out of a chain of tests.
 * \param name name of test (not string)
 * \param setUp function is called before start of suite
 * \param tearDown function is called after completion of suite
 */
#define VTEST_DEFINE_LAST_SUITE(name, setUp,tearDown) \
extern VTEST_TEST_SUITE *get_vtest_##name () \
{ \
  VTEST_ACTION argSetUp = (VTEST_ACTION) (setUp), argTearDown = (VTEST_ACTION) (tearDown);\
  \
  VTEST_TEST_SUITE *next_suite = 0;\
  \
  const char *suitename = #name;\
  \
  static VTEST_TEST_SUITE test; \
  \
  static VTEST_TEST test_cases[] = { \


/**
 * Start declaration of a test suite
 * \param name name of test (not string)
 * \param setUp function is called before start of suite
 * \param tearDown function is called after completion of suite
 * \param nextsuite the next test suite to follow after this one (not string)
 */
#define VTEST_DEFINE_SUITE(name, setUp, tearDown, nextsuite) \
\
extern VTEST_TEST_SUITE *get_vtest_##nextsuite (); \
\
extern VTEST_TEST_SUITE *get_vtest_##name () \
{ \
  VTEST_ACTION argSetUp = (VTEST_ACTION) (setUp), argTearDown = (VTEST_ACTION) (tearDown);\
  \
  VTEST_TEST_SUITE *next_suite = get_vtest_##nextsuite ();\
  \
  const char *suitename = #name ;\
  \
  static VTEST_TEST_SUITE test; \
  \
  static VTEST_TEST test_cases[] = { \


/**
 * declarations ends definition of a test suite
 */
#define VTEST_END_SUITE \
{ 0, 0, 0 } \
};\
  test.test_cases = (VTEST_TEST *) test_cases;\
  test.next_suite = next_suite;\
  test.setUp = argSetUp;\
  test.tearDown = argTearDown;\
  test.name = suitename;\
  \
  return &test;\
}

/**
 * define a test that is repeated for a number of times
 * \param tname  test name
 * \param test_fun test function of type VTEST_ACTION
 * \param repeat_count number of times that test is repeated
 */
#define VTEST_TEST_REPEATED( tname, test_fun, repeat_count) \
{ (tname), (VTEST_ACTION) (test_fun), (int) (repeat_count) }, 

/**
 * define a new test 
 * \param tname  test name
 * \param test_fun test function of type VTEST_ACTION
 */
#define VTEST_TEST( tname, test_fun) \
{ (tname), (VTEST_ACTION) (test_fun), 1 }, 

/**
 *  call to this function will signal test failure.
 */
void VFAIL(const char *cond, const char *file,int line);

/**
 *  Check the condition, failure of condtion will signal test failure, but test execution will continue.
 */
#define VCHECK(cond) { if (!(cond)) { VFAIL( #cond, __FILE__,__LINE__); } while(0);


/**
 *  Assert the condition, failure of condtion will signal test failure,and return from function (this assumes that curernt function is void).
 */
#define VASSERT(cond) { if (!(cond)) { VFAIL( #cond, __FILE__,__LINE__); return; } } while(0);


/**
 *  Assert the condition, failure of condtion will signal test failure, 
 *  and return with will be called (this assumes that curernt function is void).
 */
#define VASSERT_RET(cond,ret) { if (!(cond)) { VFAIL( #cond, __FILE__,__LINE__); return (ret); } } while(0);


/**
 * calls function that creates forward declaration for the test suite structure accessor.
 */
#define VTEST_SUITE_DECLARE_GET(name) \
extern VTEST_TEST_SUITE *get_vtest_##name ();


/**
 * calls function that returns the test suite structure for a given test.
 */
#define VTEST_SUITE_GET(name) \
  (VTEST_TEST_SUITE *)get_vtest_##name ()

#ifdef  __cplusplus
}
#endif

#endif

