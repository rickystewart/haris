#include "cgenu.h"

static CJobStatus write_util_h_top(CJob *job, FILE *out);
static CJobStatus write_util_h_typedefs(CJob *job, FILE *out);
static CJobStatus write_util_h_defines(CJob *job, FILE *out);
static CJobStatus write_util_h_prototypes(CJob *job, FILE *out);
static CJobStatus write_util_h_bottom(CJob *job, FILE *out);

static CJobStatus write_util_c_top(CJob *job, FILE *out);
static CJobStatus write_util_c_readint(CJob *job, FILE *out);
static CJobStatus write_util_c_readuint(CJob *job, FILE *out);
static CJobStatus write_util_c_writeint(CJob *job, FILE *out);
static CJobStatus write_util_c_writeuint(CJob *job, FILE *out);
static CJobStatus write_util_c_readfloat(CJob *job, FILE *out);
static CJobStatus write_util_c_writefloat(CJob *job, FILE *out);

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_utility_library_header(CJob *job, FILE *out)
{
  CJobStatus result;
  if ((result = write_util_h_top(job, out)) != CJOB_SUCCESS ||
      (result = write_util_h_typedefs(job, out)) != CJOB_SUCCESS ||
      (result = write_util_h_defines(job, out)) != CJOB_SUCCESS ||
      (result = write_util_h_prototypes(job, out)) != CJOB_SUCCESS ||
      (result = write_util_h_bottom(job, out)) != CJOB_SUCCESS)
    return result;
  return CJOB_SUCCESS;
}

CJobStatus write_utility_library_source(CJob *job, FILE *out)
{
  CJobStatus result;
  if ((result = write_util_c_top(job, out)) != CJOB_SUCCESS ||
      (result = write_util_c_readint(job, out)) != CJOB_SUCCESS ||
      (result = write_util_c_readuint(job, out)) != CJOB_SUCCESS ||
      (result = write_util_c_writeint(job, out)) != CJOB_SUCCESS ||
      (result = write_util_c_writeuint(job, out)) != CJOB_SUCCESS ||
      (result = write_util_c_readfloat(job, out)) != CJOB_SUCCESS ||
      (result = write_util_c_writefloat(job, out)) != CJOB_SUCCESS)
    return result;
  return CJOB_SUCCESS; 
}

/* =============================STATIC FUNCTIONS============================= */

/* These functions generally contain literal strings that will be written
   into the generated source files; these are the utility functions that
   do not change depending on the schema. The strings are all no longer
   than 500 characters long, in order to honor an obscure part of the C89
   that says that 512 is the maximum length that must be supported by a
   conformant C implementation. I used a script to split up the strings,
   and so they're generally split across odd, arbitrary boundaries. */

static CJobStatus write_util_h_top(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s%s%s", "#ifndef __HARIS_UTIL_H\n\
#define __HARIS_UTIL_H\n\
\n\
/* In order to generate C code, the utility library needs exact-precision\n\
   unsigned and signed integers. In particular, in order to ensure that we don't\n\
   overflow the native integer containers when parsing Haris messages, we need\n\
   to be certain about the minimum size of our integers. Haris defines the\n\
   typedefs\n\
   haris_intN_t\n\
   and\n\
   haris_uintN_t\n\
   for N in [8, 16, 32, 64]. The intN types must be signed, and the uin",
"tN types\n\
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
\n\
   These type definitions trade time for space; that is, they use the fastest\n\
   possible types with those sizes rather than the smallest. This means that\n\
   the in-memory representation of a structure might be larger than is \n\
   technically necessary to store the number. If you wish to u",
"se less space\n\
   in-memory in exchange for a potentially longer running time, use the\n\
   [u]int_leastN_t types rather than the [u]int_fastN_t types.\n\
*/\n\
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS; 
}

