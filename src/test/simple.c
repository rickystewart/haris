#include "htest.h"
#include "simple.haris.h"
#include "test_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

static int buffer_encoding_test_1(void)
{
  unsigned char buffer[10], *out_addr,
    test_buffer[10] = { 0x40, 0x8, 0, 0, 0, 0, 0, 0, 0, 0 };
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple);
  simple->u = 0U;
  HTEST_ASSERT(Simple_to_buffer(simple, buffer, sizeof buffer, &out_addr)
               == HARIS_SUCCESS);
  HTEST_ASSERT(out_addr - buffer == 10);
  HTEST_ASSERT(buffer_equal(buffer, test_buffer, 10));
  Simple_destroy(simple);
  return 1;
}

static int buffer_encoding_test_2(void)
{
  unsigned char *buffer, 
    test_buffer[10] = { 0x40, 0x8, 0, 0, 0, 0, 0, 0, 0, 0 };
  haris_uint32_t sz;
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple);
  simple->u = 0U;
  HTEST_ASSERT(Simple_to_buffer_a(simple, &buffer, &sz) == HARIS_SUCCESS);
  HTEST_ASSERT(sz == 10);
  HTEST_ASSERT(buffer_equal(buffer, test_buffer, 10));
  Simple_destroy(simple);
  return 1;
}

static int buffer_encoding_test_3(void)
{
  unsigned char buffer[10], *out_addr,
    test_buffer[10] = { 0x40, 0x8, 0, 0x11, 0, 0, 0, 0, 0, 0 };
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple);
  simple->u = 0x1100U;
  HTEST_ASSERT(Simple_to_buffer(simple, buffer, sizeof buffer, &out_addr)
               == HARIS_SUCCESS);
  HTEST_ASSERT(out_addr - buffer == 10);
  HTEST_ASSERT(buffer_equal(buffer, test_buffer, 10));
  Simple_destroy(simple);
  return 1;
}

static int buffer_encoding_test_4(void)
{
  unsigned char *buffer, 
    test_buffer[10] = { 0x40, 0x8, 0, 0x11, 0, 0, 0, 0, 0, 0 };
  haris_uint32_t sz;
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple);
  simple->u = 0x1100U;
  HTEST_ASSERT(Simple_to_buffer_a(simple, &buffer, &sz) == HARIS_SUCCESS);
  HTEST_ASSERT(sz == 10);
  HTEST_ASSERT(buffer_equal(buffer, test_buffer, 10));
  Simple_destroy(simple);
  return 1;
}

static int buffer_encoding_test_5(void)
{
  unsigned char buffer[10], *out_addr,
    test_buffer[10] = { 0x40, 0x8, 0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE };
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple);
  simple->u = 0xDEADBEEFDEADBEEFU;
  HTEST_ASSERT(Simple_to_buffer(simple, buffer, sizeof buffer, &out_addr)
               == HARIS_SUCCESS);
  HTEST_ASSERT(out_addr - buffer == 10);
  HTEST_ASSERT(buffer_equal(buffer, test_buffer, 10));
  Simple_destroy(simple);
  return 1;
}

static int buffer_encoding_test_6(void)
{
  unsigned char *buffer, 
    test_buffer[10] = { 0x40, 0x8, 0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE };
  haris_uint32_t sz;
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple);
  simple->u = 0xDEADBEEFDEADBEEFU;
  HTEST_ASSERT(Simple_to_buffer_a(simple, &buffer, &sz) == HARIS_SUCCESS);
  HTEST_ASSERT(sz == 10);
  HTEST_ASSERT(buffer_equal(buffer, test_buffer, 10));
  Simple_destroy(simple);
  return 1;
}

static int (* const buffer_encoding_test_functions[])(void) = {
  buffer_encoding_test_1, buffer_encoding_test_2,
  buffer_encoding_test_3, buffer_encoding_test_4,
  buffer_encoding_test_5, buffer_encoding_test_6
};

static int buffer_encoding_tests(void)
{
  unsigned i;
  for (i = 0; 
       i < sizeof buffer_encoding_test_functions /
           sizeof buffer_encoding_test_functions[0];
       i++)
    HTEST_RUN(buffer_encoding_test_functions[i]);
  return 1;
}

