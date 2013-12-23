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
    CJOB_FPRINTF(out, "static unsigned char *%s%s_lib_write_nonnull_header(unsigned char *);\n\
static unsigned char *%s%s_lib_write_header(%s%s *, unsigned char *);\n\
static unsigned char *%s%s_lib_write_body(%s%s *, unsigned char *);\n\
static unsigned char *%s%s_lib_write_hb(%s%s *, unsigned char *);\n\
static void %s%s_lib_read_body(%s%s *, unsigned char *);\n\
static haris_uint32_t %s%s_lib_size(%s%s *, int, HarisStatus *);\n\n",
                prefix, field, prefix, field,
                prefix, field, prefix, field,
                prefix, field, prefix, field,
                prefix, field, prefix, field,
                prefix, field, prefix, field,
                prefix, field);
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

/* To write the constructor, we need to call "malloc" with the size of the
   structure. We return NULL if the allocation fails, and do the necessary
   initializations: set _null = 0, F = NULL for every child field F, and
   _alloc_F = 0 for every list field F.
*/
static CJobStatus write_public_constructor(CJob *job, ParsedStruct *strct, 
                                           FILE *out)
{
  int i;
  CJOB_FPRINTF(out, "%s%s *%s%s_create(void)\n\
{\n\
  %s%s *strct = (%s%s)malloc(sizeof *strct);\n\
  if (!strct) return NULL;\n\
  strct->_null = 0;\n", 
              job->prefix, strct->name, job->prefix, strct->name,
              job->prefix, strct->name, job->prefix, strct->name);
  for (i=0; i < strct->num_children; i++) {
    switch (strct->children[i].tag) {
    case CHILD_TEXT:
    case CHILD_SCALAR_LIST:
    case CHILD_STRUCT_LIST:
      CJOB_FPRINTF(out, "  strct->_alloc_%s = 0;\n", 
                  strct->children[i].name);
      /* There is no break here on purpose */
    case CHILD_STRUCT:
      CJOB_FPRINTF(out, "  strct->%s = NULL;\n",
                  strct->children[i].name);
    }
  }
  CJOB_FPRINTF(out, "  return strct;\n}\n\n");
  return CJOB_SUCCESS;
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
  CJOB_FPRINTF(out, "void %s%s_destroy(%s%s *strct)\n{\n",
              job->prefix, strct->name, job->prefix, strct->name);
  for (i=0; i < strct->num_children; i++) {
    switch (strct->children[i].tag) {
    case CHILD_TEXT:
    case CHILD_SCALAR_LIST:
      CJOB_FPRINTF(out, "  if (strct->_alloc_%s > 0) free(strct->%s);\n",
                  strct->children[i].name, strct->children[i].name);
      break;
    case CHILD_STRUCT:
      CJOB_FPRINTF(out, "  if (strct->%s) free(strct->%s);\n", 
                  strct->children[i].name, strct->children[i].name);
      break;
    case CHILD_STRUCT_LIST:
      CJOB_FPRINTF(out, "  if (strct->_alloc_%s > 0) {\n\
    haris_uint32_t i;\n\
    for (i=0; i < strct->_alloc_%s; i++)\n\
      free(strct->%s[i]);\n\
    free(strct->%s);\n\
  }\n",
                  strct->children[i].name, strct->children[i].name,
                  strct->children[i].name, strct->children[i].name);
      break;
    }
  }
  CJOB_FPRINTF(out, "free(strct);\n}\n\n");
  return CJOB_SUCCESS;
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
                                  int i, FILE *out)
{
  const char *type_name;
  CJOB_FPRINTF(out, "HarisStatus %s%s_init_%s(%s%s *strct, \
haris_uint32_t sz)\n{\n", job->prefix, strct->name,
              strct->children[i].name, 
              job->prefix, strct->name);
  switch (strct->children[i].tag) {
  case CHILD_STRUCT_LIST:
    CJOB_FPRINTF(out, "  haris_uint32_i i;\n  %s **alloced;\n",
                (type_name = strct->children[i].type.struct_list->name));
    break;
  case CHILD_TEXT:
    CJOB_FPRINTF(out, "  char *alloced;\n");
    break;
  case CHILD_SCALAR_LIST:
    CJOB_FPRINTF(out, "  %s *alloced;\n", 
                (type_name = 
                  scalar_type_name(strct->children[i].type.scalar_list.tag)));
    break;
  }
  CJOB_FPRINTF(out, "  if (strct->_alloc_%s >= sz || sz == 0) goto Success;\n\
  alloced = (%s %s)realloc(strct->%s, sz * sizeof * strct->%s);\n\
  if (!alloced) return HARIS_MEM_ERROR;\n\
  strct->%s = alloced;\n", 
              strct->children[i].name, type_name,
              strct->children[i].tag == CHILD_STRUCT_LIST ? "*" : "**",
              strct->children[i].name, strct->children[i].name,
              strct->children[i].name);
  switch (strct->children[i].tag) {
  case CHILD_STRUCT_LIST:
    CJOB_FPRINTF(out, "  for (i = strct->_alloc_%s; i < sz; i++) {\n\
    alloced[i] = %s_create();\n\
    if (!alloced[i]) return HARIS_MEM_ERROR;\n\
    strct->_alloc_%s ++;\n  }\n",
                strct->children[i].name, 
                strct->children[i].type.struct_list->name,
                strct->children[i].name);
    break;
  default:
    CJOB_FPRINTF(out, "  strct->_alloc_%s = sz;\n",
                strct->children[i].name);
    break;
  }
  CJOB_FPRINTF(out, " Success:\n\
  strct->_len_%s = sz;\n\
  return HARIS_SUCCESS;\n}\n\n", strct->children[i].name);
  return CJOB_SUCCESS;
}

