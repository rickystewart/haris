#include "cgenc_util.h"

static CJobStatus write_readint(CJob *);
static CJobStatus write_readuint(CJob *);
static CJobStatus write_writeint(CJob *);
static CJobStatus write_writeuint(CJob *);
static CJobStatus write_readfloat(CJob *);
static CJobStatus write_writefloat(CJob *);

static CJobStatus write_scalar_readers(CJob *);
static CJobStatus write_scalar_reader_function(CJob *);

static CJobStatus write_scalar_writers(CJob *);
static CJobStatus write_scalar_writer_function(CJob *);

static CJobStatus (* const util_writer_functions[])(CJob *) = {
  write_readint, write_readuint, write_writeint, write_writeuint, 
  write_readfloat, write_writefloat,

  write_scalar_readers, write_scalar_reader_function,

  write_scalar_writers, write_scalar_writer_function
};

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_utility_library(CJob *job)
{
  CJobStatus result;
  unsigned i;
  for (i = 0; 
  	   i < sizeof util_writer_functions / 
  		   sizeof util_writer_functions[0];
  	   i ++) {
  	if ((result = util_writer_functions[i](job)) != CJOB_SUCCESS) 
  	  return result;
  }
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

/* ********* SCALAR-WRITING AND -READING FUNCTIONS ********* */
static CJobStatus write_readint(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_int8(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_int8_t *ptr = (haris_int8_t*)_ptr;\n\
  haris_uint8_t uint;\n\
  haris_read_uint8(b, &uint);\n\
  if (*b & 0x80) \n\
    *ptr = -(haris_int8_t)~(uint - 1);\n\
  else\n\
    *ptr = (haris_int8_t)uint;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_int16(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_int16_t *ptr = (haris_int16_t*)_ptr;\n\
  haris_uint16_t uint;\n\
  haris_read_uint16(b, &uint);\n\
  if (*b & 0x80)\n\
    *ptr = -(haris_int16_t)~(uint - 1);\n\
  else\n\
    *ptr = (haris_int16_t)uint;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_int32(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_int32_t *ptr = (haris_int32_t*)_ptr;\n\
  haris_uint32_t uint;\n\
  haris_read_uint32(b, &uint);\n\
  if (*b & 0x80)\n\
    *ptr = -(haris_int32_t)~(uint - 1);\n\
  else\n\
    *ptr = (haris_int32_t)uint;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_int64(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_int64_t *ptr = (haris_int64_t*)_ptr;\n\
  haris_uint64_t uint;\n\
  haris_read_uint64(b, &uint);\n\
  if (*b & 0x80)\n\
    *ptr = -(haris_int64_t)~(uint - 1);\n\
  else\n\
    *ptr = (haris_int64_t)uint;\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_readuint(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_uint8(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_uint8_t *ptr = (haris_uint8_t*)_ptr;\n\
  *ptr = *b;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_uint16(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_uint16_t *ptr = (haris_uint16_t*)_ptr;\n\
  *ptr = ((haris_uint16_t)b[0]) | ((haris_uint16_t)b[1] << 8);\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_uint24(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_uint32_t *ptr = (haris_uint32_t*)_ptr;\n\
  *ptr = ((haris_uint32_t)b[0]) | ((haris_uint32_t)b[1] << 8) |\n\
    ((haris_uint32_t)b[2] << 16);\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_uint32(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_uint32_t *ptr = (haris_uint32_t*)_ptr;\n\
  *ptr = ((haris_uint32_t)b[0]) | ((haris_uint32_t)b[1] << 8) |\n\
    ((haris_uint32_t)b[2] << 16) | ((haris_uint32_t)b[3] << 24);\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_uint64(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_uint64_t *ptr = (haris_uint64_t*)_ptr;\n\
  *ptr = ((haris_uint64_t)b[0]) | ((haris_uint64_t)b[1] << 8) |\n\
    ((haris_uint64_t)b[2] << 16) | ((haris_uint64_t)b[3] << 24) |\n\
    ((haris_uint64_t)b[4] << 32) | ((haris_uint64_t)b[5] << 40) |\n\
    ((haris_uint64_t)b[6] << 48) | ((haris_uint64_t)b[7] << 56);\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_writeint(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_int8(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_int8_t i = *(const haris_int8_t*)_ptr;\n\
  if (i >= 0)\n\
    *b = (unsigned char)i;\n\
  else\n\
    *b = ~(unsigned char)(-i) + (unsigned char)1;\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static void haris_write_int16(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_int16_t i = *(const haris_int16_t*)_ptr;\n\
  haris_uint16_t ui;\n\
  if (i >= 0)\n\
    ui = (haris_uint16_t)i;\n\
  else\n\
    ui = ~(haris_uint16_t)(-i) + 1;\n\
  haris_write_uint16(b, &ui);\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static void haris_write_int32(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_int32_t i = *(const haris_int32_t*)_ptr;\n\
  haris_uint32_t ui;\n\
  if (i >= 0)\n\
    ui = (haris_uint32_t)i;\n\
  else\n\
    ui = ~(haris_uint32_t)(-i) + 1;\n\
  haris_write_uint32(b, &ui);\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static void haris_write_int64(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_int64_t i = *(const haris_int64_t*)_ptr;\n\
  haris_uint64_t ui;\n\
  if (i >= 0)\n\
    ui = (haris_uint64_t)i;\n\
  else\n\
    ui = ~(haris_uint64_t)(-i) + 1;\n\
  haris_write_uint64(b, &ui);\n\
  return;\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_writeuint(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_uint8(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_uint8_t i = *(const haris_uint8_t*)_ptr;\n\
  *b = i;\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_uint16(unsigned char *b, const void *_ptr)\n\
{  \n\
  haris_uint16_t i = *(const haris_uint16_t*)_ptr;\n\
  *b = i;\n\
  *(b+1) = i >> 8;\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_uint24(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_uint32_t i = *(const haris_uint32_t*)_ptr;\n\
  *b = i;\n\
  *(b+1) = i >> 8;\n\
  *(b+2) = i >> 16;\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_uint32(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_uint32_t i = *(const haris_uint32_t*)_ptr;\n\
  *b = i;\n\
  *(b+1) = i >> 8;\n\
  *(b+2) = i >> 16;\n\
  *(b+3) = i >> 24;\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_uint64(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_uint64_t i = *(const haris_uint64_t*)_ptr;\n\
  *b = i;\n\
  *(b+1) = i >> 8;\n\
  *(b+2) = i >> 16;\n\
  *(b+3) = i >> 24;\n\
  *(b+4) = i >> 32;\n\
  *(b+5) = i >> 40;\n\
  *(b+6) = i >> 48;\n\
  *(b+7) = i >> 56;\n\
  return;\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_readfloat(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_float32(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_float32 *ptr = (haris_float32*)_ptr;\n\
  const int float32_sigbits = 23, float32_bias = 127;\n\
  haris_float64 result;\n\
  haris_int64_t shift;\n\
  haris_uint32_t i;\n\
  haris_read_uint32(b, &i);\n\
  \n\
  if (i == 0) { *ptr = 0.0; return; }\n\
  \n\
  result = (i & (((haris_uint32_t)1 << float32_sigbits) - 1));\n\
  result /= ((haris_uint32_t)1 << float32_sigbits); \n\
  result += 1.0;\n\
\n\
  shift = ((i >> float32_sigbits) & 255) - float32_bias;\n\
  while (shift > 0) { result *= 2.0; shift--; }\n\
  while (shift < 0) { result /= 2.0; shift++; }\n\
\n\
  result *= (i >> 31) & 1 ? -1.0: 1.0;\n\
\n\
  *ptr = (haris_float32)result;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_float64(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_float64 result, *ptr = (haris_float64*)_ptr;\n\
  const int float64_sigbits = 52, float64_bias = 1023;\n\
  haris_int64_t shift;\n\
  haris_uint64_t i;\n\
  haris_read_uint64(b, &i);\n\
  \n\
  if (i == 0) { *ptr = 0.0; return; }\n\
  \n\
  result = (i & (((haris_uint64_t)1 << float64_sigbits) - 1));\n\
  result /= ((haris_uint64_t)1 << float64_sigbits); \n\
  result += 1.0;\n\
\n\
  shift = ((i >> float64_sigbits) & 2047) - float64_bias;\n\
  while (shift > 0) { result *= 2.0; shift--; }\n\
  while (shift < 0) { result /= 2.0; shift++; }\n\
\n\
  result *= (i >> 63) & 1 ? -1.0: 1.0;\n\
\n\
  *ptr = result;\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_writefloat(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job,  
"static void haris_write_float32(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_float32 f = *(const haris_float32*)_ptr;\n\
  haris_float64 fnorm;\n\
  int shift;\n\
  const int float32_sigbits = 23;\n\
  long sign, exp, significand;\n\
  haris_uint32_t result;\n\
\n\
  if (f == 0.0) {\n\
    result = 0.0;\n\
    goto Finish;\n\
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
  fnorm = fnorm - 1.0;\n\
\n\
  significand = fnorm * (((haris_uint32_t)1 << float32_sigbits)\n\
                          + 0.5f);\n\
\n\
  exp = shift + ((1<<7) - 1); \n\
\n\
  result = (sign<<31) | (exp<<23) | significand;\n\
  Finish:\n\
  haris_write_uint32(b, &result);\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_float64(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_float64 fnorm, f = *(const haris_float64*)_ptr;\n\
  const int float64_sigbits = 52;\n\
  int shift;\n\
  long sign, exp, significand;\n\
  haris_uint64_t result;\n\
\n\
  if (f == 0.0) {\n\
    result = 0.0;\n\
    goto Finish;\n\
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
  significand = fnorm * (((haris_uint64_t)1<<float64_sigbits)\n\
                         + 0.5f);\n\
\n\
  exp = shift + ((1<<10) - 1); \n\
\n\
  result = (sign<<63) | (exp<<52) | significand;\n\
  Finish:\n\
  haris_write_uint64(b, &result);\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* READING ********* */

/* Write the array of scalar-reading functions to the output file; as 
   above, this array is keyed by HarisScalarType.
*/
static CJobStatus write_scalar_readers(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, 
"static\n\
void (* const haris_lib_scalar_readers[])(const unsigned char *, void *) = {\n\
  haris_read_uint8, haris_read_int8, haris_read_uint16, haris_read_int16,\n\
  haris_read_uint32, haris_read_int32, haris_read_uint64, haris_read_int64,\n\
  haris_read_float32, haris_read_float64\n\
};\n\n");
  return CJOB_SUCCESS;
}

/* Writes the scalar-reading function to the output file. Parameters are
   analogous to the above, except the scalar is READ from the Haris buffer.
*/
static CJobStatus write_scalar_reader_function(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_lib_read_scalar(const unsigned char *message, void *src,\n\
                                   HarisScalarType type)\n\
{\n  haris_lib_scalar_readers[type](message, src);\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* WRITING ********* */

static CJobStatus write_scalar_writers(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, 
"static\n\
void (* const haris_lib_scalar_writers[])(unsigned char *, const void *) = {\n\
  haris_write_uint8, haris_write_int8, haris_write_uint16, haris_write_int16,\n\
  haris_write_uint32, haris_write_int32, haris_write_uint64, haris_write_int64,\n\
  haris_write_float32, haris_write_float64\n\
};\n\n");
  return CJOB_SUCCESS;
}

/* Writes the scalar-writing function to the output file. This function 
   consumes an unsigned char * "message" parameter, which is a pointer
   to an in-memory Haris message buffer, a void pointer to a "src" 
   parameter, whose type is dictated by the HarisScalarType type
   parameter. For instance, if type == HARIS_SCALAR_INT8, src
   should point to a haris_int8_t object. The scalar will be
   WRITTEN to the Haris buffer.
*/
static CJobStatus write_scalar_writer_function(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_lib_write_scalar(unsigned char *message, const void *src,\n\
                                    HarisScalarType type)\n\
{\n  haris_lib_scalar_writers[type](message, src);\n}\n\n");
  return CJOB_SUCCESS;
}
