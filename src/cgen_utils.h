#ifndef __CGEN_UTILS_H
#define __CGEN_UTILS_H

/* This file contains the contents of the util.h and util.c files, encoded
   as arrays of constant C strings. (I have encoded them as arrays of small
   strings rather than as large strings because ANSI C89 only guarantees
   that compilers be able to support strings that are 509 characters long.
   To write the whole file to disk, loop through the elements of the 
   array and write each string to disk (without any space in between them).
*/

const char *cgen_util_c_contents[] = { "#include \"util.h\"\n\
\n\
haris_uint8_t haris_read_uint8(unsigned char *b)\n\
{\n\
  return *b;\n\
}\n\
\n\
haris_int8_t haris_read_int8(unsigned char *b)\n\
{\n\
  if (*b & 0x80) \n\
    return -(haris_int8_t)(*b & 0x7F) - 1;\n\
  else\n\
    return (haris_int8_t)haris_read_uint8(b);\n\
}\n\
\n\
haris_uint16_t haris_read_uint16(unsigned char *b)\n\
{\n\
  return (haris_uint16_t)*b << 8 | *(b+1);\n\
}\n\
\n\
haris_int16_t haris_read_int16(unsigned char *b)\n\
{\n\
  if (*b & 0x80)\n\
    return", 
" -((haris_int16_t)(*b & 0x7F) << 8 | (haris_int16_t)*(b+1)) - 1;\n\
  else\n\
    return (haris_int16_t)haris_read_uint16(b);\n\
}\n\
\n\
haris_uint32_t haris_read_uint32(unsigned char *b)\n\
{\n\
  return (haris_uint32_t)*b << 24 | (haris_uint32_t)*(b+1) << 16 |\n\
    (haris_uint32_t)*(b+2) << 8 | *(b+3);\n\
}\n\
\n\
haris_int32_t haris_read_int32(unsigned char *b)\n\
{\n\
  if (*b & 0x80)\n\
    return -((haris_int32_t)(*b & 0x7F) << 24 | (haris_int32_t)*(b+1) << 16 |\n\
             (haris_i", "nt32_t)(*b+2) << 8 | (haris_int32_t)(*b+3)) - 1;\n\
  else\n\
    return (haris_int32_t)haris_read_uint32(b);\n\
}\n\
\n\
haris_uint64_t haris_read_uint64(unsigned char *b)\n\
{\n\
  return (haris_uint64_t)*b << 56 | (haris_uint64_t)*(b+1) << 48 |\n\
    (haris_uint64_t)*(b+2) << 40 | (haris_uint64_t)*(b+3) << 32 |\n\
    (haris_uint64_t)*(b+4) << 24 | (haris_uint64_t)*(b+5) << 16 |\n\
    (haris_uint64_t)*(b+6) << 8 | *(b+7);\n\
}\n\
\n\
haris_int64_t haris_read_int64(unsigned char *b)\n\
{\n\
", 
"  if (*b & 0x80)\n\
    return -((haris_int64_t)(*b & 0x7F) << 56 | (haris_int64_t)*(b+1) << 48 |\n\
             (haris_int64_t)*(b+2) << 40 | (haris_int64_t)*(b+3) << 32 |\n\
             (haris_int64_t)*(b+4) << 24 | (haris_int64_t)*(b+5) << 16 |\n\
             (haris_int64_t)*(b+6) << 8 | (haris_int64_t)*(b+7)) - 1;\n\
  else\n\
    return (haris_int64_t)haris_read_uint64(b);\n\
}\n\
\n\
void haris_write_uint8(unsigned char *b, haris_uint8_t i)\n\
{\n\
  *b = i;\n\
  return;\n\
}\n\
\n\
voi", 
"d haris_write_int8(unsigned char *b, haris_int8_t i)\n\
{\n\
  if (i >= 0)\n\
    *b = (unsigned char)i;\n\
  else {\n\
    *b = (unsigned char)(-i - 1);\n\
    *b |= 0x80;\n\
  }\n\
  return;\n\
}\n\
\n\
void haris_write_uint16(unsigned char *b, haris_uint16_t i)\n\
{\n\
  *b = i >> 8;\n\
  *(b+1) = i;\n\
  return;\n\
}\n\
\n\
void haris_write_int16(unsigned char *b, haris_int16_t i)\n\
{\n\
  if (i >= 0)\n\
    haris_write_uint16(b, (haris_uint16_t)i);\n\
  else {\n\
    haris_write_uint16(b, ", "(haris_uint16_t)(-i - 1));\n\
    *b |= 0x80;\n\
  }\n\
  return;\n\
}\n\
\n\
void haris_write_uint32(unsigned char *b, haris_uint32_t i)\n\
{\n\
  *b = i >> 24;\n\
  *(b+1) = i >> 16;\n\
  *(b+2) = i >> 8;\n\
  *(b+3) = i;\n\
  return;\n\
}\n\
\n\
void haris_write_int32(unsigned char *b, haris_int32_t i)\n\
{\n\
  if (i >= 0)\n\
    haris_write_uint32(b, (haris_uint32_t)i);\n\
  else {\n\
    haris_write_uint32(b, (haris_uint32_t)(-i - 1));\n\
    *b |= 0x80;\n\
  }\n\
  return;\n\
}\n\
\n\
voi", 
"d haris_write_uint64(unsigned char *b, haris_uint64_t i)\n\
{\n\
  *b = i >> 56;\n\
  *(b+1) = i >> 48;\n\
  *(b+2) = i >> 40;\n\
  *(b+3) = i >> 32;\n\
  *(b+4) = i >> 24;\n\
  *(b+5) = i >> 16;\n\
  *(b+6) = i >> 8;\n\
  *(b+7) = i;\n\
  return;\n\
}\n\
\n\
void haris_write_int64(unsigned char *b, haris_int64_t i)\n\
{\n\
  if (i >= 0)\n\
    haris_write_uint64(b, (haris_uint64_t)i);\n\
  else {\n\
    haris_write_uint64(b, (haris_uint64_t)(-i - 1));\n\
    *b |= 0x80;\n\
  }\n\
  return;\n\
}", 
"\n\
\n\
float haris_read_float32(unsigned char *b)\n\
{\n\
  double result;\n\
  haris_int64_t shift;\n\
  haris_uint32_t i = haris_read_uint32(b);\n\
  \n\
  if (i == 0) return 0.0;\n\
  \n\
  result = (i & ((1LL << HARIS_FLOAT32_SIGBITS) - 1));\n\
  result /= (1LL << HARIS_FLOAT32_SIGBITS); \n\
  result += 1.0;\n\
\n\
  shift = ((i >> HARIS_FLOAT32_SIGBITS) & 255) - HARIS_FLOAT32_BIAS;\n\
  while (shift > 0) { result *= 2.0; shift--; }\n\
  while (shift < 0) { result /= 2.0; shift++; }\n\
\n\
", 
"  result *= (i >> 31) & 1 ? -1.0: 1.0;\n\
\n\
  return (float)result;\n\
}\n\
\n\
double haris_read_float64(unsigned char *b)\n\
{\n\
  double result;\n\
  haris_int64_t shift;\n\
  haris_uint64_t i = haris_read_uint64(b);\n\
  \n\
  if (i == 0) return 0.0;\n\
  \n\
  result = (i & (( 1LL << HARIS_FLOAT64_SIGBITS) - 1));\n\
  result /= (1LL << HARIS_FLOAT64_SIGBITS); \n\
  result += 1.0;\n\
\n\
  shift = ((i >> HARIS_FLOAT64_SIGBITS) & 2047) - HARIS_FLOAT64_BIAS;\n\
  while (shift > 0) { result ", 
"*= 2.0; shift--; }\n\
  while (shift < 0) { result /= 2.0; shift++; }\n\
\n\
  result *= (i >> 63) & 1 ? -1.0: 1.0;\n\
\n\
  return result;\n\
}\n\
\n\
void haris_write_float32(unsigned char *b, float f)\n\
{\n\
  double fnorm;\n\
  int shift;\n\
  long sign, exp, significand;\n\
  haris_uint32_t result;\n\
\n\
  if (f == 0.0) {\n\
    haris_write_uint32(b, 0);\n\
    return;\n\
  } \n\
\n\
  if (f < 0) {\n\
    sign = 1; \n\
    fnorm = -f; \n\
  } else { \n\
    sign = 0; \n\
    fnorm = f; \n", 
"\
  }\n\
\n\
  shift = 0;\n\
  while (fnorm >= 2.0) { fnorm /= 2.0; shift++; }\n\
  while (fnorm < 1.0) { fnorm *= 2.0; shift--; }\n\
  fnorm = fnorm - 1.0;\n\
\n\
  significand = fnorm * ((1LL << HARIS_FLOAT32_SIGBITS) + 0.5f);\n\
\n\
  exp = shift + ((1<<7) - 1); \n\
\n\
  result = (sign<<31) | (exp<<23) | significand;\n\
  haris_write_uint32(b, result);\n\
  return;\n\
}\n\
\n\
void haris_write_float64(unsigned char *b, double f)\n\
{\n\
  double fnorm;\n\
  int shift;\n\
  long sign, exp, si", 
"gnificand;\n\
  haris_uint64_t result;\n\
\n\
  if (f == 0.0) {\n\
    haris_write_uint64(b, 0);\n\
  }\n\
\n\
  if (f < 0) {\n\
    sign = 1; \n\
    fnorm = -f; \n\
  } else { \n\
    sign = 0; \n\
    fnorm = f; \n\
  }\n\
\n\
  shift = 0;\n\
  while (fnorm >= 2.0) { fnorm /= 2.0; shift++; }\n\
  while (fnorm < 1.0) { fnorm *= 2.0; shift--; }\n\
  fnorm = fnorm - 1.0;\n\
\n\
  significand = fnorm * ((1LL<<HARIS_FLOAT64_SIGBITS) + 0.5f);\n\
\n\
  exp = shift + ((1<<10) - 1); \n\
\n\
  result =", 
" (sign<<63) | (exp<<52) | significand;\n\
  haris_write_uint64(b, result);\n\
  return;\n\
}\n", 
NULL };