static CJobStatus write_init_struct(CJob *job, ParsedStruct *strct,
                                    int i, FILE *out)
{
  char *prefix = job->prefix, *name = strct->name,
       *child_name = strct->children[i].name;
  CJOB_FPRINTF(out, "HarisStatus %s%s_init_%s(%s%s *strct)\n{\n\
  if (strct->%s) return HARIS_SUCCESS;\n\
  if ((strct->%s = %s_create()) == NULL)\n\
    return HARIS_MEM_ERROR;\n\
  return HARIS_SUCCESS;\n}\n\n",
              prefix, name, child_name, prefix, name, child_name, child_name, 
              strct->children[i].type.strct->name);
  return CJOB_SUCCESS;
}

/* Writes the core structure-writing functions to the source file. */
static CJobStatus write_core_wfuncs(CJob *job, ParsedStruct *strct, FILE *out)
{
  int i;
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FPRINTF(out, "static unsigned char *%s%s_lib_write_nonnull_header(\
unsigned char *buf)\n{\n  buf[0] = (unsigned char)%d;\n  buf[1] = \
(unsigned char)%d;\n  return buf + 2;\n}\n\n",
              prefix, name, strct->num_children, strct->offset);
  CJOB_FPRINTF(out, "static unsigned char *%s%s_lib_write_header(%s%s *strct, \
unsigned char *buf)\n{\n  if (strct->_null) {\n    *buf = (unsigned char)64;\n\
    return buf + 1;\n  } else {\n\
    return %s%s_lib_write_nonnull_header(buf);\n  }\n}\n\n",
              prefix, name, prefix, name, prefix, name);
  CJOB_FPRINTF(out, "static unsigned char *%s%s_lib_write_body(%s%s *strct, \
unsigned char *buf)\n{\n  if (strct->_null) return buf;\n",
              prefix, name, prefix, name);
  for (i=0; i < strct->num_scalars; i++) {
    CJOB_FPRINTF(out, "  haris_write_%s(buf + %d, strct->%s);\n", 
                scalar_type_suffix(strct->scalars[i].type.tag),
                strct->scalars[i].offset, strct->scalars[i].name);
  }
  CJOB_FPRINTF(out, "  return buf + %d;\n}\n\n", strct->offset);
  CJOB_FPRINTF(out, "static unsigned char *%s%s_lib_write_hb(%s%s *strct, \
unsigned char *buf)\n{\n  return %s%s_lib_write_body(strct, %s%s_lib_write_\
header(strct, buf));\n}\n\n", 
              prefix, name, prefix, name, prefix, name, prefix, name);
  return CJOB_SUCCESS;
}

