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

typedef void (*VTEST_ACTION) (void);

typedef struct tagVTEST_TEST
{
  const char  *name;
  VTEST_ACTION function;
  int          repeat; 

} VTEST_TEST;

typedef struct tagVTEST_TEST_SUITE
{
  const char        *name;
  VTEST_ACTION      setUp,tearDown;
  struct tagVTEST_TEST_SUITE 
				    *next_suite;
  VTEST_TEST        *test_cases; 
  
} VTEST_TEST_SUITE;


/**
 *
 */
#define VTEST_DEFINE_LAST_SUITE(name, setUp,tearDown) \
extern VTEST_TEST_SUITE *get_vtest_##name () \
{ \
  VTEST_ACTION argSetUp = setUp, argTearDown = tearDown;\
  \
  VTEST_TEST_SUITE *next_suite = 0;\
  \
  const char *suitename = "##name";\
  \
  static VTEST_TEST_SUITE test; \
  \
  static VTEST_TEST test_cases[] = { \


/**
 *
 */
#define VTEST_DEFINE_SUITE(name, setUp, tearDown, nextsuite) \
\
extern VTEST_TEST_SUITE *get_vtest_##nextsuite (); \
\
extern VTEST_TEST_SUITE *get_vtest_##name () \
{ \
  VTEST_ACTION argSetUp = setUp, argTearDown = tearDown;\
  \
  VTEST_TEST_SUITE *next_suite = get_vtest_##nextsuite ();\
  \
  const char *suitename = "##name";\
  \
  static VTEST_TEST_SUITE test; \
  \
  static VTEST_TEST test_cases[] = { \


/**
 *
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

#define VTEST_TEST_REPEATED( tname, test_fun, repeat_count) \
{ tname, (VTEST_ACTION) test_fun, (int) repeat_count }, 

#define VTEST_TEST( tname, test_fun) \
{ tname, (VTEST_ACTION) test_fun, 1 }, 

void VFAIL(const char *cond, const char *file,int line);

#define VASSERT(cond) (!cond ? VFAIL( #cond, __FILE__,__LINE__) : 0)

#define VASSERT_RET(cond,ret) {(!cond ? VFAIL( "##cond", __FILE__,__LINE) : 0); return ret; }




#endif

