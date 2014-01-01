#include "cgenc_core.h"

/* Functions relating to the public constructors, destructors, and
   initializers */

static CJobStatus write_readint(CJob *job);
static CJobStatus write_readuint(CJob *job);
static CJobStatus write_writeint(CJob *job);
static CJobStatus write_writeuint(CJob *job);
static CJobStatus write_readfloat(CJob *job);
static CJobStatus write_writefloat(CJob *job);

static CJobStatus write_public_constructor(CJob *, ParsedStruct *);
static CJobStatus write_general_constructor(CJob *);

static CJobStatus write_public_destructor(CJob *, ParsedStruct *);
static CJobStatus write_general_destructor(CJob *);

static CJobStatus write_public_initializers(CJob *, ParsedStruct *);
static CJobStatus write_init_list(CJob *, ParsedStruct *, int);
static CJobStatus write_general_init_list_member(CJob *);
static CJobStatus write_init_struct(CJob *, ParsedStruct *, int);
static CJobStatus write_general_init_struct_member(CJob *);

static CJobStatus write_reflective_arrays(CJob *);

static CJobStatus write_in_memory_scalar_sizes(CJob *);
static CJobStatus write_message_scalar_sizes(CJob *);
static CJobStatus write_message_bit_patterns(CJob *);
static CJobStatus write_scalar_readers(CJob *);
static CJobStatus write_scalar_writers(CJob *);

static CJobStatus write_scalar_writer_function(CJob *job);
static CJobStatus write_scalar_reader_function(CJob *job);

static CJobStatus write_core_wfuncs(CJob *);
static CJobStatus write_core_rfuncs(CJob *);
static CJobStatus write_core_size(CJob *);

static CJobStatus (* const general_core_writer_functions[])(CJob *) = {
  write_readint, write_readuint, write_writeint, write_writeuint,
  write_readfloat, write_writefloat,
  write_in_memory_scalar_sizes, write_message_scalar_sizes, 
  write_scalar_readers, write_scalar_writers, write_core_wfuncs,
  write_message_bit_patterns,
  write_scalar_writer_function, write_scalar_reader_function,
  write_general_constructor, write_general_destructor, 
  write_general_init_list_member, write_general_init_struct_member,
  write_core_rfuncs, write_core_size
};

/* =============================PUBLIC INTERFACE============================= */

/* As the header file says, the public functions are 
   S *S_create(void);
   void S_destroy(S *);
   HarisStatus S_init_F(S *, haris_uint32_t);
     ... for every list field F in S and
   HarisStatus S_init_F(S *);
     ... for every structure field F in S.
*/
CJobStatus write_source_public_funcs(CJob *job)
{
  CJobStatus result;
  int i;
  ParsedStruct *strct;
  for (i=0; i < job->schema->num_structs; i++) {
    strct = &job->schema->structs[i];
    if ((result = write_public_constructor(job, strct)) != CJOB_SUCCESS ||
        (result = write_public_destructor(job, strct)) != CJOB_SUCCESS ||
        (result = write_public_initializers(job, strct)) != CJOB_SUCCESS)
      return result;
  }
  return CJOB_SUCCESS;
}