static int file_encoding_test_1(void)
{
  unsigned char buffer[10],
    test_buffer[10] = { 0x40, 0x8, 0, 0, 0, 0, 0, 0, 0, 0 };
  haris_uint32_t sz;
  FILE *f = tmpfile();
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple && f);
  simple->u = 0U;
  HTEST_ASSERT(Simple_to_file(simple, f, &sz) == HARIS_SUCCESS);
  HTEST_ASSERT(sz == 10);
  rewind(f);
  fread(buffer, 1, sz, f);
  HTEST_ASSERT(buffer_equal(buffer, test_buffer, 10));
  HTEST_ASSERT(fgetc(f) == EOF);
  fclose(f);
  Simple_destroy(simple);
  return 1;
}

static int file_encoding_test_2(void)
{
  unsigned char buffer[10],
    test_buffer[10] = { 0x40, 0x8, 0, 0x11, 0, 0, 0, 0, 0, 0 };
  haris_uint32_t sz;
  FILE *f = tmpfile();
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple && f);
  simple->u = 0x1100U;
  HTEST_ASSERT(Simple_to_file(simple, f, &sz) == HARIS_SUCCESS);
  HTEST_ASSERT(sz == 10);
  rewind(f);
  fread(buffer, 1, sz, f);
  HTEST_ASSERT(buffer_equal(buffer, test_buffer, 10));
  HTEST_ASSERT(fgetc(f) == EOF);
  fclose(f);
  Simple_destroy(simple);
  return 1;
}

static int file_encoding_test_3(void)
{
  unsigned char buffer[10],
    test_buffer[10] = { 0x40, 0x8, 0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE };
  haris_uint32_t sz;
  FILE *f = tmpfile();
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple && f);
  simple->u = 0xDEADBEEFDEADBEEFU;
  HTEST_ASSERT(Simple_to_file(simple, f, &sz) == HARIS_SUCCESS);
  HTEST_ASSERT(sz == 10);
  rewind(f);
  fread(buffer, 1, sz, f);
  HTEST_ASSERT(buffer_equal(buffer, test_buffer, 10));
  HTEST_ASSERT(fgetc(f) == EOF);
  fclose(f);
  Simple_destroy(simple);
  return 1;
}

static int (* const file_encoding_test_functions[])(void) = {
  file_encoding_test_1, file_encoding_test_2,
  file_encoding_test_3
};

static int file_encoding_tests(void)
{
  unsigned i;
  for (i = 0; 
       i < sizeof file_encoding_test_functions /
           sizeof file_encoding_test_functions[0];
       i++)
    HTEST_RUN(file_encoding_test_functions[i]);
  return 1;
}

static int encoding_tests(void)
{
  HTEST_RUN(buffer_encoding_tests);
  HTEST_RUN(file_encoding_tests);
  return 1;
}

static int buffer_decoding_test_1(void)
{
  unsigned char buffer[10] = { 0x40, 0x8, 0, 0, 0, 0, 0, 0, 0, 0 },
    *out_addr;
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple);
  HTEST_ASSERT(Simple_from_buffer(simple, buffer, sizeof buffer, &out_addr)
               == HARIS_SUCCESS);
  HTEST_ASSERT(out_addr - buffer == 10);
  HTEST_ASSERT(simple->u == 0);
  Simple_destroy(simple);
  return 1;
}

static int buffer_decoding_test_2(void)
{
  unsigned char buffer[10] = { 0x40, 0x8, 0, 0x11, 0, 0, 0, 0, 0, 0 },
    *out_addr;
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple);
  HTEST_ASSERT(Simple_from_buffer(simple, buffer, sizeof buffer, &out_addr)
               == HARIS_SUCCESS);
  HTEST_ASSERT(out_addr - buffer == 10);
  HTEST_ASSERT(simple->u == 0x1100);
  Simple_destroy(simple);
  return 1;
}