const char *cgen_util_h_contents[] = { "#ifndef __UTIL_H\n\
#define __UTIL_H\n\
\n\
/* In order to generate C code, the utility library needs exact-precision\n\
   unsigned and signed integers. In particular, in order to ensure that we don't\n\
   overflow the native integer containers when parsing Haris messages, we need\n\
   to be certain about the minimum size of our integers. Haris defines the\n\
   typedefs\n\
   haris_intN_t\n\
   and\n\
   haris_uintN_t\n\
   for N in [8, 16, 32, 64]. The intN types must be signed, and the uin", "tN types\n\
   must be unsigned. Each type must have at least N bits (that is, haris_int8_t\n\
   must have at least 8 bits and must be signed). If your system includes \n\
   stdint.h (which it will if you have a standard-conforming C99 compiler), then\n\
   the typedef's automatically generated by the code generator should be \n\
   sufficient, and you should be able to include the generated files in your \n\
   project without changing them. If you do not have stdint.h, then you'll have\n\
  ",
" to manually modify the following 8 typedef's yourself (though that shouldn't\n\
   take more than a minute to do). Make sure to remove the #include directive \n\
   if you do not have stdint.h.\n\
*/\n\
\n\
#include <stdint.h>\n\
\n\
typedef uint_fast8_t    haris_uint8_t;\n\
typedef int_fast8_t     haris_int8_t;\n\
typedef uint_fast16_t   haris_uint16_t;\n\
typedef int_fast16_t    haris_int16_t;\n\
typedef uint_fast32_t   haris_uint32_t;\n\
typedef int_fast32_t    haris_int32_t;\n\
typedef uint", 
"_fast64_t   haris_uint64_t;\n\
typedef int_fast64_t    haris_int64_t;\n\
\n\
/* \"Magic numbers\" for use by float-reading and -writing functions; do not \n\
   modify\n\
*/\n\
\n\
#define HARIS_FLOAT32_SIGBITS 23\n\
#define HARIS_FLOAT32_BIAS    127\n\
#define HARIS_FLOAT64_SIGBITS 52\n\
#define HARIS_FLOAT64_BIAS    1023\n\
\n\
haris_uint8_t haris_read_uint8(unsigned char *);\n\
haris_int8_t haris_read_int8(unsigned char *);\n\
haris_uint16_t haris_read_uint16(unsigned char *);\n\
haris_int16_", 
"t haris_read_int16(unsigned char *);\n\
haris_uint32_t haris_read_uint32(unsigned char *);\n\
haris_int32_t haris_read_int32(unsigned char *);\n\
haris_uint64_t haris_read_uint64(unsigned char *);\n\
haris_int64_t haris_read_int64(unsigned char *);\n\
\n\
void haris_write_uint8(unsigned char *, haris_uint8_t);\n\
void haris_write_int8(unsigned char *, haris_int8_t);\n\
void haris_write_uint16(unsigned char *, haris_uint16_t);\n\
void haris_write_int16(unsigned char *, haris_int16_t);\n\
void har", 
"is_write_uint32(unsigned char *, haris_uint32_t);\n\
void haris_write_int32(unsigned char *, haris_int32_t);\n\
void haris_write_uint64(unsigned char *, haris_uint64_t);\n\
void haris_write_int64(unsigned char *, haris_int64_t);\n\
\n\
float haris_read_float32(unsigned char *);\n\
double haris_read_float64(unsigned char *);\n\
\n\
void haris_write_float32(unsigned char *, float);\n\
void haris_write_float64(unsigned char *, double);\n\
\n\
#endif\n",
NULL };

#endif