/* Write all the core functions (whose static definitions are given above)
   to the given file.
*/
CJobStatus write_source_core_funcs(CJob *job)
{
  CJobStatus result;
  unsigned i;
  if ((result = write_reflective_arrays(job)) != CJOB_SUCCESS)
    return result;
  for (i = 0; i < sizeof general_core_writer_functions / 
                  sizeof general_core_writer_functions[0]; i++)
    if ((result = general_core_writer_functions[i](job)) != CJOB_SUCCESS)
      return result;
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

/* ********* SCALAR-WRITING AND -READING FUNCTIONS ********* */
static CJobStatus write_readint(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, "static void haris_read_int8(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_int8_t *ptr = (haris_int8_t*)_ptr;\n\
  haris_uint8_t uint;\n\
  haris_read_uint8(b, &uint);\n\
  if (*b & 0x80) \n\
    *ptr = -(haris_int8_t)~(uint - 1);\n\
  else\n\
    *ptr = (haris_int8_t)uint;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, "static void haris_read_int16(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_int16_t *ptr = (haris_int16_t*)_ptr;\n\
  haris_uint16_t uint;\n\
  haris_read_uint16(b, &uint);\n\
  if (*b & 0x80)\n\
    *ptr = -(haris_int16_t)~(uint - 1);\n\
  else\n\
    *ptr = (haris_int16_t)uint;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, "static void haris_read_int32(const unsigned char *b, void *_ptr)\n\
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
  *ptr = (haris_uint16_t)*b << 8 | *(b+1);\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_uint24(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_uint32_t *ptr = (haris_uint32_t*)_ptr;\n\
  *ptr = (haris_uint32_t)*b << 16 | (haris_uint32_t)*(b+1) << 8 |\n\
    *(b+2);\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_uint32(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_uint32_t *ptr = (haris_uint32_t*)_ptr;\n\
  *ptr = (haris_uint32_t)*b << 24 | (haris_uint32_t)*(b+1) << 16 |\n\
    (haris_uint32_t)*(b+2) << 8 | *(b+3);\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_read_uint64(const unsigned char *b, void *_ptr)\n\
{\n\
  haris_uint64_t *ptr = (haris_uint64_t*)_ptr;\n\
  *ptr = (haris_uint64_t)*b << 56 | (haris_uint64_t)*(b+1) << 48 |\n\
    (haris_uint64_t)*(b+2) << 40 | (haris_uint64_t)*(b+3) << 32 |\n\
    (haris_uint64_t)*(b+4) << 24 | (haris_uint64_t)*(b+5) << 16 |\n\
    (haris_uint64_t)*(b+6) << 8 | *(b+7);\n\
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
  *b = i >> 8;\n\
  *(b+1) = i;\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_uint24(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_uint32_t i = *(const haris_uint32_t*)_ptr;\n\
  *b = i >> 16;\n\
  *(b+1) = i >> 8;\n\
  *(b+2) = i;\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_uint32(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_uint32_t i = *(const haris_uint32_t*)_ptr;\n\
  *b = i >> 24;\n\
  *(b+1) = i >> 16;\n\
  *(b+2) = i >> 8;\n\
  *(b+3) = i;\n\
  return;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_write_uint64(unsigned char *b, const void *_ptr)\n\
{\n\
  haris_uint64_t i = *(const haris_uint64_t*)_ptr;\n\
  *b = i >> 56;\n\
  *(b+1) = i >> 48;\n\
  *(b+2) = i >> 40;\n\
  *(b+3) = i >> 32;\n\
  *(b+4) = i >> 24;\n\
  *(b+5) = i >> 16;\n\
  *(b+6) = i >> 8;\n\
  *(b+7) = i;\n\
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
  haris_float64 result;\n\
  haris_int64_t shift;\n\
  haris_uint32_t i;\n\
  haris_read_uint32(b, &i);\n\
  \n\
  if (i == 0) { *ptr = 0.0; return; }\n\
  \n\
  result = (i & ((1LL << HARIS_FLOAT32_SIGBITS) - 1));\n\
  result /= (1LL << HARIS_FLOAT32_SIGBITS); \n\
  result += 1.0;\n\
\n\
  shift = ((i >> HARIS_FLOAT32_SIGBITS) & 255) - HARIS_FLOAT32_BIAS;\n\
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
  haris_int64_t shift;\n\
  haris_uint64_t i;\n\
  haris_read_uint64(b, &i);\n\
  \n\
  if (i == 0) { *ptr = 0.0; return; }\n\
  \n\
  result = (i & (( 1LL << HARIS_FLOAT64_SIGBITS) - 1));\n\
  result /= (1LL << HARIS_FLOAT64_SIGBITS); \n\
  result += 1.0;\n\
\n\
  shift = ((i >> HARIS_FLOAT64_SIGBITS) & 2047) - HARIS_FLOAT64_BIAS;\n\
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
  significand = fnorm * ((1LL << HARIS_FLOAT32_SIGBITS) + 0.5f);\n\
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
  significand = fnorm * ((1LL<<HARIS_FLOAT64_SIGBITS) + 0.5f);\n\
\n\
  exp = shift + ((1<<10) - 1); \n\
\n\
  result = (sign<<63) | (exp<<52) | significand;\n\
  Finish:\n\
  haris_write_uint64(b, &result);\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* ARRAYS ********* */

static const char *scalar_enumerated_name(ScalarTag type)
{
  switch (type) {
  case SCALAR_UINT8:
  case SCALAR_ENUM:
  case SCALAR_BOOL:
    return "HARIS_SCALAR_UINT8";
  case SCALAR_INT8:
    return "HARIS_SCALAR_INT8";
  case SCALAR_UINT16:
    return "HARIS_SCALAR_UINT16";
  case SCALAR_INT16:
    return "HARIS_SCALAR_INT16";
  case SCALAR_UINT32:
    return "HARIS_SCALAR_UINT32";
  case SCALAR_INT32:
    return "HARIS_SCALAR_INT32";
  case SCALAR_UINT64:
    return "HARIS_SCALAR_UINT64";
  case SCALAR_INT64:
    return "HARIS_SCALAR_INT64";
  case SCALAR_FLOAT32:
    return "HARIS_SCALAR_FLOAT32";
  case SCALAR_FLOAT64:
    return "HARIS_SCALAR_FLOAT64";
  default:
    return NULL;
  }
}

static const char *child_enumerated_name(ChildTag type)
{
  switch (type) {
  case CHILD_TEXT:
    return "HARIS_CHILD_TEXT";
  case CHILD_STRUCT:
    return "HARIS_CHILD_STRUCT";
  case CHILD_SCALAR_LIST:
    return "HARIS_CHILD_SCALAR_LIST";
  case CHILD_STRUCT_LIST:
    return "HARIS_CHILD_STRUCT_LIST";
  default:
    return NULL;
  }
}

static CJobStatus write_reflective_scalar_array(CJob *job, ParsedStruct *strct)
{
  int i;
  if (strct->num_scalars == 0) {
    CJOB_FMT_SOURCE_STRING(job, "#define %s%s_lib_scalars NULL\n\n", 
                           job->prefix, strct->name);
    return CJOB_SUCCESS;
  }
  CJOB_FMT_SOURCE_STRING(job, 
"static const HarisScalar %s%s_lib_scalars[] = {\n", job->prefix, strct->name);
  for (i = 0; i < strct->num_scalars; i ++)
    CJOB_FMT_SOURCE_STRING(job, "  { offsetof(%s%s, %s), %s }%s\n",
                           job->prefix, strct->name, strct->scalars[i].name, 
                           scalar_enumerated_name(strct->scalars[i].type.tag),
                           (i + 1 >= strct->num_scalars ? "" : ","));
  CJOB_FMT_SOURCE_STRING(job, "};\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_reflective_child_array(CJob *job, ParsedStruct *strct)
{
  int i;
  ChildField *child;
  if (strct->num_children == 0) {
    CJOB_FMT_SOURCE_STRING(job, "#define %s%s_lib_children NULL\n\n",
                           job->prefix, strct->name);
    return CJOB_SUCCESS;
  }
  CJOB_FMT_SOURCE_STRING(job, "static const HarisChild %s%s_lib_children[] = {\n", 
                         job->prefix, strct->name);
  for (i = 0; i < strct->num_children; i ++) {
    child = &strct->children[i];
    CJOB_FMT_SOURCE_STRING(job, 
"  { offsetof(%s%s, _%s%s), %d, %s, ",
                           job->prefix, strct->name, child->name,
                           child->tag == CHILD_STRUCT ? "" : "_info", 
                           child->nullable, 
                           child->tag == CHILD_SCALAR_LIST ? 
                           scalar_enumerated_name(child->type.scalar_list.tag) :
                           "HARIS_SCALAR_BLANK");
    if (child->tag == CHILD_STRUCT_LIST || child->tag == CHILD_STRUCT) {
      CJOB_FMT_SOURCE_STRING(job, "&haris_lib_structures[%d], ",
                             child->tag == CHILD_STRUCT_LIST ?
                             child->type.struct_list->schema_index :
                             child->type.strct->schema_index);
    } else {
      CJOB_FMT_SOURCE_STRING(job, "NULL, ");
    }
    CJOB_FMT_SOURCE_STRING(job, "%s }%s\n", 
                           child_enumerated_name(child->tag), 
                           (i + 1 >= strct->num_children ? "" : ","));
  }
  CJOB_FMT_SOURCE_STRING(job, "};\n\n");
  return CJOB_SUCCESS;
}

/* Write the reflective structure arrays to the output file;
   each structure has an entry in the array describing its makeup. Each 
   structure's position in the array is determined by its position in
   the compiled in-memory schema that the Haris tool generates, which is known
   for a fact at compile time.
*/
static CJobStatus write_reflective_arrays(CJob *job)
{
  int i;
  ParsedStruct *strct;
  CJobStatus result;
  CJOB_FMT_SOURCE_STRING(job, 
"static const HarisStructureInfo haris_lib_structures[%d];\n\n",
                         job->schema->num_structs);
  for (i = 0; i < job->schema->num_structs; i ++) {
    strct = &job->schema->structs[i];
    if ((result = write_reflective_scalar_array(job, strct)) != CJOB_SUCCESS)
      return result;
    if ((result = write_reflective_child_array(job, strct)) != CJOB_SUCCESS)
      return result;
  }
  CJOB_FMT_SOURCE_STRING(job, 
"static const HarisStructureInfo haris_lib_structures[] = {\n");
  for (i = 0; i < job->schema->num_structs; i ++) {
    strct = &job->schema->structs[i];
    CJOB_FMT_SOURCE_STRING(job, 
"  { %d, %s%s_lib_scalars, %d, %s%s_lib_children, %d, sizeof(%s%s) }%s\n",
                           strct->num_scalars, job->prefix, strct->name, 
                           strct->num_children, job->prefix, 
                           strct->name, strct->offset, job->prefix, 
                           strct->name, 
                           (i + 1 >= job->schema->num_structs ? "" : ","));
  }
  CJOB_FMT_SOURCE_STRING(job, "};\n\n");
  return CJOB_SUCCESS;
}

/* Write the array of in-memory C scalar sizes to the output file.
   This array is keyed by the HarisScalarType enumerators. 
*/
static CJobStatus write_in_memory_scalar_sizes(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, 
"static const size_t haris_lib_in_memory_scalar_sizes[] = {\n\
  sizeof(haris_uint8_t), sizeof(haris_int8_t), sizeof(haris_uint16_t),\n\
  sizeof(haris_int16_t), sizeof(haris_uint32_t), sizeof(haris_int32_t),\n\
  sizeof(haris_uint64_t), sizeof(haris_int64_t), sizeof(haris_float32),\n\
  sizeof(haris_float64)\n};\n\n");
  return CJOB_SUCCESS;
}

/* Write the array of in-message scalar sizes to the output file. 
   As above, this array is also keyed by HarisScalarType.
*/
static CJobStatus write_message_scalar_sizes(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, 
"static const size_t haris_lib_message_scalar_sizes[] = {\n\
  1, 1, 2, 2, 4, 4, 8, 8, 4, 8\n\
};\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_message_bit_patterns(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, 
"static const size_t haris_lib_scalar_bit_patterns[] = {\n\
  0, 0, 1, 1, 2, 2, 3, 3, 2, 3\n\
};\n\n");
  return CJOB_SUCCESS;
}

/* Write the array of scalar-reading functions to the output file; as 
   above, this array is keyed by HarisScalarType.
*/
static CJobStatus write_scalar_readers(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, 
"static void (* const haris_lib_scalar_readers[])(const unsigned char *, void *) = {\n\
  haris_read_uint8, haris_read_int8, haris_read_uint16, haris_read_int16,\n\
  haris_read_uint32, haris_read_int32, haris_read_uint64, haris_read_int64,\n\
  haris_read_float32, haris_read_float64\n\
};\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_scalar_writers(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, 
"static void (* const haris_lib_scalar_writers[])(unsigned char *, const void *) = {\n\
  haris_write_uint8, haris_write_int8, haris_write_uint16, haris_write_int16,\n\
  haris_write_uint32, haris_write_int32, haris_write_uint64, haris_write_int64,\n\
  haris_write_float32, haris_write_float64\n\
};\n\n");
  return CJOB_SUCCESS;
}

/* ********* CONSTRUCTOR ********* */

/* Write the public constructor for the given structure to the given file. */
static CJobStatus write_public_constructor(CJob *job, ParsedStruct *strct)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job, "%s%s *%s%s_create(void) {\n\
  return (%s%s*)haris_lib_create(&haris_lib_structures[%d]);\n}\n\n",
               prefix, name, prefix, name, prefix, name, strct->schema_index);
  return CJOB_SUCCESS;
}

/* Write the GENERAL constructor to the given file. This is a function
   that consumes a HarisStructureInfo pointer and returns a new structure
   of the given type as a void*.
*/
static CJobStatus write_general_constructor(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void *haris_lib_create(const HarisStructureInfo *info)\n\
{\n\
  void *new = malloc(info->size_of);\n\
  if (!new) return NULL;\n\
  return haris_lib_create_contents(new, info);\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void *haris_lib_create_contents(void *new,\n\
                                        const HarisStructureInfo *info)\n\
{\n\
  int i;\n\
  HarisListInfo *list_info;\n\
  *((char*)new) = 0;\n\
  for (i = 0; i < info->num_children; i++) {\n\
    switch (info->children[i].child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_CHILD_SCALAR_LIST:\n\
    case HARIS_CHILD_STRUCT_LIST:\n\
      list_info = (HarisListInfo*)((char*)new + info->children[i].offset);\n\
      list_info->alloc = list_info->len = 0;\n\
      /* Intentional break omission */\n\
    case HARIS_CHILD_STRUCT:\n\
      *((void**)((char*)new + info->children[i].offset)) = NULL;\n\
      break;\n\
    }\n\
  }\n\
  return new;\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* DESTRUCTOR ********* */

/* Writes the destructor for the given structure to the output file. */
static CJobStatus write_public_destructor(CJob *job, ParsedStruct *strct)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job, "void %s%s_destroy(%s%s *strct)\n\
{\n\
  haris_lib_destroy((void*)strct, &haris_lib_structures[%d]);\n\
}\n\n",
              prefix, name, prefix, name,
              strct->schema_index);
  return CJOB_SUCCESS;
}

/* Writes the GENERAL destructor to the given file. This is a 
   function that consumes a void pointer to an in-memory C structure,
   as well as a HarisStructureInfo that describes the makeup of the 
   C structure, and destroys it.
*/
static CJobStatus write_general_destructor(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_lib_destroy_contents(void *ptr, const HarisStructureInfo *info)\n\
{\n\
  haris_uint32_t j, alloced;\n\
  int i;\n\
  HarisListInfo *list_info;\n\
  const HarisStructureInfo *child_structure;\n\
  const HarisChild *child;\n\
  for (i = 0; i < info->num_children; i++) {\n\
    child = &info->children[i];\n\
    list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
    switch (info->children[i].child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_CHILD_SCALAR_LIST:\n\
      if (list_info->alloc > 0)\n\
        free(list_info->ptr);\n\
      break;\n\
    case HARIS_CHILD_STRUCT_LIST:\n\
      alloced = ((HarisListInfo*)((char*)ptr + child->offset))->alloc;\n\
      child_structure = child->struct_element;\n\
      for (j = 0; j < alloced; j++)\n\
        haris_lib_destroy_contents((char*)list_info->ptr +\n\
                                     j * child_structure->size_of,\n\
                                   child_structure);\n\
      /* Intentional break omission */\n\
    case HARIS_CHILD_STRUCT:\n\
      free(list_info->ptr);\n\
      break;\n\
    }\n\
  }\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void haris_lib_destroy(void *ptr, const HarisStructureInfo *info)\n\
{\n\
  haris_lib_destroy_contents(ptr, info);\n\
  free(ptr);\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* INITIALIZERS ********* */

/* Write all public initializer functions to the output file. */
static CJobStatus write_public_initializers(CJob *job, ParsedStruct *strct)
{
  int i;
  CJobStatus result;
  for (i=0; i < strct->num_children; i++) {
    switch (strct->children[i].tag) {
      case CHILD_TEXT:
      case CHILD_SCALAR_LIST:
      case CHILD_STRUCT_LIST:
        if ((result = write_init_list(job, strct, i)) != 
             CJOB_SUCCESS) return result;
        break;
      case CHILD_STRUCT:
        if ((result = write_init_struct(job, strct, i)) !=
        	 CJOB_SUCCESS) return result;	
        break;	
    }
  }
  return CJOB_SUCCESS;
}

/* Write a list initializer function to the output file. */
static CJobStatus write_init_list(CJob *job, ParsedStruct *strct, 
                                  int field)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job, 
"HarisStatus %s%s_init_%s(%s%s *strct, haris_uint32_t sz)\n\
{\n\
  return haris_lib_init_list_mem((void*)strct, &haris_lib_structures[%d], \
%d, sz);\n}\n\n", prefix, name, strct->children[field].name, prefix, name, 
                  strct->schema_index, field);
  return CJOB_SUCCESS;
}

/* Write a structure initializer function to the output file. */
static CJobStatus write_init_struct(CJob *job, ParsedStruct *strct,
                                    int field)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job, 
"HarisStatus %s%s_init_%s(%s%s *strct)\n\
{\n\
  return haris_lib_init_struct_mem((void*)strct, &haris_lib_structures[%d], \
%d);\n}\n\n", prefix, name, strct->children[field].name, prefix, name, 
              strct->schema_index, field);
  return CJOB_SUCCESS;
}

/* Write the GENERAL list initializer static function to the output file.
   This function consumes a void pointer to an in-memory C structure,
   a HarisStructureInfo describing its makeup, a field number (this should
   be the 0-indexed number of the list field in question), and a size
   parameter (which shall be the length of the list to allocate).
*/
static CJobStatus write_general_init_list_member(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static HarisStatus haris_lib_init_list_mem(void *ptr,\
const HarisStructureInfo *info, int field, haris_uint32_t sz)\n\
{\n\
  void *testptr;\n\
  const HarisChild *child = &info->children[field];\n\
  HarisListInfo *list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
  haris_uint32_t i;\n\
  size_t element_size;\n\
  if (sz == 0 || \n\
      list_info->alloc >= sz)\n\
    goto Success;\n\
  switch (child->child_type) {\n\
  case HARIS_CHILD_TEXT:\n\
    element_size = 1;\n\
    break;\n\
  case HARIS_CHILD_SCALAR_LIST:\n\
    element_size = haris_lib_in_memory_scalar_sizes[child->scalar_element];\n\
    break;\n\
  case HARIS_CHILD_STRUCT_LIST:\n\
    element_size = child->struct_element->size_of;\n\
    break;\n\
  case HARIS_CHILD_STRUCT:\n\
    return HARIS_STRUCTURE_ERROR;\n\
  }\n\
  testptr = realloc(list_info->ptr, sz * element_size);\n\
  if (!testptr) return HARIS_MEM_ERROR;\n\
  list_info->ptr = testptr;\n\
  if (child->child_type == HARIS_CHILD_STRUCT_LIST) {\n\
    for (i = list_info->alloc; i < sz; i ++) {\n\
      haris_lib_create_contents((char*)testptr + i * element_size,\n\
                                child->struct_element);\n\
      list_info->alloc ++;\n\
    }\n\
  } else list_info->alloc = sz;\n\
  Success:\n\
    list_info->len = sz;\n\
    return HARIS_SUCCESS;\n\
}\n\n");
  return CJOB_SUCCESS;
}

/* Write the GENERAL structure initializer static function to the output
   file. This function accepts arguments that are equivalent to the above,
   except without a size parameter (as this child field is not a list).
*/
static CJobStatus write_general_init_struct_member(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static HarisStatus haris_lib_init_struct_mem(void *ptr,\n\
                                              const HarisStructureInfo *info,\n\
                                              int field)\n\
{\n\
  void **vdptrptr;\n\
  vdptrptr = (void**)((char*)ptr + info->children[field].offset);\n\
  if (*vdptrptr) return HARIS_SUCCESS;\n\
  if ((*vdptrptr = \n\
      haris_lib_create(info->children[field].struct_element))\n\
      == NULL) return HARIS_MEM_ERROR;\n\
  return HARIS_SUCCESS;\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* WRITING ********* */

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

/* Writes the core writing-functions to the output file. These functions
   consume a HarisStructureInfo pointer, which is taken to be the type
   of the Haris message, as well as an unsigned char pointer that is
   the relevant portion of the in-memory Haris message. An extra void*
   pointer is passed to functions that need it; this is a pointer
   to the in-memory C structure matching the HarisStructureInfo parameter.
   In each case, a portion of the message will be written to the buffer.
*/
static CJobStatus write_core_wfuncs(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static\n\
unsigned char *haris_lib_write_nonnull_header(const HarisStructureInfo *info,\n\
                                                      unsigned char *buf)\n\
{\n\
  buf[0] = (unsigned char)0x40 | (unsigned char)info->num_children;\n\
  buf[1] = (unsigned char)info->body_size;\n\
  return buf + 2;\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static unsigned char *haris_lib_write_header(const void *ptr,\n\
                                              const HarisStructureInfo *info,\n\
                                              unsigned char *buf)\n\
{\n\
  if (*(char*)ptr) { buf[0] = 0; return buf + 1; }\n\
  else return haris_lib_write_nonnull_header(info, buf);\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static unsigned char *haris_lib_write_body(const void *ptr,\n\
                                            const HarisStructureInfo *info,\n\
                                            unsigned char *buf)\n\
{\n\
  int i;\n\
  HarisScalarType type;\n\
  if (*(char*)ptr) return buf;\n\
  for (i = 0; i < info->num_scalars; i ++) {\n\
    type = info->scalars[i].type;\n\
    haris_lib_write_scalar(buf, (char*)ptr + info->scalars[i].offset, type);\n\
    buf += haris_lib_message_scalar_sizes[type];\n\
  }\n  return buf;\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* READING ********* */

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

/* Writes the principal core message-reading function. This function's purpose
   is to read the body of a Haris message structure (pointerd to by buf) into
   the in-memory C structure given by the void *ptr. The info parameter, as 
   always, tells us the type of the structure we are reading.
*/
static CJobStatus write_core_rfuncs(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static unsigned char *haris_lib_read_body(void *ptr,\n\
                                           const HarisStructureInfo *info,\n\
                                           unsigned char *buf)\n\
{\n\
  int i;\n\
  HarisScalarType type;\n\
  for (i = 0; i < info->num_scalars; i ++) {\n\
    type = info->scalars[i].type;\n\
    haris_lib_read_scalar(buf, (char*)ptr + info->scalars[i].offset, type);\n\
    buf = buf + haris_lib_message_scalar_sizes[type];\n\
  }\n  return buf;\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* SIZE ********* */

/* Writes the core size-measuring function to the output file. This function's
   purpose is twofold: first, to find the size of the buffer that would be
   required to completely hold the Haris message given by the in-memory C
   structure ptr. Second, it detects any structural errors in the C structure.
   If the function successfully returns a non-zero size, then the structure can
   be safely transcribed into the Haris format.
*/
static CJobStatus write_core_size(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"haris_uint32_t haris_lib_size(void *ptr, const HarisStructureInfo *info,\n\
                               int depth, HarisStatus *out)\n\
{\n\
  int i;\n\
  haris_uint32_t accum, buf, j;\n\
  const HarisChild *child;\n\
  HarisListInfo *list_info;\n\
  if (*(char*)ptr) return 1;\n\
  else if (depth > HARIS_DEPTH_LIMIT) {\n\
    *out = HARIS_DEPTH_ERROR;\n\
    return 0;\n\
  }\n\
  accum = info->body_size + 2;\n\
  for (i = 0; i < info->num_children; i ++) {\n\
    child = &info->children[i];\n\
    list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
    if (!child->nullable) {\n\
      int child_is_null = child->child_type == HARIS_CHILD_STRUCT ?\n\
        (int)!*(void**)list_info : list_info->null;\n\
      if (child_is_null) goto StructureError;\n\
    }\n\
    switch (child->child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_CHILD_SCALAR_LIST:\n\
      if (list_info->null)\n\
        accum += 1;\n\
      else\n\
        accum += 4 + \n\
          list_info->len * (child->child_type == HARIS_CHILD_TEXT ?\n\
                            1 : haris_lib_message_scalar_sizes[child->scalar_element]);\n\
      break;\n\
    case HARIS_CHILD_STRUCT_LIST:\n\
      if (list_info->null)\n\
        accum += 1;\n\
      else {\n\
        accum += 6;\n\
        for (j = 0; j < list_info->len; j ++) {\n\
          buf = haris_lib_size((void*)((char*)list_info->ptr + \n\
                                j * child->struct_element->size_of),\n\
                                child->struct_element,\n\
                                depth + 1, out);\n\
          if (buf == 0) return 0;\n\
          else if (buf == 1) goto StructureError;\n\
          else if ((accum += buf - 2) > HARIS_MESSAGE_SIZE_LIMIT) {\n\
            *out = HARIS_SIZE_ERROR; return 0;\n\
          }\n\
        }\n\
      }\n\
      break;\n\
    case HARIS_CHILD_STRUCT:\n\
      buf = haris_lib_size(*(void**)list_info, child->struct_element,\n\
                           depth + 1, out);\n\
      if (buf == 0) return 0;\n\
      else if ((accum += buf) > HARIS_MESSAGE_SIZE_LIMIT) {\n\
        *out = HARIS_SIZE_ERROR; return 0;\n\
      }\n\
    }\n\
  }\n\
  return accum;\n\
  StructureError:\n\
  *out = HARIS_STRUCTURE_ERROR;\n\
  return 0;\n\
}\n\n");
  return CJOB_SUCCESS;
}
