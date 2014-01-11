#ifndef HTEST_H_
#define HTEST_H_

/* A very, very, very, VERY simple unit testing framework for use by the
   unit tests. Here's how it works:

    1) A "test" is a function of this type:
       int test(void);
       A test that returns 0 is failed; any other return value is considered
   	   a success.
    2) Tests use assertions; assertions are run with the HTEST_ASSERT macro,
       which is below. The macro consumes a conditional expression and returns
       0 with an error message if the condition is false. For example:
       HTEST_ASSERT(1 == 1);
       A test should be made up of a series of asserts, terminated by a
       `return 1;`.
    3) You can hook together testing functions with the HTEST_RUN macro,
       which runs a test function, returns 0 if the test failed, and does
       nothing otherwise. For example:

int test1(void)
{
  HTEST_ASSERT(1 == 1);
  return 1;
}
int test2(void)
{
  HTEST_ASSERT(2 == 2);
  return 1;
}
int run_tests(void)
{
  HTEST_RUN(test1);
  HTEST_RUN(test2);
  return 1;
}
    HTEST_RUN allows you to tie the success of other tests to the success of
    the current test; a test function can use both ASSERT's and RUN's.

    These macros are adapted from the minunit testing framework
    ( http://www.jera.com/techinfo/jtns/jtn002.html ).

    No additional source files are required; the whole framework is contained
    in this file.
*/


#define HTEST_ASSERT(cond) \
  do { \
    if (!(cond)) { \
      fprintf(stderr, \
      		  "HTEST ERROR at line %d in file %s: assertion \"%s\" failed\n", \
      		  __LINE__, __FILE__, #cond);\
      return 0;\
    }\
 } while (0)

#define HTEST_RUN(test) do { if (!(test)()) return 0; } while (0)

#endif