static int buffer_decoding_test_3(void)
{
  unsigned char buffer[10] = { 0x40, 0x8, 0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE },
    *out_addr;
  Simple *simple = Simple_create();
  HTEST_ASSERT(simple);
  HTEST_ASSERT(Simple_from_buffer(simple, buffer, sizeof buffer, &out_addr)
               == HARIS_SUCCESS);
  HTEST_ASSERT(out_addr - buffer == 10);
  HTEST_ASSERT(simple->u == 0xDEADBEEFDEADBEEF);
  Simple_destroy(simple);
  return 1;
}

static int (* const buffer_decoding_test_functions[])(void) = {
  buffer_decoding_test_1, buffer_decoding_test_2,
  buffer_decoding_test_3
};

static int buffer_decoding_tests(void)
{
  unsigned i;
  for (i = 0; 
       i < sizeof buffer_decoding_test_functions /
           sizeof buffer_decoding_test_functions[0];
       i++)
    HTEST_RUN(buffer_decoding_test_functions[i]);
  return 1;
}

static int file_decoding_test_1(void)
{
  unsigned char buffer[10] = { 0x40, 0x8, 0, 0, 0, 0, 0, 0, 0, 0 };
  haris_uint32_t sz;
  Simple *simple = Simple_create();
  FILE *f = tmpfile();
  HTEST_ASSERT(simple && f);
  fwrite(buffer, 1, 10, f);
  rewind(f);
  HTEST_ASSERT(Simple_from_file(simple, f, &sz) == HARIS_SUCCESS);
  HTEST_ASSERT(sz == 10);
  HTEST_ASSERT(fgetc(f) == EOF);
  HTEST_ASSERT(simple->u == 0);
  Simple_destroy(simple);
  fclose(f);
  return 1;
}

static int file_decoding_test_2(void)
{
  unsigned char buffer[10] = { 0x40, 0x8, 0, 0x11, 0, 0, 0, 0, 0, 0 };
  haris_uint32_t sz;
  Simple *simple = Simple_create();
  FILE *f = tmpfile();
  HTEST_ASSERT(simple && f);
  fwrite(buffer, 1, 10, f);
  rewind(f);
  HTEST_ASSERT(Simple_from_file(simple, f, &sz) == HARIS_SUCCESS);
  HTEST_ASSERT(sz == 10);
  HTEST_ASSERT(fgetc(f) == EOF);
  HTEST_ASSERT(simple->u == 0x1100);
  Simple_destroy(simple);
  fclose(f);
  return 1;
}

static int file_decoding_test_3(void)
{
  unsigned char buffer[10] = { 0x40, 0x8, 0xEF, 0xBE, 0xAD, 0xDE, 0xEF, 0xBE, 0xAD, 0xDE };
  haris_uint32_t sz;
  Simple *simple = Simple_create();
  FILE *f = tmpfile();
  HTEST_ASSERT(simple && f);
  fwrite(buffer, 1, 10, f);
  rewind(f);
  HTEST_ASSERT(Simple_from_file(simple, f, &sz) == HARIS_SUCCESS);
  HTEST_ASSERT(sz == 10);
  HTEST_ASSERT(fgetc(f) == EOF);
  HTEST_ASSERT(simple->u == 0xDEADBEEFDEADBEEF);
  Simple_destroy(simple);
  fclose(f);
  return 1;
}

static int (* const file_decoding_test_functions[])(void) = {
  file_decoding_test_1, file_decoding_test_2,
  file_decoding_test_3
};

static int file_decoding_tests(void)
{
  unsigned i;
  for (i = 0; 
       i < sizeof file_decoding_test_functions /
           sizeof file_decoding_test_functions[0];
       i++)
    HTEST_RUN(file_decoding_test_functions[i]);
  return 1;
}

static int decoding_tests(void)
{
  HTEST_RUN(buffer_decoding_tests);
  HTEST_RUN(file_decoding_tests);
  return 1;
}

static int all_tests(void)
{
  HTEST_RUN(encoding_tests);
  HTEST_RUN(decoding_tests);
  return 1;
}

int main(void)
{
  if (!all_tests()) return -1;
  else {
  	printf("All tests succeeded.\n");
  	return 0;
  }
}
