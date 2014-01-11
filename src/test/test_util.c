#include "test_util.h"

int buffer_equal(const unsigned char *b1, const unsigned char *b2, size_t sz)
{
  size_t i;
  for (i = 0; i < sz; i ++) {
  	if (b1[i] != b2[i]) return 0;
  }
  return 1;
}

