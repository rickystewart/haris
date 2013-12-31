#include "cgenc_core.h"

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

static CJobStatus write_in_memory_scalar_sizes(CJob *);
static CJobStatus write_message_scalar_sizes(CJob *);
static CJobStatus write_message_bit_patterns(CJob *job);
static CJobStatus write_scalar_readers(CJob *);
static CJobStatus write_scalar_writers(CJob *);

static CJobStatus write_scalar_writer_function(CJob *job);
static CJobStatus write_scalar_reader_function(CJob *job);

static CJobStatus write_core_wfuncs(CJob *);
static CJobStatus write_core_rfuncs(CJob *);
static CJobStatus write_core_size(CJob *);

static CJobStatus (* const general_core_writer_functions[])(CJob *) = {
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
  for (i=0; i < job->schema->num_structs; i++) {
    if ((result = write_public_constructor(job, &job->schema->structs[i]))
        != CJOB_SUCCESS)
      return result;
    if ((result = write_public_destructor(job, &job->schema->structs[i]))
        != CJOB_SUCCESS)
      return result;
    if ((result = write_public_initializers(job, &job->schema->structs[i]))
        != CJOB_SUCCESS)
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
  for (i = 0; i < sizeof general_core_writer_functions / 
                  sizeof general_core_writer_functions[0]; i++)
    if ((result = general_core_writer_functions[i](job)) != CJOB_SUCCESS)
      return result;
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

/* Write the public constructor for the given structure to the given file. */
static CJobStatus write_public_constructor(CJob *job, ParsedStruct *strct)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job, "%s%s *%s%s_create() {\n\
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
  CJOB_FMT_PRIV_FUNCTION(job, "static void *haris_lib_create(const HarisStructureInfo *info)\n\
{\n\
  int i;\n\
  HarisListInfo *list_info;\n\
  void *new = malloc(info->size_of);\n\
  if (!new) return NULL;\n\
  return haris_lib_create_contents(new, info);\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, "static void *haris_lib_create_contents(void *new,\
 const HarisStructureInfo *info)\n\
{\n\
  *((char*)new) = 0;\n\
  for (i = 0; i < info->num_children; i++) {\n\
    switch (info->children[i].child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_SCALAR_LIST:\n\
    case HARIS_STRUCT_LIST:\n\
      list_info = (HarisListInfo*)((char*)new + info->children[i].offset);\n\
      list_info->alloc = list_info->len = 0;\n\
      /* Intentional break omission */\n\
    case HARIS_STRUCT:\n\
      *((void**)((char*)new + info->children[i].offset)) = NULL;\n\
      break;\n\
    }\n\
  }\n\
  return new;\n}\n\n");
  return CJOB_SUCCESS;
}


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
  CJOB_FMT_PRIV_FUNCTION(job, "static void haris_lib_destroy_contents(void *ptr,\
 const HarisStructureInfo *info)\n\
{\n\
  haris_uint32_t j, alloced;\n\
  int i;\n\
  HarisListInfo *list_info;\n\
  HarisStructureInfo *child_structure;\n\
  HarisChild *child;\n\
  for (i = 0; i < info->num_children; i++) {\n\
    child = &info->children[i];\n\
    list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
    switch (info->children[i].child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_SCALAR_LIST:\n\
      if (list_info->alloc > 0)\n\
        free(list_info->ptr);\n\
      break;\n\
    case HARIS_STRUCT_LIST:\n\
      alloced = ((HarisListInfo*)((char*)ptr + child->offset))->alloc;\n\
      child_structure = child->struct_element;\n\
      for (j = 0; j < alloced; j++)\n\
        haris_lib_destroy_contents((char*)list_info->ptr +\n\
                                     j * child_structure->size_of,\n\
                                   child_structure);\n\
    case HARIS_STRUCT:\n\
      free(list_info->ptr);\n\
      break;\n\
    }\n\
  }\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, "static void haris_lib_destroy(void *ptr,\
 const HarisStructureInfo *info)\n\
{\n\
  haris_lib_destroy_contents(ptr, info);\n\
  free(ptr);\n}\n\n");
  return CJOB_SUCCESS;
}

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
  CJOB_FMT_PUB_FUNCTION(job, "HarisStatus %s%s_init_%s(%s%s *strct, \
haris_uint32_t sz)\n\
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
  CJOB_FMT_PUB_FUNCTION(job, "HarisStatus %s%s_init_%s(%s%s *strct)\n\
{\n\
  return haris_lib_init_struct_mem((void*)strct, &haris_lib_structures[%d], \
%d);\n}\n\n", prefix, name, strct->children[field].name, prefix, name, 
              strct->schema_index, field);
  return CJOB_SUCCESS;
}

