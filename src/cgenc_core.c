#include "cgenc_core.h"

/* Functions relating to the public constructors, destructors, and
   initializers */
static CJobStatus write_public_constructor(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_public_destructor(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_public_initializers(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_init_list(CJob *, ParsedStruct *, int, FILE *);
static CJobStatus write_init_struct(CJob *, ParsedStruct *, int, FILE *);

static CJobStatus write_core_wfuncs(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_core_rfuncs(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_core_size(CJob *, ParsedStruct *, FILE *);

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_core_prototypes(CJob *job, FILE *out)
{
  int i;
  const char *prefix, *field;
  for (i=0; i < job->schema->num_structs; i++) {
    prefix = job->prefix;
    field = job->schema->structs[i].name;
    CJOB_FPRINTF(out, "static void *haris_lib_create(const HarisStructureInfo *);\n\
static void *haris_lib_create_contents(void *, const HarisStructureInfo *info);\n\
static void haris_lib_destroy(void *, const HarisStructureInfo *);\n\
static void haris_lib_destroy_contents(void *, const HarisStructureInfo *)\n\
static unsigned char *haris_lib_write_nonnull_header(const HarisStructureInfo *,\
 unsigned char *);\n\
static unsigned char *haris_lib_write_header(void *, const HarisStructureInfo *,\
 unsigned char *);\n\
static unsigned char *haris_lib_write_body(void *, const HarisStructureInfo *,\
 unsigned char *);\n\
static unsigned char *haris_lib_write_hb(void *, const HarisStructureInfo *,\
 unsigned char *);\n\
static void haris_lib_read_body(void *, const HarisStructureInfo *,\
 unsigned char *);\n\
static haris_uint32_t haris_lib_size(void *, const HarisStructureInfo *,\
 int, HarisStatus *);\n\n");
  }
  return CJOB_SUCCESS;
}

/* As the header file says, the public functions are 
   S *S_create(void);
   void S_destroy(S *);
   HarisStatus S_init_F(S *, haris_uint32_t);
     ... for every list field F in S and
   HarisStatus S_init_F(S *);
     ... for every structure field F in S.
*/
CJobStatus write_source_public_funcs(CJob *job, FILE *out)
{
  CJobStatus result;
  int i;
  for (i=0; i < job->schema->num_structs; i++) {
    if ((result = write_public_constructor(job, &job->schema->structs[i], out))
        != CJOB_SUCCESS)
      return result;
    if ((result = write_public_destructor(job, &job->schema->structs[i], out))
        != CJOB_SUCCESS)
      return result;
    if ((result = write_public_initializers(job, &job->schema->structs[i], out))
        != CJOB_SUCCESS)
      return result;
  }
  return CJOB_SUCCESS;
}

CJobStatus write_source_core_funcs(CJob *job, FILE *out)
{
  CJobStatus result;
  int i;
  for (i=0; i < job->schema->num_structs; i++) {
    if ((result = write_core_wfuncs(job, &job->schema->structs[i], out))
        != CJOB_SUCCESS) return result;
    if ((result = write_core_rfuncs(job, &job->schema->structs[i], out))
        != CJOB_SUCCESS) return result;
    if ((result = write_core_size(job, &job->schema->structs[i], out))
        != CJOB_SUCCESS) return result;
  }
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

static CJobStatus write_public_constructor(CJob *job, ParsedStruct *strct, 
                                           FILE *out)
{
  char *prefix = job->prefix, *name = strct->name;
  CJOB_FPRINTF(out, "%s%s *%s%s_create() {
  return (%s%s*)haris_lib_create(&haris_lib_structures[%d]);\n}\n\n"
               prefix, name, prefix, name, prefix, name, strct->schema_index);
  return CJOB_SUCCESS;
}

static CJobStatus write_general_constructor(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "static void *haris_lib_create(const HarisStructureInfo *info)\n\
{\n\
  int i;\n\
  HarisListInfo *list_info;\n\
  void *new = malloc(info->size_of);\n\
  if (!new) return NULL;\n\
  return haris_lib_create_contents(new, info);\n}\n\n");
  CJOB_FPRINTF(out, "static void *haris_lib_create_contents(void *new,\
 const HarisStructureInfo *info)\n\
{\n\
  *((char*)new) = 0; /* Set _null to 0 */\n\
  for (i = 0; i < info->num_children; i++) {\n\
    switch (info->children[i].child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_SCALAR_LIST:\n\
    case HARIS_STRUCT_LIST:\n\
      list_info = (HarisListInfo*)((char*)new + info->children[i].info_offset);\n\
      list_info->alloc = list_info->len = 0;\n\
    case HARIS_STRUCT:\n\
      *((void**)((char*)new + info->children[i].pointer_offset)) = NULL;\n\
    }\n\
  }\n\
  return new;\n}\n\n");
}


/* To write the destructor, we need to loop through all the child elements
   of the structure. If any structures are non-NULL, we need to recursively
   call their destructors as well. For scalar lists, we need to check if the
   _alloc count is greater than 0, and free the pointer if so. For structure
   lists, we need to check if _alloc is greater than 0, and recursively call
   the structure destructors up until that number before freeing the pointer.
*/
static CJobStatus write_public_destructor(CJob *job, ParsedStruct *strct, 
                                          FILE *out)
{
  int i;
  CJOB_FPRINTF(out, "void %s%s_destroy(%s%s *strct)\n\
{\n\
  haris_lib_destroy((void*)strct, &haris_lib_structures[%d]);\n\
}\n\n",
              job->prefix, strct->name, job->prefix, strct->name,
              strct->schema_index);
  return CJOB_SUCCESS;
}

static CJobStatus write_general_destructor(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "static void haris_lib_destroy_contents(void *ptr,\
 const HarisStructureInfo *info)\n\
{\n\
  haris_uint32_t j, alloced;\n\
  int i;\n\
  void *free_me;\n\
  HarisStructureInfo *child_structure;\n\
  HarisChild *child;\n\
  for (i = 0; i < info->num_children; i++) {\n\
    child = &info->children[i];\n\
    free_me = *(void**)((char*)ptr + child->pointer_offset);\n\
    switch (info->children[i].child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_SCALAR_LIST:\n\
      if (((HarisListInfo*)((char*)ptr + child->info_offset))->alloc > 0)\n\
        free(free_me);\n\
      break;\n\
    case HARIS_STRUCT_LIST:\n\
      alloced = ((HarisListInfo*)((char*)ptr + child->info_offset))->alloc;\n\
      child_structure = &haris_lib_structures[child->structure_element];
      for (j = 0; j < alloced; j++)\n\
        haris_lib_destroy_contents((char*)free_me + j * child_structure->size_of,\n\
                                   child_structure);\n\
    case HARIS_STRUCT:\n\
      free(free_me);\n\
      break;\n\
    }\n\
  }\n\
}\n\n");
  CJOB_FPRINTF(out, "static void haris_lib_destroy(void *ptr,\
 const HarisStructureInfo *info)\n\
{\n\
  haris_lib_destroy_contents(ptr, info);\n\
  free(ptr);\n}\n\n");
}

static CJobStatus write_public_initializers(CJob *job, ParsedStruct *strct, 
                                            FILE *out)
{
  int i;
  CJobStatus result;
  char *prefix = job->prefix, *name = strct->name;
  for (i=0; i < strct->num_children; i++) {
    switch (strct->children[i].tag) {
      case CHILD_TEXT:
      case CHILD_SCALAR_LIST:
      case CHILD_STRUCT_LIST:
        if ((result = write_init_list(job, strct, i, out)) != 
             CJOB_SUCCESS) return result;
        break;
      case CHILD_STRUCT:
        if ((result = write_init_struct(job, strct, i, out)) !=
        	 CJOB_SUCCESS) return result;	
        break;	
    }
  }
  return CJOB_SUCCESS;
}

static CJobStatus write_init_list(CJob *job, ParsedStruct *strct, 
                                  int field, FILE *out)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FPRINTF(out, "HarisStatus %s%s_init_%s(%s%s *strct, \
haris_uint32_t sz)\n\
{\n\
  return haris_lib_init_list_mem((void*)strct, &haris_lib_structures[%d], \
%d, sz);\n}\n\n", prefix, name, strct->children[i].name, prefix, name, 
                  strct->schema_index, field);
  return CJOB_SUCCESS;
}

