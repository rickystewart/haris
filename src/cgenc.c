#include "cgenc.h"

static CJobStatus write_source_boilerplate(CJob *job, FILE *out);
static CJobStatus write_source_prototypes(CJob *job, FILE *out);
static CJobStatus write_source_public_funcs(CJob *job, FILE *out);
static CJobStatus write_source_core_funcs(CJob *job, FILE *out);
static CJobStatus write_source_protocol_funcs(CJob *job, FILE *out);

static CJobStatus write_core_prototypes(CJob *job, FILE *out);
static CJobStatus write_buffer_static_prototypes(CJob *job, FILE *out);
static CJobStatus write_file_static_prototypes(CJob *job, FILE *out);

static CJobStatus write_public_constructor(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_public_destructor(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_public_initializers(CJob *, ParsedStruct *, FILE *);

static CJobStatus write_init_list(CJob *, ParsedStruct *, int, FILE *);
static CJobStatus write_init_struct(CJob *, ParsedStruct *, int, FILE *);

static CJobStatus write_core_wfuncs(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_core_rfuncs(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_core_size(CJob *, ParsedStruct *, FILE *);

static CJobStatus write_buffer_protocol_funcs(CJob *, FILE *);
static CJobStatus write_file_protocol_funcs(CJob *, FILE *);

static CJobStatus write_file_protocol_funcs(CJob *, FILE *);

static CJobStatus write_public_buffer_funcs(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_static_buffer_funcs(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_child_buffer_handler(CJob *, FILE *);

static CJobStatus write_public_file_funcs(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_static_file_funcs(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_child_file_handler(CJob *, FILE *);

static const char *scalar_type_suffix(ScalarTag);

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_source_file(CJob *job)
{
  CJobStatus result;
  FILE *out = open_source_file(job->prefix, ".c");
  if (!out) return CJOB_IO_ERROR;
  if ((result = write_source_boilerplate(job, out)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_prototypes(job, out)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_public_funcs(job, out)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_core_funcs(job, out)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_protocol_funcs(job, out)) != CJOB_SUCCESS)
    return result;
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

static CJobStatus write_source_boilerplate(CJob *job, FILE *out)
{
  if (fprintf(out, "#include <stdio.h>\n"
              "#include <stdlib.h\n\n"
              "#include \"util.h\"\n"
              "#include \"%s.h\"\n\n", job->output) < 0)
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

/* The public-facing prototypes were written in the header file so we don't
   need to include them here. Instead, we only need to statically declare the
   core library functions and the protocol-specific functions. 
*/
static CJobStatus write_source_prototypes(CJob *job, FILE *out)
{
  CJobStatus result;
  if ((result = write_core_prototypes(job, out)) != CJOB_SUCCESS)
    return result;
  if (job->buffer_protocol)
    if ((result = write_buffer_static_prototypes(job, out)) != CJOB_SUCCESS)
      return result;
  if (job->file_protocol)
    if ((result = write_file_static_prototypes(job, out)) != CJOB_SUCCESS)
      return result;
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
static CJobStatus write_source_public_funcs(CJob *job, FILE *out)
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

static CJobStatus write_source_core_funcs(CJob *job, FILE *out)
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

static CJobStatus write_source_protocol_funcs(CJob *job, FILE *out)
{
  CJobStatus result;
  if (job->buffer_protocol)
    if ((result = write_buffer_protocol_funcs(job, out)) != CJOB_SUCCESS)
      return result;
  if (job->file_protocol)
    if ((result = write_file_protocol_funcs(job, out)) != CJOB_SUCCESS)
      return result;
  return CJOB_SUCCESS;
}

/* The following core functions exist for every structure S:
   static unsigned char *S_lib_write_nonnull_header(unsigned char *);
   static unsigned char *S_lib_write_header(S *, unsigned char *);
   static unsigned char *S_lib_write_body(S *, unsigned char *);
   static unsigned char *S_lib_write_hb(S *, unsigned char *);
   static void S_lib_read_body(S *, unsigned char *);
   static haris_uint32_t S_lib_size(S *, int, HarisStatus *);
*/
static CJobStatus write_core_prototypes(CJob *job, FILE *out)
{
  int i;
  const char *prefix, *field;
  for (i=0; i < job->schema->num_structs; i++) {
    prefix = job->prefix;
    field = job->schema->structs[i].name;
    if (fprintf(out, "static unsigned char *%s%s_lib_write_nonnull_header(unsigned char *);\n\
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
                prefix, field) < 0) 
      return CJOB_IO_ERROR;
  }
  return CJOB_SUCCESS;
}

/* The following static buffer functions exist for every structure S:
   static HarisStatus _S_from_buffer(S *, unsigned char *, haris_uint32_t ind,
                                     haris_uint32_t sz,
                                     unsigned char **, int depth);
   static HarisStatus _S_to_buffer(S *, unsigned char *, unsigned char **);

   Finally, we have the following recursive function:
   static unsigned char *handle_child_buffer(unsigned char *, int);
*/
static CJobStatus write_buffer_static_prototypes(CJob *job, FILE *out)
{
  const char *prefix = job->prefix, *name;
  int i;
  for (i=0; i < job->schema->num_structs; i++) {
    name = job->schema->structs[i].name;
    if (fprintf(out, "static HarisStatus _%s%s_from_buffer(%s%s *, unsigned \
char *, haris_uint32_t, haris_uint32_t, haris_uint32_t *, int);\n\
static HarisStatus _%s%s_to_buffer(%s%s *, unsigned char *, unsigned char **);\
\n\n", 
                prefix, name, prefix, name, prefix, name, prefix, name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "static unsigned char *handle_child_buffer(unsigned char *, \
int);\n\n") < 0) 
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

/* The following file functions exist for every structure S:
   static HarisStatus _S_from_file(S *, FILE *, haris_uint32_t *, int);
   static HarisStatus _S_to_file(S *, FILE *);
   
   Finally, we have the following recursive function:
   static int handle_child_file(FILE *, int);
*/
static CJobStatus write_file_static_prototypes(CJob *job, FILE *out)
{
  const char *prefix = job->prefix, *name;
  int i;
  for (i=0; i < job->schema->num_structs; i++) {
    name = job->schema->structs[i].name;
    if (fprintf(out, "static HarisStatus _%s%s_from_file(%s%s *, FILE *, \
haris_uint32_t *, int);\
\n\
static HarisStatus _%s%s_to_file(%s%s *, FILE *);\n\n", 
                prefix, name, prefix, name, prefix, name, prefix, name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "static int handle_child_buffer(FILE *, int);\n\n") < 0) 
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

/* To write the constructor, we need to call "malloc" with the size of the
   structure. We return NULL if the allocation fails, and do the necessary
   initializations: set _null = 0, F = NULL for every child field F, and
   _alloc_F = 0 for every list field F.
*/
static CJobStatus write_public_constructor(CJob *job, ParsedStruct *strct, 
                                           FILE *out)
{
  int i;
  if (fprintf(out, "%s%s *%s%s_create(void)\n\
{\n\
  %s%s *strct = (%s%s)malloc(sizeof *strct);\n\
  if (!strct) return NULL;\n\
  strct->_null = 0;\n", 
              job->prefix, strct->name, job->prefix, strct->name,
              job->prefix, strct->name, job->prefix, strct->name) < 0)
    return CJOB_IO_ERROR;
  for (i=0; i < strct->num_children; i++) {
    switch (strct->children[i].tag) {
    case CHILD_TEXT:
    case CHILD_SCALAR_LIST:
    case CHILD_STRUCT_LIST:
      if (fprintf(out, "  strct->_alloc_%s = 0;\n", 
                  strct->children[i].name) < 0) return CJOB_IO_ERROR;
      /* There is no break here on purpose */
    case CHILD_STRUCT:
      if (fprintf(out, "  strct->%s = NULL;\n",
                  strct->children[i].name) < 0) return CJOB_IO_ERROR;
    }
  }
  if (fprintf(out, "  return strct;\n}\n\n") < 0) return CJOB_IO_ERROR;
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
  if (fprintf(out, "void %s%s_destroy(%s%s *strct)\n{\n",
              job->prefix, strct->name, job->prefix, strct->name) < 0)
    return CJOB_IO_ERROR;
  for (i=0; i < strct->num_children; i++) {
    switch (strct->children[i].tag) {
    case CHILD_TEXT:
    case CHILD_SCALAR_LIST:
      if (fprintf(out, "  if (strct->_alloc_%s > 0) free(strct->%s);\n",
                  strct->children[i].name, strct->children[i].name) < 0)
        return CJOB_IO_ERROR;
      break;
    case CHILD_STRUCT:
      if (fprintf(out, "  if (strct->%s) free(strct->%s);\n", 
                  strct->children[i].name, strct->children[i].name) < 0)
        return CJOB_IO_ERROR;
      break;
    case CHILD_STRUCT_LIST:
      if (fprintf(out, "  if (strct->_alloc_%s > 0) {\n\
    haris_uint32_t i;\n\
    for (i=0; i < strct->_alloc_%s; i++)\n\
      free(strct->%s[i]);\n\
    free(strct->%s);\n\
  }\n",
                  strct->children[i].name, strct->children[i].name,
                  strct->children[i].name, strct->children[i].name) < 0)
        return CJOB_IO_ERROR;
      break;
    }
  }
  if (fprintf(out, "free(strct);\n}\n\n") < 0)
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_public_initializers(CJob *job, ParsedStruct *strct, 
                                            FILE *out)
{
  int i;
  CJobStatus result;
  for (i=0; i < strct->num_children; i++) {
    switch (strct->children[i].tag) {
      case CHILD_TEXT:
      case CHILD_SCALAR_LIST:
      case CHILD_STRUCT_LIST:
        if ((result = write_init_list(job, strct, i, out)) != 
             CJOB_SUCCESS) return result;
      case CHILD_STRUCT:
        if (fprintf(out, "HarisStatus %s%s_init_%s(%s%s *strct)\n{\n\
  %s *alloced;\n",
                    job->prefix, strct->name,
                    strct->children[i].name,
                    job->prefix, strct->name, 
                    strct->children[i].type.strct->name) < 0)
          return CJOB_IO_ERROR;

        break;
    }
  }
  return CJOB_SUCCESS;
}

static CJobStatus write_init_list(CJob *job, ParsedStruct *strct, 
                                  int i, FILE *out)
{
  const char *type_name;
  if (fprintf(out, "HarisStatus %s%s_init_%s(%s%s *strct, \
haris_uint32_t sz)\n{\n", job->prefix, strct->name,
              strct->children[i].name, 
              job->prefix, strct->name) < 0) 
    return CJOB_IO_ERROR;
  switch (strct->children[i].tag) {
  case CHILD_STRUCT_LIST:
    if (fprintf(out, "  haris_uint32_i i;\n  %s **alloced;\n",
                (type_name = strct->children[i].type.struct_list->name)) < 0) 
      return CJOB_IO_ERROR;
    break;
  case CHILD_TEXT:
    if (fprintf(out, "  char *alloced;\n") < 0) return CJOB_IO_ERROR;
    break;
  case CHILD_SCALAR_LIST:
    if (fprintf(out, "  %s *alloced;\n", 
                (type_name = 
                  scalar_type_name(strct->children[i].type.scalar_list.tag))) 
         < 0) 
      return CJOB_IO_ERROR;
    break;
  }
  if (fprintf(out, "  if (strct->_alloc_%s >= sz || sz == 0) goto Success;\n\
  alloced = (%s %s)realloc(strct->%s, sz * sizeof * strct->%s);\n\
  if (!alloced) return HARIS_MEM_ERROR;\n\
  strct->%s = alloced;\n", 
              strct->children[i].name, type_name,
              strct->children[i].tag == CHILD_STRUCT_LIST ? "*" : "**",
              strct->children[i].name, strct->children[i].name,
              strct->children[i].name) < 0) 
    return CJOB_IO_ERROR;
  switch (strct->children[i].tag) {
  case CHILD_STRUCT_LIST:
    if (fprintf(out, "  for (i = strct->_alloc_%s; i < sz; i++) {\n\
    alloced[i] = %s_create();\n\
    if (!alloced[i]) return HARIS_MEM_ERROR;\n\
    strct->_alloc_%s ++;\n  }\n",
                strct->children[i].name, 
                strct->children[i].type.struct_list->name,
                strct->children[i].name) < 0) return CJOB_IO_ERROR;
    break;
  default:
    if (fprintf(out, "  strct->_alloc_%s = sz;\n",
                strct->children[i].name) < 0) return CJOB_IO_ERROR;
    break;
  }
  if (fprintf(out, " Success:\n\
  strct->_len_%s = sz;\n\
  return HARIS_SUCCESS;\n}\n\n", strct->children[i].name) < 0)
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_init_struct(CJob *job, ParsedStruct *strct,
                                    int i, FILE *out)
{
  if (fprintf(out, "HarisStatus %s%s_init_%s(%s%s *strct)\n{\n\
  if (strct->%s) return HARIS_SUCCESS;\n\
  if ((strct->%s = %s_create()) == NULL)\n\
    return HARIS_MEM_ERROR;\n\
  return HARIS_SUCCESS;\n}\n\n",
              job->prefix, strct->name,
              strct->children[i].name,
              job->prefix, strct->name,
              strct->children[i].name,
              strct->children[i].name, 
              strct->children[i].type.strct->name) < 0)
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

/* Writes the core structure-writing functions to the source file. */
static CJobStatus write_core_wfuncs(CJob *job, ParsedStruct *strct, FILE *out)
{
  int i;
  const char *prefix = job->prefix, *name = strct->name;
  if (fprintf(out, "static unsigned char *%s%s_lib_write_nonnull_header(\
unsigned char *buf)\n{\n  buf[0] = (unsigned char)%d;\n  buf[1] = \
(unsigned char)%d;\n  return buf + 2;\n}\n\n",
              prefix, name, strct->num_children, strct->offset) < 0)
    return CJOB_IO_ERROR;
  if (fprintf(out, "static unsigned char *%s%s_lib_write_header(%s%s *strct, \
unsigned char *buf)\n{\n  if (strct->_null) {\n    *buf = (unsigned char)64;\n\
    return buf + 1;\n  } else {\n\
    return %s%s_lib_write_nonnull_header(buf);\n  }\n}\n\n",
              prefix, name, prefix, name, prefix, name) < 0)
    return CJOB_IO_ERROR;
  if (fprintf(out, "static unsigned char *%s%s_lib_write_body(%s%s *strct, \
unsigned char *buf)\n{\n  if (strct->_null) return buf;\n",
              prefix, name, prefix, name) < 0)
    return CJOB_IO_ERROR;
  for (i=0; i < strct->num_scalars; i++) {
    if (fprintf(out, "  haris_write_%s(buf + %d, strct->%s);\n", 
                scalar_type_suffix(strct->scalars[i].type.tag),
                strct->scalars[i].offset, strct->scalars[i].name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "  return buf + %d;\n}\n\n", strct->offset) < 0)
    return CJOB_IO_ERROR;
  if (fprintf(out, "static unsigned char *%s%s_lib_write_hb(%s%s *strct, \
unsigned char *buf)\n{\n  return %s%s_lib_write_body(strct, %s%s_lib_write_\
header(strct, buf));\n}\n\n", 
              prefix, name, prefix, name, prefix, name, prefix, name) < 0)
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

/* Writes the core structure-reading function (S_lib_read_body) to the
   source file.
*/
static CJobStatus write_core_rfuncs(CJob *job, ParsedStruct *strct, FILE *out)
{
  int i;
  const char *prefix = job->prefix, *name = strct->name;
  if (fprintf(out, "static void %s%s_lib_read_body(%s%s *strct, unsigned char *\
buf)\n{\n", prefix, name, prefix, name) < 0) return CJOB_IO_ERROR;
  for (i=0; i < strct->num_scalars; i++) {
    if (fprintf(out, "  strct->%s = haris_read_%s(buf + %d);\n", 
                strct->scalars[i].name, 
                scalar_type_suffix(strct->scalars[i].type.tag),
                strct->scalars[i].offset) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "  return;\n}\n\n") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

/* Writes the core structure size-measurement function to the source file. */
static CJobStatus write_core_size(CJob *job, ParsedStruct *strct, FILE *out)
{
  int i;
  const char *prefix = job->prefix, *name = strct->name;
  if (fprintf(out, "haris_uint32_t %s%s_lib_size(%s%s *strct, int depth, \
HarisStatus *out)\n{\n", prefix, name, prefix, name) < 0)
    return CJOB_IO_ERROR;
  if (strct->num_children == 0) {
    if (fprintf(out, "  if (strct->_null) return 1;\n\
  else return 2 + %s%s_LIB_BODY_SZ;\n}\n\n", prefix, name) < 0)
      return CJOB_IO_ERROR;
  } else {
    if (fprintf(out, "  if (strct->_null) return 1;\n\
  else if (depth > HARIS_MAXIMUM_DEPTH) {\n\
    *out = HARIS_DEPTH_ERROR;\n\
    return 0;\n\
  } else {\n\
    haris_uint32_t accum = 2 + %s%s_LIB_BODY_SZ, buf;\n",
               prefix, name) < 0)
      return CJOB_IO_ERROR;
    for (i = 0; i < strct->num_children; i++) {
      switch (strct->children[i].tag) {
      case CHILD_TEXT:
      case CHILD_SCALAR_LIST:
        if (fprintf(out, "  if (strct->_null_%s) %s\n\
  else {\n\
    accum += 4 + strct->_len_%s * (sizeof %s);\n\
    if (accum > HARIS_MESSAGE_SIZE_LIMIT) \
{ *out = HARIS_SIZE_ERROR; return 0; }\n\
  }\n", strct->children[i].name, 
                    (strct->children[i].nullable ? "return 1;" : 
                     "{ *out = HARIS_STRUCTURE_ERROR; return 0; }"),
                    strct->children[i].name, 
                    (strct->children[i].tag == CHILD_TEXT ? "char" :
                     scalar_type_name(strct->children[i].type.scalar_list.tag)))
            < 0) return CJOB_IO_ERROR;
        break;
      case CHILD_STRUCT:
        if (fprintf(out, "  if (strct->%s->_null) %s\n\
  else {\n\
    if ((buf = %s%s_lib_size(strct->%s, depth + 1, out)) == 0) return 0;\n\
    else if ((accum += buf) > HARIS_MESSAGE_SIZE_LIMIT) \
{ *out = HARIS_SIZE_ERROR; return 0; }\n\
  }\n", strct->children[i].name, 
                    (strct->children[i].nullable ? "return 1;" : 
                     "{ *out = HARIS_STRUCTURE_ERROR; return 0; }"),
                    job->prefix, strct->children[i].type.strct->name,
                    strct->children[i].name) < 0)
          return CJOB_IO_ERROR;
        break;
      case CHILD_STRUCT_LIST:
        if (fprintf(out, "  if (strct->_null_%s) %s\n\
  else {\n\
    haris_uint32_t i;\n\
    accum += 6;\n\
    for (i = 0; i < strct->_len_%s; i++) {\n\
      if ((buf = %s%s_lib_size(strct->%s[i], depth + 1, out)) == 0) return 0;\n\
      else if (buf == 1) { *out = HARIS_STRUCTURE_ERROR; return 0; }\n\
      else if ((accum += buf) > HARIS_MESSAGE_SIZE_LIMIT) \
{ *out = HARIS_SIZE_ERROR; return 0; }\n\
    }\n\
  }\n", strct->children[i].name, 
                    (strct->children[i].nullable ? "return 1;" :
                     "{ *out = HARIS_STRUCTURE_ERROR; return 0; }"),
                    strct->children[i].name, 
                    job->prefix, strct->children[i].type.struct_list->name,
                    strct->children[i].name) < 0)
          return CJOB_IO_ERROR;
      }
    }
    if (fprintf(out, "  return accum;\n}\n\n") < 0) return CJOB_IO_ERROR;
  }
  return CJOB_SUCCESS;
}

/* Writes the buffer protocol functions to the output source stream. They are
   HarisStatus S_from_buffer(S *, unsigned char *, haris_uint32_t,
                             unsigned char **);
   HarisStatus S_to_buffer(S *, unsigned char **, haris_uint32_t *);
   static HarisStatus _S_from_buffer(S *, unsigned char *, 
                                     haris_uint32_t, haris_uint32_t, 
                                     unsigned char **, int);
   static HarisStatus _S_to_buffer(S *, unsigned char *, unsigned char **);
   static unsigned char *handle_child_buffer(unsigned char *, int);
*/
static CJobStatus write_buffer_protocol_funcs(CJob *job, FILE *out)
{
  CJobStatus result;
  int i;
  ParsedSchema *schema = job->schema;
  for (i = 0; i < schema->num_structs; i++) {
    if ((result = write_public_buffer_funcs(job, &schema->structs[i], out)) 
        != CJOB_SUCCESS) return result;
    if ((result = write_static_buffer_funcs(job, &schema->structs[i], out))
        != CJOB_SUCCESS) return result;
  }
  if ((result = write_child_buffer_handler(job, out)) != CJOB_SUCCESS)
    return result;
  return CJOB_SUCCESS;
}

/* Writes the file protocol functions to the output source stream. They are
   HarisStatus S_from_file(S *, FILE *);
   HarisStatus S_to_file(S *, FILE *);
   static HarisStatus _S_from_file(S *, FILE *, haris_uint32_t *, int);
   static HarisStatus _S_to_file(S *, FILE *);
   static int handle_child_file(FILE *, int);
*/
static CJobStatus write_file_protocol_funcs(CJob *job, FILE *out)
{
  CJobStatus result;
  int i;
  ParsedSchema *schema = job->schema;
  for (i = 0; i < schema->num_structs; i++) {
    if ((result = write_public_file_funcs(job, &schema->structs[i], out))
        != CJOB_SUCCESS) return result;
    if ((result = write_static_file_funcs(job, &schema->structs[i], out))
        != CJOB_SUCCESS) return result;
  }
  if ((result = write_child_file_handler(job, out)) != CJOB_SUCCESS)
    return result;
  return CJOB_SUCCESS;
}

/* This function handles the _from_buffer and _to_buffer public functions.
   These functions both primarily call their nonstatic helper functions, though
   they do some error checking to ensure that the structure produces a 
   valid message.
*/
   
static CJobStatus write_public_buffer_funcs(CJob *job, ParsedStruct *strct,
                                            FILE *out)
{
  const char *prefix = job->prefix, *name = strct->name;
  if (fprintf(out, "HarisStatus %s%s_from_buffer(%s%s *strct, \
unsigned char *buf, haris_uint32_t sz, unsigned char **out_addr)\n\
{\n\
  HarisStatus result;\n\
  if ((result = _%s%s_from_buffer(strct, buf, 0, sz, out_addr, 0)) != \n\
      HARIS_SUCCESS) return result;\n\
  if (strct->_null) return HARIS_STRUCTURE_ERROR;\n\
  return HARIS_SUCCESS;\n}\n\n", 
              prefix, name, prefix, name, prefix, name) < 0) 
    return CJOB_IO_ERROR;
  if (fprintf(out, "HarisStatus %s%s_to_buffer(%s%s *strct, \
unsigned char **out_buf, haris_uint32_t *out_sz)\n\
{\n\
  unsigned char *unused;\n\
  HarisStatus result;\n\
  if (strct->_null) return HARIS_STRUCTURE_ERROR;\n\
  *out_sz = %s%s_lib_size(strct, 0, &result);\n\
  if (*out_sz == 0) return result;\n\
  *out_buf = (unsigned char *)malloc(sz);\n\
  if (!*out_buf) return HARIS_MEM_ERROR;\n\
  return _%s%s_to_buffer(strct, buf, &unused);\n}\n\n",
              prefix, name, prefix, name, prefix, name, prefix, name) < 0)
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_static_buffer_funcs(CJob *job, ParsedStruct *strct,
                                            FILE *out)
{
  int i;
  const char *prefix = job->prefix, *name = strct->name;
  if (fprintf(out, "HarisStatus _%s%s_from_buffer(%s%s *strct, \
unsigned char *buf, haris_uint32_t ind, haris_uint32_t sz, haris_uint32_t *\
out_ind, int depth)\n{\n\
  HarisStatus result;\n\
  unsigned char c;\n\
  int num_children, body_size, i;\n\
  unsigned char *msg = buf + ind;\n\
  if (depth > HARIS_DEPTH_LIMIT) return HARIS_DEPTH_ERROR;\n\
  if (ind >= sz || ind > HARIS_MESSAGE_SIZE_LIMIT)\n\
    return HARIS_INPUT_ERROR;\n\
  c = msg[0];\n\
  if (c & 0x80) return HARIS_STRUCTURE_ERROR;\n\
  if (c & 0x40) { strct->_null = 1; *out_ind = ind + 1; \
return HARIS_SUCCESS; }\n\
  if (ind + 1 >= sz || ind + 1 >= HARIS_MESSAGE_SIZE_LIMIT)\n\
    return HARIS_INPUT_ERROR;\n\
  body_size = msg[1];\n\
  num_children = msg[0] & 0x7F;\n\
  if (body_size < %s%s_LIB_BODY_SZ || \n\
      num_children < %s%s_LIB_NUM_CHILDREN) return HARIS_STRUCTURE_ERROR;\n\
  if (ind + body_size >= sz || \n\
      ind + body_size >= HARIS_MESSAGE_SIZE_LIMIT)\n\
    return HARIS_INPUT_ERROR;\n\
  %s%s_lib_read_body(strct, msg + 2);\n",
              prefix, name, prefix, name, prefix, name, prefix, name, 
              prefix, name) < 0) return CJOB_IO_ERROR;
  for (i = 0; i < strct->num_children; i++) {
    switch (strct->children[i].tag) {
      case CHILD_TEXT:
      case CHILD_STRUCT:
      case CHILD_SCALAR_LIST:
      case CHILD_STRUCT_LIST:
    }
  }
  return CJOB_SUCCESS;
}

static CJobStatus write_child_buffer_handler(CJob *job, FILE *out)
{
  return CJOB_SUCCESS;
}

static CJobStatus write_public_file_funcs(CJob *job, ParsedStruct *strct,
                                          FILE *out)
{
  return CJOB_SUCCESS;
}

static CJobStatus write_static_file_funcs(CJob *job, ParsedStruct *strct,
                                          FILE *out)
{
  return CJOB_SUCCESS;
}

static CJobStatus write_child_file_handler(CJob *job, FILE *out)
{
  return CJOB_SUCCESS;
}

static const char *scalar_type_suffix(ScalarTag type)
{
  switch (type) {
  case SCALAR_UINT8:
  case SCALAR_ENUM:
    return "uint8";
  case SCALAR_INT8:
    return "int8";
  case SCALAR_UINT16:
    return "uint16";
  case SCALAR_INT16:
    return "int16";
  case SCALAR_UINT32:
    return "uint32";
  case SCALAR_INT32:
    return "int32";
  case SCALAR_UINT64:
    return "uint64";
  case SCALAR_INT64:
    return "int64";
  case SCALAR_FLOAT32:
    return "float32";
  case SCALAR_FLOAT64:
    return "float64";
  case SCALAR_BOOL:
    return "uint8";
  default:
    return NULL;
  }
}