/* Write the array of in-memory C scalar sizes to the output file.
   This array is keyed by the HarisScalarType enumerators. 
*/
static CJobStatus write_in_memory_scalar_sizes(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, "static const size_t haris_lib_in_memory_scalar_sizes[] = {\n\
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
  CJOB_FMT_SOURCE_STRING(job, "static const size_t haris_lib_message_scalar_sizes[] = {\n\
  1, 1, 2, 2, 4, 4, 8, 8, 4, 8\n\
};\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_message_bit_patterns(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, "static const size_t haris_lib_scalar_bit_patterns[] = {\n\
  0, 0, 1, 1, 2, 2, 3, 3, 2, 3\n\
};\n\n");
  return CJOB_SUCCESS;
}

/* Write the array of scalar-reading functions to the output file; as 
   above, this array is keyed by HarisScalarType.
*/
static CJobStatus write_scalar_readers(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, "static void (* const haris_lib_scalar_readers[])(const unsigned char *, void *) = {\n\
  haris_read_uint8, haris_read_int8, haris_read_uint16, haris_read_int16,\n\
  haris_read_uint32, haris_read_int32, haris_read_uint64, haris_read_int64,\n\
  haris_read_float32, haris_read_float64\n\
};\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_scalar_writers(CJob *job)
{
  CJOB_FMT_SOURCE_STRING(job, "static void (* const haris_lib_scalar_readers[])(unsigned char *, const void *) = {\n\
  haris_write_uint8, haris_write_int8, haris_write_uint16, haris_write_int16,\n\
  haris_write_uint32, haris_write_int32, haris_write_uint64, haris_write_int64,\n\
  haris_write_float32, haris_write_float64\n\
};\n\n");
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
  CJOB_FMT_PRIV_FUNCTION(job, "static HarisStatus haris_lib_init_list_mem(void *ptr,\
const HarisStructureInfo *info, int field, haris_uint32_t sz)\n\
{\n\
  void *testptr;\n\
  HarisChild *child = &info->children[field];\n\
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
  case HARIS_SCALAR_LIST:\n\
    element_size = haris_lib_in_memory_scalar_sizes[child->scalar_element];\n\
    break;\n\
  case HARIS_STRUCTURE_LIST:\n\
    element_size = child->struct_element->size_of;\n\
    break;\n\
  case HARIS_STRUCTURE:\n\
    return HARIS_STRUCTURE_ERROR;\n\
  }\n\
  testptr = realloc(list_info->ptr, sz * element_size);\n\
  if (!testptr) return HARIS_MEM_ERROR;\n\
  list_info->ptr = testptr;\n\
  if (child->child_type == HARIS_STRUCTURE_LIST) {\n\
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
  CJOB_FMT_PRIV_FUNCTION(job, "static HarisStatus haris_lib_init_struct_mem(void *ptr,\
const HarisStructureInfo *info, int field)\n\
{\n\
  void **vdptrptr;\n\
  vdptrptr = (void**)((char*)ptr + info->children[i].offset);\n\
  if (*vdptrptr) return HARIS_SUCCESS;\n\
  if ((*vdptrptr = \n\
      haris_lib_create(info->children[field].struct_element)\n\
      == NULL) return HARIS_MEM_ERROR;\n\
  return HARIS_SUCCESS;\n}\n\n");
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
  CJOB_FMT_PRIV_FUNCTION(job, "static void haris_lib_write_scalar(unsigned char \
*message, const void *src, HarisScalarType type)\n\
{\n  haris_lib_scalar_writers[type](message, src);\n}\n\n");
  return CJOB_SUCCESS;
}

/* Writes the scalar-reading function to the output file. Parameters are
   analogous to the above, except the scalar is READ from the Haris buffer.
*/
static CJobStatus write_scalar_reader_function(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, "static void haris_lib_read_scalar(const unsigned char \
*message, void *src, HarisScalarType type)\n\
{\n  haris_lib_scalar_readers[type](message, src);\n}\n\n");
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
  CJOB_FMT_PRIV_FUNCTION(job, "static unsigned char *haris_lib_write_nonnull_header(\
const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  buf[0] = (unsigned char)0x40 | (unsigned char)info->num_children;\n\
  buf[1] = (unsigned char)info->body_size;\n\
  return buf + 2;\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, "static unsigned char *haris_lib_write_header(\
const void *ptr, const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  if (*(char*)ptr) { buf[0] = 0; return buf + 1; }\n\
  else return haris_lib_write_nonnull_header(info, buf);\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, "static unsigned char *haris_lib_write_body(\
const void *ptr, const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  int i;\n\
  HarisScalarType type;\n\
  if (*(char*)ptr) return buf;\n\
  for (i = 0; i < info->num_scalars; i ++) {\n\
    type = info->scalars[i].type;\n\
    haris_lib_write_scalar(buf, ptr, type);\n\
    ptr += haris_lib_message_scalar_sizes[type];\n\
  }\n  return ptr;\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, "static unsigned char *haris_lib_write_hb(\
const void *ptr, const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  return haris_lib_write_body(ptr, info, \n\
                              haris_lib_write_header(ptr, info, buf));\n\
}\n\n");
  return CJOB_SUCCESS;
}

/* Writes the principal core message-reading function. This function's purpose
   is to read the body of a Haris message structure (pointerd to by buf) into
   the in-memory C structure given by the void *ptr. The info parameter, as 
   always, tells us the type of the structure we are reading.
*/
static CJobStatus write_core_rfuncs(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, "static unsigned char *haris_lib_read_body(const void *ptr, \
const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  int i;\n\
  HarisScalarType type;\n\
  for (i = 0; i < info->num_scalars; i ++) {\n\
    type = info->scalars[i].type;\n\
    haris_lib_read_scalar(buf, ptr, type);\n\
    ptr = (void*)((char*)ptr + haris_lib_message_scalar_sizes[type]);\n\
  }\n  return ptr;\n}\n\n");
  return CJOB_SUCCESS;
}

/* Writes the core size-measuring function to the output file. This function's
   purpose is twofold: first, to find the size of the buffer that would be
   required to completely hold the Haris message given by the in-memory C
   structure ptr. Second, it detects any structural errors in the C structure.
   If the function successfully returns a non-zero size, then the structure can
   be safely transcribed into the Haris format.
*/
static CJobStatus write_core_size(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, "haris_uint32_t haris_lib_size(void *ptr, \
const HarisStructureInfo *info, int depth, HarisStatus *out)\n\
{\n\
  int i, j;\n\
  haris_uint32_t accum, buf;\n\
  HarisChild *child;\n\
  HarisListInfo *list_info;\n\
  if (*(char*)ptr) return 1;\n\
  else if (depth > HARIS_MAXIMUM_DEPTH) {\n\
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
          else if ((accum += buf) > HARIS_MESSAGE_SIZE_LIMIT) {\n\
            *out = HARIS_SIZE_ERROR; return 0;\n\
          }\n\
        }\n\
      }\n\
    case HARIS_CHILD_STRUCT:\n\
      if (*(void**)list_info)\n\
        accum += 1;\n\
      else {\n\
        buf = haris_lib_size((void*)list_info, child->struct_element,\n\
                             depth + 1, out);\n\
        if (buf == 0) return 0;\n\
        else if ((accum += buf) > HARIS_MESSAGE_SIZE_LIMIT) {\n\
          *out = HARIS_SIZE_ERROR; return 0;\n\
        }\n\
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