static CJobStatus write_init_struct(CJob *job, ParsedStruct *strct,
                                    int field, FILE *out)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FPRINTF(out, "HarisStatus %s%s_init_%s(%s%s *strct)\n\
{\n\
  return haris_lib_init_struct_mem((void*)strct, &haris_lib_structures[%d], \
%d);\n}\n\n", prefix, name, strct->children[i].name, prefix, name, 
              strct->schema_index, field);
  return CJOB_SUCCESS;
}

static CJobStatus write_in_memory_scalar_sizes(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "static const size_t haris_lib_in_memory_scalar_sizes[] = {\n\
  sizeof(haris_uint8_t), sizeof(haris_int8_t), sizeof(haris_uint16_t),\n\
  sizeof(haris_int16_t), sizeof(haris_uint32_t), sizeof(haris_int32_t),\n\
  sizeof(haris_uint64_t), sizeof(haris_int64_t), sizeof(haris_float32),\n\
  sizeof(haris_float64)\n};\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_message_scalar_sizes(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "static const size_t haris_lib_message_scalar_sizes[] = {\n\
  1, 1, 2, 2, 4, 4, 8, 8, 4, 8\n\
};\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_scalar_readers(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "static const void (*haris_lib_scalar_readers[])(const unsigned char *, void *) = {\n\
  haris_read_uint8, haris_read_int8, haris_read_uint16, haris_read_int16,\n\
  haris_read_uint32, haris_read_int32, haris_read_uint64, haris_read_int64,\n\
  haris_read_float32, haris_read_float64\n\
};\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_general_init_list_member(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "static HarisStatus haris_lib_init_list_mem(void *ptr,\
const HarisStructureInfo *info, int field, haris_uint32_t sz)\n\
{\n\
  void *vdptr, **vdptrptr;\n\
  HarisChild *child = &info->children[field];\n\
  HarisListInfo *list_info = (HarisListInfo*)((char*)ptr + child->info_offset);\n\
  haris_uint32_t i;\n\
  size_t element_size;\n\\
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
    element_size = &haris_lib_structures[child->struct_element]->size_of;\n\
    break;\n\
  case HARIS_STRUCTURE:\n\
    return HARIS_STRUCTURE_ERROR;\n\
  }\n\
  vdptrptr = (void**)((char*)ptr + child->pointer_offset);\n\
  vdptr = realloc(*vdptrptr, sz * element_size);\n\
  if (!vdptr) return HARIS_MEM_ERROR;\n\
  *vdptrptr = vptr;\n\
  if (child->child_type == HARIS_STRUCTURE_LIST) {\n\
    for (i = list_info->alloc; i < sz; i ++) {\n\
      haris_lib_create_contents((char*)vdptr + i * element_size,\n\
                                &haris_lib_structures[child->struct_element]);\n\
      list_info->alloc ++;\n\
    }\n\
  }\n\
  list_info->alloc = sz;\n\
  Success:\n\
    list_info->len = sz;\n\
    return HARIS_SUCCESS;\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_general_init_struct_member(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "static HarisStatus haris_lib_init_struct_mem(void *ptr,\
const HarisStructureInfo *info, int field)\n\
{\n\
  void **vdptrptr;\n\
  vdptrptr = (void**)((char*)ptr + info->children[i].pointer_offset);\n\
  if (*vdptrptr) return HARIS_SUCCESS;\n\
  if ((*vdptrptr = \n\
      haris_lib_create(&haris_lib_structures[info->children[field].struct_element])\n\
      == NULL) return HARIS_MEM_ERROR;\n\
  return HARIS_SUCCESS;\n}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_scalar_writer(CJob *job, FILE *out)
{
  CJOB_FPRINTF("static void haris_lib_write_scalar(unsigned char *message, const void *src, HarisScalarType type)\n\
{\n\
  switch (type) {\n\
  case HARIS_SCALAR_UINT8:\n\
    haris_write_uint8(message, *(haris_uint8_t*)src);\n\
    break;\n\
  case HARIS_SCALAR_INT8:\n\
    haris_write_int8(message, *(haris_int8_t*)src);\n\
    break;\n\
  case HARIS_SCALAR_UINT16:\n\
    haris_write_uint16(message, *(haris_uint16_t*)src);\n\
    break;\n\
  case HARIS_SCALAR_INT16:\n\
    haris_write_int16(message, *(haris_int16_t*)src);\n\
    break;\n\
  case HARIS_SCALAR_UINT32:\n\
    haris_write_uint32(message, *(haris_uint32_t*)src);\n\
    break;\n\
  case HARIS_SCALAR_INT32:\n\
    haris_write_int32(message, *(haris_int32_t*)src);\n\
    break;\n\
  case HARIS_SCALAR_UINT64:\n\
    haris_write_uint64(message, *(haris_uint64_t*)src);\n\
    break;\n\
  case HARIS_SCALAR_INT64:\n\
    haris_write_int64(message, *(haris_int64_t*)src);\n\
    break;\n\
  case HARIS_SCALAR_FLOAT32:\n\
    haris_write_float32(message, *(haris_float32*)src);\n\
    break;\n\
  case HARIS_SCALAR_FLOAT64:\n\
    haris_write_float64(message, *(haris_float64*)src);\n\
    break;\n\
  }\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_scalar_reader(CJob *job, FILE *out)
{
  CJOB_FPRINTF("static void haris_lib_read_scalar(unsigned char *message, const void *src, HarisScalarType type)\n\
{\n  haris_lib_scalar_readers[type](message, src);\n}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_core_wfuncs(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "static unsigned char *haris_lib_write_nonnull_header(\
const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  buf[0] = (unsigned char)0x40 | (unsigned char)info->num_children;\n\
  buf[1] = (unsigned char)info->body_size;\n\
  return buf + 2;\n}\n\n");
  CJOB_FPRINTF(out, "static unsigned char *haris_lib_write_header(\
const void *ptr, const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  if (*(char*)ptr) { buf[0] = 0; return buf + 1; }\n\
  else return haris_lib_write_nonnull_header(info, buf);\n}\n\n");
  CJOB_FPRINTF(out, "static unsigned char *haris_lib_write_body(\
const void *ptr, const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  int i;\n\
  HarisScalarType type;
  if (*(char*)ptr) return buf;\n\
  for (i = 0; i < info->num_scalars; i ++) {\n\
    type = info->scalars[i].type;\n\
    haris_lib_write_scalar(buf, ptr, type);\n\
    ptr += haris_lib_message_scalar_sizes[type];\n\
  }\n  return ptr;\n}\n\n");
  CJOB_FPRINTF(out, "static unsigned char *haris_lib_write_hb(\
const void *ptr, const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  return haris_lib_write_body(ptr, info, \n\
                              haris_lib_write_header(ptr, info, buf));\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_core_rfuncs(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "static unsigned char *haris_lib_read_body(void *ptr, \
const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  int i;\n\
  HarisScalarType type;\n\
  for (i = 0; i < info->num_scalars; i ++) {\n\
    type = info->scalars[i].type;\n\
    haris_lib_read_scalar(buf, ptr, type);\n\
    ptr = (void*)((char*)ptr + haris_lib_message_scalar_sizes[type]);\n\
  }\n  return ptr;\n}\n\n")
}

static CJobStatus write_core_size(CJob *job, FILE *out)
{
  CJOB_FPRINTF(out, "haris_uint32_t haris_lib_size(void *ptr, \
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
    list_info = (HarisListInfo*)((char*)ptr + child->info_offset);
    if (!child->nullable) {\n\
      switch (child->child_type) {\n\
      case HARIS_CHILD_TEXT:\n\
      case HARIS_CHILD_SCALAR_LIST:\n\
      case HARIS_CHILD_STRUCT_LIST:
        if (list_info->null) goto StructureError;\n\
        break;\n\
      case HARIS_CHILD_STRUCT:\n\
        if (**((char**)ptr + child->pointer_offset))\n\
          goto StructureError;\n\
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
          buf = haris_lib_size((char*)(*(void**)((char*)ptr + child->pointer_offset)) + \n\
                                j * &haris_lib_structures[child->structure_element]->size_of,\n\
                                &haris_lib_structures[child->structure_element],\n\
                                depth + 1, out);\n\
          if (buf == 0) return 0;\n\
          else if (buf == 1) goto StructureError;\n\
          else if ((accum += buf) > HARIS_MESSAGE_SIZE_LIMIT) {\n\
            *out = HARIS_SIZE_ERROR; return 0;\n\
          }\n\
        }\n\
      }\n\
    }\n\
  }\n\
  return accum;\n\
  StructureError:\n\
  *out = HARIS_STRUCTURE_ERROR;\n\
  return 0;
}\n\n")
}
