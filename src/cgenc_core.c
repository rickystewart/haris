#include "cgenc_core.h"
#include "cgenc_util.h"

/* Functions relating to the public constructors, destructors, and
   initializers */

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

static CJobStatus write_core_wfuncs(CJob *);
static CJobStatus write_core_rfuncs(CJob *);
static CJobStatus write_core_size(CJob *);

static CJobStatus write_general_child_handler(CJob *);
static CJobStatus write_from_stream_funcs(CJob *);
static CJobStatus write_to_stream_funcs(CJob *);

static CJobStatus (* const general_core_writer_functions[])(CJob *) = {
  write_in_memory_scalar_sizes, write_message_scalar_sizes, 
  write_message_bit_patterns,

  write_general_constructor, write_general_destructor, 

  write_general_init_list_member, write_general_init_struct_member,

  write_core_wfuncs, write_core_rfuncs, write_core_size,

  write_general_child_handler, write_from_stream_funcs,
  write_to_stream_funcs
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
  if ((result = write_reflective_arrays(job)) != CJOB_SUCCESS ||
      (result = write_utility_library(job)) != CJOB_SUCCESS)
    return result;
  for (i = 0; 
       i < sizeof general_core_writer_functions / 
           sizeof general_core_writer_functions[0]; 
       i ++) {
    if ((result = general_core_writer_functions[i](job)) != CJOB_SUCCESS)
      return result;
  }
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

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
  const char *prefix = job->prefix, *strct_name = strct->name;
  if (strct->num_scalars == 0) {
    CJOB_FMT_SOURCE_STRING(job, "#define %s%s_lib_scalars NULL\n\n", 
                           prefix, strct_name);
    return CJOB_SUCCESS;
  }
  CJOB_FMT_SOURCE_STRING(job, 
"static const HarisScalar %s%s_lib_scalars[] = {\n", prefix, strct_name);
  for (i = 0; i < strct->num_scalars; i ++)
    CJOB_FMT_SOURCE_STRING(job, "  { offsetof(%s%s, %s), %s }%s\n",
                           prefix, strct_name, strct->scalars[i].name, 
                           scalar_enumerated_name(strct->scalars[i].type.tag),
                           (i + 1 >= strct->num_scalars ? "" : ","));
  CJOB_FMT_SOURCE_STRING(job, "};\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_reflective_child_array(CJob *job, ParsedStruct *strct)
{
  int i;
  ChildField *child;
  const char *prefix = job->prefix, *strct_name = strct->name, *child_name;
  if (strct->num_children == 0) {
    CJOB_FMT_SOURCE_STRING(job, "#define %s%s_lib_children NULL\n\n",
                           prefix, strct_name);
    return CJOB_SUCCESS;
  }
  CJOB_FMT_SOURCE_STRING(job, 
"static const HarisChild %s%s_lib_children[] = {\n", 
                         prefix, strct_name);
  for (i = 0; i < strct->num_children; i ++) {
    child = &strct->children[i];
    child_name = child->name;
    CJOB_FMT_SOURCE_STRING(job, 
"  { offsetof(%s%s, _%s_info), %d, %s, ",
                           prefix, strct_name, child_name,
                           child->nullable, 
                           child->tag == CHILD_SCALAR_LIST ? 
                           scalar_enumerated_name(child->type.scalar_list.tag) :
                             child->tag == CHILD_TEXT ?
                             "HARIS_SCALAR_UINT8" :
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
  const char *prefix = job->prefix, *strct_name;
  CJOB_FMT_SOURCE_STRING(job, 
"extern const HarisStructureInfo haris_lib_structures[%d];\n\n",
                         job->schema->num_structs);
  for (i = 0; i < job->schema->num_structs; i ++) {
    strct = &job->schema->structs[i];
    if ((result = write_reflective_scalar_array(job, strct)) != CJOB_SUCCESS)
      return result;
    if ((result = write_reflective_child_array(job, strct)) != CJOB_SUCCESS)
      return result;
  }
  CJOB_FMT_SOURCE_STRING(job, 
"const HarisStructureInfo haris_lib_structures[] = {\n");
  for (i = 0; i < job->schema->num_structs; i ++) {
    strct = &job->schema->structs[i];
    strct_name = strct->name;
    CJOB_FMT_SOURCE_STRING(job, 
"  { %d, %s%s_lib_scalars, %d, %s%s_lib_children, %d, sizeof(%s%s) }%s\n",
                           strct->num_scalars, prefix, strct_name, 
                           strct->num_children, prefix, 
                           strct_name, strct->offset, prefix, 
                           strct_name, 
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
  CJOB_FMT_SOURCE_STRING(job,
"static const size_t haris_lib_message_size_from_bit_pattern[] = {\n\
  1, 2, 4, 8\n\
};\n\n");
  return CJOB_SUCCESS;
}

/* ********* CONSTRUCTOR ********* */

/* Write the public constructor for the given structure to the given file. */
static CJobStatus write_public_constructor(CJob *job, ParsedStruct *strct)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job, "%s%s *%s%s_create(void) {\n\
  return (%s%s*)_haris_lib_create(&haris_lib_structures[%d]);\n}\n\n",
               prefix, name, prefix, name, prefix, name, strct->schema_index);
  return CJOB_SUCCESS;
}

/* Write the GENERAL constructor to the given file. This is a function
   that consumes a HarisStructureInfo pointer and returns a new structure
   of the given type as a void*.

   While none of the scalar fields of the structure will be initialized,
   all pointer elements (structures and lists) will be initialized to
   NULL.
*/
static CJobStatus write_general_constructor(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void *haris_lib_create_contents(void *strct,\n\
                                        const HarisStructureInfo *info)\n\
{\n\
  memset(strct, 0, info->size_of);\n\
  return strct;\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void *_haris_lib_create(const HarisStructureInfo *info)\n\
{\n\
  void *strct = HARIS_MALLOC(info->size_of);\n\
  if (!strct) return NULL;\n\
  return memset(strct, 0, info->size_of);\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* DESTRUCTOR ********* */

/* Writes the destructor for the given structure to the output file. */
static CJobStatus write_public_destructor(CJob *job, ParsedStruct *strct)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job, "void %s%s_destroy(%s%s *strct)\n\
{\n\
  _haris_lib_destroy((void*)strct, &haris_lib_structures[%d]);\n\
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
"static void haris_lib_destroy_contents(void *ptr,\n\
                                        const HarisStructureInfo *info)\n\
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
        HARIS_FREE(list_info->ptr);\n\
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
      HARIS_FREE(list_info->ptr);\n\
      break;\n\
    }\n\
  }\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static void _haris_lib_destroy(void *ptr, const HarisStructureInfo *info)\n\
{\n\
  haris_lib_destroy_contents(ptr, info);\n\
  HARIS_FREE(ptr);\n}\n\n");
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
  return _haris_lib_init_list_mem((void*)strct, &haris_lib_structures[%d], \
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
  return _haris_lib_init_struct_mem((void*)strct, &haris_lib_structures[%d], \
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
"static HarisStatus _haris_lib_init_list_mem(void *ptr,\
const HarisStructureInfo *info, int field, haris_uint32_t sz)\n\
{\n\
  void *testptr;\n\
  const HarisChild *child = &info->children[field];\n\
  HarisListInfo *list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
  size_t element_size;\n\
  if (sz == 0 || \n\
      (list_info->alloc >= sz &&\n\
       (double)sz / (double)list_info->alloc >= HARIS_DEALLOC_FACTOR))\n\
    goto Success;\n\
  switch (child->child_type) {\n\
  case HARIS_CHILD_TEXT:\n\
  case HARIS_CHILD_SCALAR_LIST:\n\
    element_size = haris_lib_in_memory_scalar_sizes[child->scalar_element];\n\
    break;\n\
  case HARIS_CHILD_STRUCT_LIST:\n\
    element_size = child->struct_element->size_of;\n\
    break;\n\
  case HARIS_CHILD_STRUCT:\n\
    return HARIS_STRUCTURE_ERROR;\n\
  }\n\
  testptr = HARIS_REALLOC(list_info->ptr, sz * element_size);\n\
  if (!testptr) return HARIS_MEM_ERROR;\n\
  list_info->ptr = testptr;\n\
  if (child->child_type == HARIS_CHILD_STRUCT_LIST) {\n\
    memset(testptr, 0, sz * element_size);\n\
  }\n\
  list_info->alloc = sz;\n\
  Success:\n\
    list_info->has = 1;\n\
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
"static\n\
HarisStatus _haris_lib_init_struct_mem(void *ptr,\n\
                                      const HarisStructureInfo *info,\n\
                                      int field)\n\
{\n\
  HarisSubstructInfo *substruct;\n\
  substruct = (HarisSubstructInfo*)((char*)ptr + \n\
                                    info->children[field].offset);\n\
  if (substruct->ptr) goto Success;\n\
  if ((substruct->ptr = \n\
       _haris_lib_create(info->children[field].struct_element)) == NULL)\n\
    return HARIS_MEM_ERROR;\n\
  Success:\n\
  substruct->has = 1;\n\
  return HARIS_SUCCESS;\n}\n\n");
  return CJOB_SUCCESS;
}

/* ********* WRITING AND READING MESSAGE BODIES ********* */

/* Writes the core writing-functions to the output file. These functions
   consume a HarisStructureInfo pointer, which is taken to be the type
   of the Haris message, as well as an unsigned char pointer that is
   the relevant portion of the in-memory Haris message. An extra void*
   pointer is passed to functions that need it; this is a pointer
   to the in-memory C structure matching the HarisStructureInfo parameter.
   In each case, a portion of the message will be written to the buffer,
   whether that be the header or the entire body.
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
"static unsigned char *haris_lib_write_body(const void *ptr,\n\
                                            const HarisStructureInfo *info,\n\
                                            unsigned char *buf)\n\
{\n\
  int i;\n\
  HarisScalarType type;\n\
  for (i = 0; i < info->num_scalars; i ++) {\n\
    type = info->scalars[i].type;\n\
    haris_lib_write_scalar(buf, (char*)ptr + info->scalars[i].offset, type);\n\
    buf += haris_lib_message_scalar_sizes[type];\n\
  }\n  return buf;\n}\n\n");
  return CJOB_SUCCESS;
}

/* Writes the principal core message-reading function. This function's purpose
   is to read the body of a Haris message structure (pointed to by buf) into
   the in-memory C structure given by the void *ptr. The info parameter, as 
   always, tells us the type of the structure we are reading.
*/
static CJobStatus write_core_rfuncs(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static const unsigned char *haris_lib_read_body(void *ptr,\n\
                                                const HarisStructureInfo *info,\n\
                                                const unsigned char *buf)\n\
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
  HarisSubstructInfo *substruct_info;\n\
  if (depth > HARIS_DEPTH_LIMIT) {\n\
    *out = HARIS_DEPTH_ERROR;\n\
    return 0;\n\
  }\n\
  accum = info->body_size + 2;\n\
  for (i = 0; i < info->num_children; i ++) {\n\
    child = &info->children[i];\n\
    list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
    if (!child->nullable) {\n\
      if ((child->child_type == HARIS_CHILD_STRUCT) ?\n\
          !((HarisSubstructInfo*)list_info)->has : !list_info->has)\n\
        goto StructureError;\n\
    }\n\
    switch (child->child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_CHILD_SCALAR_LIST:\n\
      if (!list_info->has)\n\
        accum += 1;\n\
      else\n\
        accum += 4 + \n\
          list_info->len * haris_lib_message_scalar_sizes[child->scalar_element];\n\
      break;\n\
    case HARIS_CHILD_STRUCT_LIST:\n\
      if (!list_info->has)\n\
        accum += 1;\n\
      else {\n\
        accum += 6;\n\
        for (j = 0; j < list_info->len; j ++) {\n\
          buf = haris_lib_size((void*)((char*)list_info->ptr + \n\
                                j * child->struct_element->size_of),\n\
                                child->struct_element,\n\
                                depth + 1, out);\n\
          if (buf == 0) return 0;\n\
          else if ((accum += buf - 2) > HARIS_MESSAGE_SIZE_LIMIT) {\n\
            *out = HARIS_SIZE_ERROR; return 0;\n\
          }\n\
        }\n\
      }\n\
      break;\n\
    case HARIS_CHILD_STRUCT:\n\
      substruct_info = (HarisSubstructInfo*)list_info;\n\
      buf = (!substruct_info->has ? \n\
             1 :\n\
             haris_lib_size(substruct_info->ptr, child->struct_element,\n\
                            depth + 1, out));\n\
      if (buf == 0) return 0;\n\
      else if ((accum += buf) > HARIS_MESSAGE_SIZE_LIMIT) {\n\
        *out = HARIS_SIZE_ERROR;\n\
        return 0;\n\
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

/* ********* PROTOCOL FUNCTIONS ********* */

/* These functions are the principal core protocol functions. The protocol
   core has a special purpose: while the haris_write_ and haris_read_ functions
   above provide the functionality to write and read message bodies, these 
   functions give us the power to construct entire messages.

   The protocol core is extremely generalized. The _haris_from_stream and
   _haris_to_stream functions respectively read and write an in-memory C 
   structure from an opaque stream object which is passed a void pointer. 
   Bytes are read and written to the stream with the companion interface
   function, which is called reader or writer (depending on what it does).

   In order to use these core functions, therefore, you must pass a stream
   argument and a reader/writer argument that meets the interface. (For 
   example, in order to serialize to an in-memory buffer, the buffer library
   defines a HarisBufferStream structure, in addition to a set of functions
   that allow an in-memory buffer to be reasoned about as if it were a stream.)
   As long as you expose this streaming functionality, and the functions
   you pass in match the HarisStreamReader or HarisStreamWriter prototypes,
   your new protocol should snap into the generated codebase without an issue,
   and serialize and deserialize correctly as a matter of course.

   If you would like to go about defining your own protocols, the contract
   is as follows:
   - First, define a structure that will capture all of the data needed to
     represent the stream. If you do not believe you need to implement
     a new structure to capture all of this information, then you can
     pass in as the `stream` argument a pointer to any other pre-defined
     object. You can even use NULL as your stream if you can find a way to
     go without a stream object.
   - Next, define the reading and writing functions that the library will
     use to read bytes from and write bytes to the stream. These functions
     are expected to work as follows:
     HarisStreamReader: Read n bytes from the stream, and copy into the 
     out parameter a pointer to a buffer (n bytes long) that contains the
     bytes that were read from the stream. The caller does not need to free
     the pointer (that is, it is managed by the stream interface). The pointer
     is assumed to be valid until the next call to the reader function, at 
     which point the pointer immediately becomes invalid. For this reason,
     the same buffer can be used for all calls. 1000 is the maximum number of
     bytes that a stream reader needs to support.
     HarisStreamWriter: Write n bytes from the given buffer onto the stream.
     Again, only writes of size 1000 need be supported.

     Both of these functions should return HARIS_SUCCESS if everything went
     well, and another error code otherwise. A success code should indicate
     the read or write was entirely successful (that is, all n bytes were
     written or read).  

     Furthermore, these functions are expected to do error checking. The
     core library does not assure the messages do not get too large; the
     stream functions are expected to keep track of how many bytes have 
     been written or read and raise an error if any illegal operation
     is attempted. 
   - Finally, pass in a pointer to your stream object and a pointer to one
     of your stream-management functions, depending on which operation 
     you'd like to perform. You're all done.
*/

static CJobStatus write_general_child_handler(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus handle_child(void *stream, HarisStreamReader reader,\n\
                                int depth)\n\
{\n\
  HarisStatus result;\n\
  unsigned char first_byte_of_header;\n\
  const unsigned char *read_buffer;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  if ((result = reader(stream, 1, &read_buffer)) != HARIS_SUCCESS) \n\
    return result;\n\
  first_byte_of_header = *read_buffer;\n\
  if (!first_byte_of_header) return HARIS_SUCCESS; /* test for null */\n\
  if ((first_byte_of_header & 0xC0) == 0x40) { /* structure child */\n\
    int num_children, body_size;\n\
    if ((result = reader(stream, 1, &read_buffer)) != HARIS_SUCCESS) \n\
      return result;\n\
    num_children = first_byte_of_header & 0x3F;\n\
    body_size = *read_buffer;\n\
    return handle_child_struct_posthead(stream, reader, depth, \n\
                                        num_children, body_size);\n\
  } else if ((first_byte_of_header & 0xC0) == 0x80) { /* scalar list */\n\
    haris_uint32_t len, msg_size, array_size;\n\
    if ((result = reader(stream, 3, &read_buffer)) != HARIS_SUCCESS)\n\
      return result;\n\
    msg_size = haris_lib_message_size_from_bit_pattern[first_byte_of_header\n\
                                                       & 0x3];\n\
    haris_read_uint24(read_buffer, &len);\n\
    array_size = msg_size * len;\n\
    while (array_size > 0) { \n\
      haris_uint32_t read_size = (array_size <= 256 ? array_size : 256);\n\
      if ((result = reader(stream, read_size, &read_buffer)) != HARIS_SUCCESS)\n\
        return result;\n\
      array_size -= read_size;\n\
    }\n\
    return HARIS_SUCCESS;\n\
  } else { /* structure list */\n\
    haris_uint32_t x, len;\n\
    int num_children, body_size;\n\
    if ((result = reader(stream, 5, &read_buffer)) != HARIS_SUCCESS)\n\
      return result;\n\
    haris_read_uint24(read_buffer, &len);\n\
    num_children = read_buffer[3] & 0x3F;\n\
    body_size = read_buffer[4];\n\
    for (x = 0; x < len; x++)\n\
      if ((result = handle_child_struct_posthead(stream, reader, depth,\n\
                                                 num_children, \n\
                                                 body_size)) != HARIS_SUCCESS)\n\
        return result;\n\
    return HARIS_SUCCESS;\n\
  }\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static HarisStatus handle_child_struct_posthead(void *stream,\n\
                                                HarisStreamReader reader,\n\
                                                int depth,\n\
                                                int num_children,\n\
                                                int body_size)\n\
{\n\
  HarisStatus result;\n\
  int i;\n\
  const unsigned char *read_buffer;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  if ((result = reader(stream, (haris_uint32_t)body_size, &read_buffer)) \n\
      != HARIS_SUCCESS)\n\
    return result;\n\
  for (i = 0; i < num_children; i++)\n\
    if ((result = handle_child(stream, reader, depth + 1)) != HARIS_SUCCESS)\n\
      return result;\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_from_stream_funcs(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static HarisStatus _haris_from_stream(void *ptr,\n\
                                     const HarisStructureInfo *info,\n\
                                     void *stream,\n\
                                     HarisStreamReader reader,\n\
                                     int depth)\n\
{\n\
  HarisStatus result;\n\
  int num_children, body_size;\n\
  const unsigned char *read_buffer;\n\
  unsigned char first_byte_of_header;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  if ((result = reader(stream, 1, &read_buffer)) != HARIS_SUCCESS) \n\
    return result;\n\
  first_byte_of_header = *read_buffer;\n\
  HARIS_ASSERT(first_byte_of_header && !(first_byte_of_header & 0x80), \n\
               STRUCTURE); /* check this isn't null and this isn't a list */\n\
  if ((result = reader(stream, 1, &read_buffer)) != HARIS_SUCCESS) \n\
    return result;\n\
  num_children = first_byte_of_header & 0x3F;\n\
  body_size = *read_buffer;\n\
  HARIS_ASSERT(body_size >= info->body_size &&\n\
               num_children >= info->num_children, STRUCTURE);\n\
  return _haris_from_stream_posthead(ptr, info, stream, reader, depth, \n\
                                    num_children, body_size);\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, "%s%s",
"static HarisStatus _haris_from_stream_posthead(void *ptr,\n\
                                              const HarisStructureInfo *info,\n\
                                              void *stream, \n\
                                              HarisStreamReader reader,\n\
                                              int depth, int num_children,\n\
                                              int body_size)\n\
{\n\
  HarisStatus result;\n\
  int i;\n\
  const HarisChild *child;\n\
  HarisListInfo *list_info;\n\
  const unsigned char *body, *read_buffer;\n\
  unsigned char first_byte_of_child_header;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  if ((result = reader(stream, (haris_uint32_t)body_size, &body)) \n\
      != HARIS_SUCCESS)\n\
    return result;\n\
  haris_lib_read_body(ptr, info, body);\n\
  for (i = 0; i < info->num_children; i ++) {\n\
    child = &info->children[i];\n\
    list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
    if ((result = reader(stream, 1, &read_buffer)) != HARIS_SUCCESS) \n\
      return result;\n\
    first_byte_of_child_header = *read_buffer;\n\
    if (!first_byte_of_child_header) { /* check whether child is null */\n\
      HARIS_ASSERT(child->nullable, STRUCTURE);\n\
      if (child->child_type != HARIS_CHILD_STRUCT) \n\
        list_info->has = 0;\n\
      else \n\
        ((HarisSubstructInfo*)list_info)->has = 0;\n\
      continue;\n\
    }\n\
    switch (child->child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_CHILD_SCALAR_LIST:\n\
    {\n\
      haris_uint32_t len, msg_size, mem_size, bit_pattern, j;\n\
      char *in_mem_element_pointer;\n\
      if ((result = reader(stream, 3, &read_buffer)) != HARIS_SUCCESS)\n\
        return result;\n\
      msg_size = haris_lib_message_scalar_sizes[child->scalar_element];\n\
      mem_size = haris_lib_in_memory_scalar_sizes[child->scalar_element];\n\
      bit_pattern = haris_lib_scalar_bit_patterns[child->scalar_element];\n\
      HARIS_ASSERT(first_byte_of_child_header == (0x80 | bit_pattern),\n\
                   STRUCTURE);\n\
      haris_read_uint24(read_buffer, &len);\n\
      if ((result = _haris_lib_init_list_mem(ptr, info, i, len))\n\
           != HARIS_SUCCESS)\n\
        return result;\n\
      for (j = 0,  in_mem_element_pointer = (char*)list_info->ptr; \n\
           j < len; \n\
           j ++,   in_mem_element_pointer += mem_size) {\n\
        /* This reads one element from the message at a time into the buffer;\n\
           in the future, batching reads will be an easy optimization */\n\
        if ((result = reader(stream, msg_size, &read_buffer)) != HARIS_SUCCESS)\n\
          return result;\n\
        haris_lib_read_scalar(read_buffer, (void*)in_mem_element_pointer,\n\
                              child->scalar_element);\n\
      }\n\
      break;\n\
    }\n",
"    case HARIS_CHILD_STRUCT_LIST:\n\
    {\n\
      haris_uint32_t len, j;\n\
      char *in_mem_element_pointer;\n\
      int num_children, body_size;\n\
      if ((result = reader(stream, 5, &read_buffer)) != HARIS_SUCCESS)\n\
        return result;\n\
      HARIS_ASSERT(first_byte_of_child_header == 0xC0, STRUCTURE);\n\
      haris_read_uint24(read_buffer, &len);\n\
      if ((result = _haris_lib_init_list_mem(ptr, info, i, len))\n\
           != HARIS_SUCCESS)\n\
        return result;\n\
      HARIS_ASSERT((read_buffer[3] & 0xC0) == 0x40, STRUCTURE);\n\
      num_children = read_buffer[3] & 0x3F;\n\
      body_size = read_buffer[4];\n\
      for (j = 0,  in_mem_element_pointer = (char*)list_info->ptr; \n\
           j < len; \n\
           j ++,   in_mem_element_pointer += child->struct_element->size_of) {\n\
        if ((result = _haris_from_stream_posthead(in_mem_element_pointer, \n\
                                                 child->struct_element, \n\
                                                 stream, reader, depth + 1, \n\
                                                 num_children, \n\
                                                 body_size)) != HARIS_SUCCESS)\n\
          return result;\n\
      }\n\
      break;\n\
    }\n\
    case HARIS_CHILD_STRUCT:\n\
    {\n\
      if ((result = _haris_lib_init_struct_mem(ptr, info, i))\n\
           != HARIS_SUCCESS)\n\
        return result;\n\
      if ((result = _haris_from_stream(*(void**)list_info,\n\
                                      child->struct_element, \n\
                                      stream, reader, depth + 1)) \n\
          != HARIS_SUCCESS)\n\
        return result;\n\
      break;\n\
    }\n\
    }\n\
  }\n\
  for (; i < num_children; i ++) {\n\
    if ((result = handle_child(stream, reader, depth + 1)) != HARIS_SUCCESS)\n\
      return result;\n\
  }\n\
  return HARIS_SUCCESS;\n}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_to_stream_funcs(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus _haris_to_stream(void *ptr,\n\
                                   const HarisStructureInfo *info, \n\
                                   void *stream,\n\
                                   HarisStreamWriter writer)\n\
{\n\
  HarisStatus result;\n\
  unsigned char header[2];\n\
  haris_lib_write_nonnull_header(info, header);\n\
  if ((result = writer(stream, header, 2)) != HARIS_SUCCESS) return result;\n\
  return _haris_to_stream_posthead(ptr, info, stream, writer);\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus _haris_to_stream_posthead(void *ptr, \n\
                                            const HarisStructureInfo *info, \n\
                                            void *stream,\n\
                                            HarisStreamWriter writer)\n\
{\n\
  int i;\n\
  const HarisChild *child;\n\
  HarisListInfo *list_info;\n\
  HarisStatus result;\n\
  unsigned char body[256], child_header[6];\n\
  if ((result = writer(stream, body,\n\
                       haris_lib_write_body(ptr, info, body) - body))\n\
      != HARIS_SUCCESS)\n\
    return result;\n\
  for (i = 0; i < info->num_children; i ++) {\n\
    child = &info->children[i];\n\
    list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
    switch (child->child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_CHILD_SCALAR_LIST:\n\
    case HARIS_CHILD_STRUCT_LIST:\n\
      if (!list_info->has) {\n\
        child_header[0] = 0x0;\n\
        if ((result = writer(stream, child_header, 1)) != HARIS_SUCCESS)\n\
          return result;\n\
        continue;\n\
      }\n\
      break;\n\
    case HARIS_CHILD_STRUCT:\n\
      if (!((HarisSubstructInfo*)list_info)->has) {\n\
        child_header[0] = 0x0;\n\
        if ((result = writer(stream, child_header, 1)) != HARIS_SUCCESS)\n\
          return result;\n\
        continue;\n\
      }\n\
      break;\n\
    }\n\
    switch (child->child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_CHILD_SCALAR_LIST:\n\
    {\n\
      unsigned char buffer[8];\n\
      haris_uint32_t msg_size, mem_size, j;\n\
      char *in_mem_element_pointer;\n\
      msg_size = haris_lib_message_scalar_sizes[child->scalar_element];\n\
      mem_size = haris_lib_in_memory_scalar_sizes[child->scalar_element];\n\
      child_header[0] = (0x80 | \n\
                         haris_lib_scalar_bit_patterns[child->scalar_element]);\n\
      haris_write_uint24(child_header + 1, &list_info->len);\n\
      if ((result = writer(stream, child_header, 4)) != HARIS_SUCCESS)\n\
        return result;\n\
      for (j = 0,             in_mem_element_pointer = (char*)list_info->ptr;\n\
           j < list_info->len; \n\
           j ++,              in_mem_element_pointer += mem_size) {\n\
        /* As above, we can easily optimize this to batch writes to the \n\
           stream */\n\
        haris_lib_write_scalar(buffer, in_mem_element_pointer,\n\
                               child->scalar_element);\n\
        if ((result = writer(stream, buffer, msg_size)) != HARIS_SUCCESS)\n\
          return result;\n\
      }\n\
      break;\n\
    }\n\
    case HARIS_CHILD_STRUCT_LIST:\n\
    {\n\
      char *in_mem_element_pointer;\n\
      haris_uint32_t j;\n\
      child_header[0] = 0xC0;\n\
      haris_write_uint24(child_header + 1, &list_info->len);\n\
      (void)haris_lib_write_nonnull_header(child->struct_element, \n\
                                           child_header + 4);\n\
      if ((result = writer(stream, child_header, 6)) != HARIS_SUCCESS)\n\
        return result;\n\
      for (j = 0, in_mem_element_pointer = (char*)list_info->ptr; \n\
           j < list_info->len; \n\
           j ++, in_mem_element_pointer += child->struct_element->size_of) {\n\
        if ((result = _haris_to_stream_posthead(in_mem_element_pointer,\n\
                                               child->struct_element, \n\
                                               stream, writer)) \n\
            != HARIS_SUCCESS)\n\
          return result;\n\
      }\n\
      break;\n\
    }\n\
    case HARIS_CHILD_STRUCT:\n\
      if ((result = _haris_to_stream(((HarisSubstructInfo*)list_info)->ptr,\n\
                                    child->struct_element, \n\
                                    stream, writer)) != HARIS_SUCCESS)\n\
        return result;\n\
      break;\n\
    }\n\
  }\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  return CJOB_SUCCESS;
}

