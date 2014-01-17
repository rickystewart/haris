TEST_PROGRAMS = simple.test #numtest.test

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99

# To add a test program to the framework, take the following steps:
# 1) Come up with a unique name for the test. Your test should use
#    exactly 1 .haris file; once you have your name N, create the
#    schema N.haris and put it in the test/ directory. Add `N.test`
#    to the TEST_PROGRAMS list up at the top of this file.
# 2) Create a file N.c which will have the test logic in it; this file
#    needs a main() function, as it contains the logic for the
#    entire test program. Make sure it includes "test_util.h"
#    and uses the htest.h library.
# 3) Go up to the src/ directory; in the Makefile there, there is
#    a variable called TEST_FILES. Just add the file N.haris.c to
#    the end of the list.
# Now, in src/, your new test will be built and run when make check is run.

check:	$(TEST_PROGRAMS)

%.test:	%.c %.haris.c %.haris.h test_util.c test_util.h
	$(CC) $(CFLAGS) -o $@ $(@:.test=.c) $(@:.test=.haris.c) test_util.c
	./$@

clean:
	rm $(TEST_PROGRAMS)
