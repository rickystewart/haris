#ifndef TEST_UTIL_H_
#define TEST_UTIL_H_

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* Utilities that will prove useful in all test files. */

int buffer_equal(const unsigned char *, const unsigned char *, size_t);

#endif