static CJobStatus write_util_h_typedefs(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s", "#include <stdint.h>\n\
\n\
typedef uint_fast8_t    haris_uint8_t;\n\
typedef int_fast8_t     haris_int8_t;\n\
typedef uint_fast16_t   haris_uint16_t;\n\
typedef int_fast16_t    haris_int16_t;\n\
typedef uint_fast32_t   haris_uint32_t;\n\
typedef int_fast32_t    haris_int32_t;\n\
typedef uint_fast64_t   haris_uint64_t;\n\
typedef int_fast64_t    haris_int64_t;\n\
\n\
typedef float           haris_float32;\n\
typedef double          haris_float64;\n\
\n\
typedef enum {\n\
  HARIS_SUCCESS, HARIS_ST",
"RUCTURE_ERROR, HARIS_DEPTH_ERROR, HARIS_SIZE_ERROR,\n\
  HARIS_INPUT_ERROR, HARIS_MEM_ERROR\n\
} HarisStatus;\n\
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_h_defines(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s", "/* Changeable size limits for error-checking. You can freely modify these if\n\
   you would like your Haris client to be able to process larger or deeper\n\
   messages. \n\
*/\n\
\n\
#define HARIS_DEPTH_LIMIT 64\n\
#define HARIS_MESSAGE_SIZE_LIMIT 1000000000\n\
\n\
/* \"Magic numbers\" for use by float-reading and -writing functions; do not \n\
   modify\n\
*/\n\
\n\
#define HARIS_FLOAT32_SIGBITS 23\n\
#define HARIS_FLOAT32_BIAS    127\n\
#define HARIS_FLOAT64_SIGBITS 52\n\
#define HARIS_FLOAT",
"64_BIAS    1023\n\
\n\
#define HARIS_ASSERT(cond, err) if (!(cond)) return HARIS_ ## err ## _ERROR\n\
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_h_prototypes(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s%s", "haris_uint8_t haris_read_uint8(unsigned char *);\n\
haris_int8_t haris_read_int8(unsigned char *);\n\
haris_uint16_t haris_read_uint16(unsigned char *);\n\
haris_int16_t haris_read_int16(unsigned char *);\n\
haris_uint32_t haris_read_uint24(unsigned char *);\n\
haris_uint32_t haris_read_uint32(unsigned char *);\n\
haris_int32_t haris_read_int32(unsigned char *);\n\
haris_uint64_t haris_read_uint64(unsigned char *);\n\
haris_int64_t haris_read_int64(unsigned char *);\n\
\n\
void haris_write_uint8",
"(unsigned char *, haris_uint8_t);\n\
void haris_write_int8(unsigned char *, haris_int8_t);\n\
void haris_write_uint16(unsigned char *, haris_uint16_t);\n\
void haris_write_int16(unsigned char *, haris_int16_t);\n\
void haris_write_uint24(unsigned char *, haris_uint24_t);\n\
void haris_write_uint32(unsigned char *, haris_uint32_t);\n\
void haris_write_int32(unsigned char *, haris_int32_t);\n\
void haris_write_uint64(unsigned char *, haris_uint64_t);\n\
void haris_write_int64(unsigned char *, hari",
"s_int64_t);\n\
\n\
haris_float32 haris_read_float32(unsigned char *);\n\
haris_float64 haris_read_float64(unsigned char *);\n\
\n\
void haris_write_float32(unsigned char *, haris_float32);\n\
void haris_write_float64(unsigned char *, haris_float64);\n\
\n\
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_h_bottom(CJob *job, FILE *out)
{
  if (fprintf(out, "\n#endif\n\n") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_c_top(CJob *job, FILE *out)
{
  if (fprintf(out, "#include \"%s.h\"\n\n", job->output) < 0)
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_c_readint(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s%s", "haris_int8_t haris_read_int8(unsigned char *b)\n\
{\n\
  if (*b & 0x80) \n\
    return -(haris_int8_t)(*b & 0x7F) - 1;\n\
  else\n\
    return (haris_int8_t)haris_read_uint8(b);\n\
}\n\
\n\
haris_int16_t haris_read_int16(unsigned char *b)\n\
{\n\
  if (*b & 0x80)\n\
    return -((haris_int16_t)(*b & 0x7F) << 8 | (haris_int16_t)*(b+1)) - 1;\n\
  else\n\
    return (haris_int16_t)haris_read_uint16(b);\n\
}\n\
\n\
haris_int32_t haris_read_int32(unsigned char *b)\n\
{\n\
  if (*b & 0x80)\n\
    retu",
"rn -((haris_int32_t)(*b & 0x7F) << 24 | (haris_int32_t)*(b+1) << 16 |\n\
             (haris_int32_t)(*b+2) << 8 | (haris_int32_t)(*b+3)) - 1;\n\
  else\n\
    return (haris_int32_t)haris_read_uint32(b);\n\
}\n\
\n\
haris_int64_t haris_read_int64(unsigned char *b)\n\
{\n\
  if (*b & 0x80)\n\
    return -((haris_int64_t)(*b & 0x7F) << 56 | (haris_int64_t)*(b+1) << 48 |\n\
             (haris_int64_t)*(b+2) << 40 | (haris_int64_t)*(b+3) << 32 |\n\
             (haris_int64_t)*(b+4) << 24 | (haris_",
"int64_t)*(b+5) << 16 |\n\
             (haris_int64_t)*(b+6) << 8 | (haris_int64_t)*(b+7)) - 1;\n\
  else\n\
    return (haris_int64_t)haris_read_uint64(b);\n\
}\n\
\n\
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_c_readuint(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s", "haris_uint8_t haris_read_uint8(unsigned char *b)\n\
{\n\
  return *b;\n\
}\n\
\n\
haris_uint16_t haris_read_uint16(unsigned char *b)\n\
{\n\
  return (haris_uint16_t)*b << 8 | *(b+1);\n\
}\n\
\n\
\n\
haris_uint32_t haris_read_uint24(unsigned char *b)\n\
{\n\
  return (haris_uint32_t)*b << 16 | (haris_uint32_t)*(b+1) << 8 |\n\
    *(b+2);\n\
}\n\
\n\
haris_uint32_t haris_read_uint32(unsigned char *b)\n\
{\n\
  return (haris_uint32_t)*b << 24 | (haris_uint32_t)*(b+1) << 16 |\n\
    (haris_uint32_t",
")*(b+2) << 8 | *(b+3);\n\
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
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_c_writeint(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s", "void haris_write_int8(unsigned char *b, haris_int8_t i)\n\
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
void haris_write_int16(unsigned char *b, haris_int16_t i)\n\
{\n\
  if (i >= 0)\n\
    haris_write_uint16(b, (haris_uint16_t)i);\n\
  else {\n\
    haris_write_uint16(b, (haris_uint16_t)(-i - 1));\n\
    *b |= 0x80;\n\
  }\n\
  return;\n\
}\n\
\n\
void haris_write_int32(unsigned char *b, har",
"is_int32_t i)\n\
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
void haris_write_int64(unsigned char *b, haris_int64_t i)\n\
{\n\
  if (i >= 0)\n\
    haris_write_uint64(b, (haris_uint64_t)i);\n\
  else {\n\
    haris_write_uint64(b, (haris_uint64_t)(-i - 1));\n\
    *b |= 0x80;\n\
  }\n\
  return;\n\
}\n\
\n\
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_c_writeuint(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s", "void haris_write_uint8(unsigned char *b, haris_uint8_t i)\n\
{\n\
  *b = i;\n\
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
void haris_write_uint24(unsigned char *b, haris_uint24_t i)\n\
{\n\
  *b = i >> 16;\n\
  *(b+1) = i >> 8;\n\
  *(b+2) = i;\n\
  return;\n\
}\n\
\n\
void haris_write_uint32(unsigned char *b, haris_uint32_t i)\n\
{\n\
  *b = i >> 24;\n\
  *(b+1) = i >> 16;\n\
  *(b+2) = i >> ",
"8;\n\
  *(b+3) = i;\n\
  return;\n\
}\n\
\n\
void haris_write_uint64(unsigned char *b, haris_uint64_t i)\n\
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
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_c_readfloat(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s%s", "haris_float32 haris_read_float32(unsigned char *b)\n\
{\n\
  haris_float64 result;\n\
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
  while (shift < 0) { result /= 2.0; shift++; }",
"n\
\n\
  result *= (i >> 31) & 1 ? -1.0: 1.0;\n\
\n\
  return (haris_float32)result;\n\
}\n\
\n\
haris_float64 haris_read_float64(unsigned char *b)\n\
{\n\
  haris_float64 result;\n\
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
",
"  while (shift > 0) { result *= 2.0; shift--; }\n\
  while (shift < 0) { result /= 2.0; shift++; }\n\
\n\
  result *= (i >> 63) & 1 ? -1.0: 1.0;\n\
\n\
  return result;\n\
}\n\
\n\
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_util_c_writefloat(CJob *job, FILE *out)
{
  if (fprintf(out, "%s%s%s", "void haris_write_float32(unsigned char *b, haris_float32 f)\n\
{\n\
  haris_float64 fnorm;\n\
  int shift;\n\
  long sign, exp, significand;\n\
  haris_uint32_t result;\n\
\n\
  if (f == 0.0) {\n\
    haris_write_uint32(b, 0U);\n\
    return;\n\
  } \n\
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
  ",
"fnorm = fnorm - 1.0;\n\
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
void haris_write_float64(unsigned char *b, haris_float64 f)\n\
{\n\
  haris_float64 fnorm;\n\
  int shift;\n\
  long sign, exp, significand;\n\
  haris_uint64_t result;\n\
\n\
  if (f == 0.0) {\n\
    haris_write_uint64(b, 0U);\n\
    return;\n\
  }\n",
"\
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
  result = (sign<<63) | (exp<<52) | significand;\n\
  haris_write_uint64(b, result);\n\
  return;\n\
}\n\
\n\
") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}