/* Writes the core structure-reading function (S_lib_read_body) to the
   source file.
*/
static CJobStatus write_core_rfuncs(CJob *job, ParsedStruct *strct, FILE *out)
{
  int i;
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FPRINTF(out, "static void %s%s_lib_read_body(%s%s *strct, unsigned char *\
buf)\n{\n", prefix, name, prefix, name);
  for (i=0; i < strct->num_scalars; i++) {
    CJOB_FPRINTF(out, "  strct->%s = haris_read_%s(buf + %d);\n", 
                strct->scalars[i].name, 
                scalar_type_suffix(strct->scalars[i].type.tag),
                strct->scalars[i].offset);
  }
  CJOB_FPRINTF(out, "  return;\n}\n\n");
  return CJOB_SUCCESS;
}

/* Writes the core structure size-measurement function to the source file. */
static CJobStatus write_core_size(CJob *job, ParsedStruct *strct, FILE *out)
{
  int i;
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FPRINTF(out, "haris_uint32_t %s%s_lib_size(%s%s *strct, int depth, \
HarisStatus *out)\n{\n", prefix, name, prefix, name);
  if (strct->num_children == 0) {
    CJOB_FPRINTF(out, "  if (strct->_null) return 1;\n\
  else return 2 + %s%s_LIB_BODY_SZ;\n}\n\n", prefix, name);
  } else {
    CJOB_FPRINTF(out, "  if (strct->_null) return 1;\n\
  else if (depth > HARIS_MAXIMUM_DEPTH) {\n\
    *out = HARIS_DEPTH_ERROR;\n\
    return 0;\n\
  } else {\n\
    haris_uint32_t accum = 2 + %s%s_LIB_BODY_SZ, buf;\n",
               prefix, name);
    for (i = 0; i < strct->num_children; i++) {
      switch (strct->children[i].tag) {
      case CHILD_TEXT:
      case CHILD_SCALAR_LIST:
        CJOB_FPRINTF(out, "  if (strct->_null_%s) %s\n\
  else {\n\
    accum += 4 + strct->_len_%s * %d;\n\
    if (accum > HARIS_MESSAGE_SIZE_LIMIT) \
{ *out = HARIS_SIZE_ERROR; return 0; }\n\
  }\n", strct->children[i].name, 
                    (strct->children[i].nullable ? "return 1;" : 
                     "{ *out = HARIS_STRUCTURE_ERROR; return 0; }"),
                    strct->children[i].name, 
                    (strct->children[i].tag == CHILD_TEXT ? 1 :
                     sizeof_scalar(strct->children[i].type.scalar_list.tag)));
        break;
      case CHILD_STRUCT:
        CJOB_FPRINTF(out, "  if (strct->%s->_null) %s\n\
  else {\n\
    if ((buf = %s%s_lib_size(strct->%s, depth + 1, out)) == 0) return 0;\n\
    else if ((accum += buf) > HARIS_MESSAGE_SIZE_LIMIT) \
{ *out = HARIS_SIZE_ERROR; return 0; }\n\
  }\n", strct->children[i].name, 
                    (strct->children[i].nullable ? "return 1;" : 
                     "{ *out = HARIS_STRUCTURE_ERROR; return 0; }"),
                    job->prefix, strct->children[i].type.strct->name,
                    strct->children[i].name);
        break;
      case CHILD_STRUCT_LIST:
        CJOB_FPRINTF(out, "  if (strct->_null_%s) %s\n\
  else {\n\
    haris_uint32_t i;\n\
    accum += 6;\n\
    for (i = 0; i < strct->_len_%s; i++) {\n\
      if ((buf = %s%s_lib_size(strct->%s[i], depth + 1, out)) == 0) return 0;\n\
      else if (buf == 1) { *out = HARIS_STRUCTURE_ERROR; return 0; }\n\
      else if ((accum += buf) > HARIS_MESSAGE_SIZE_LIMIT) \
{ *out = HARIS_SIZE_ERROR; return 0; }\n\
    }\n  }\n", strct->children[i].name, 
                    (strct->children[i].nullable ? "return 1;" :
                     "{ *out = HARIS_STRUCTURE_ERROR; return 0; }"),
                    strct->children[i].name, 
                    job->prefix, strct->children[i].type.struct_list->name,
                    strct->children[i].name);
      }
    }
    CJOB_FPRINTF(out, "  return accum;\n}\n\n") < 0);
  }
  return CJOB_SUCCESS;
}
