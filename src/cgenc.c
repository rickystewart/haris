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
static CJobStatus write_init_struct(CJob *, ParsedStruct *, int, FILE *)

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

static CJobStatus write_source_core_funcs(CJob *job, FILE *out);
static CJobStatus write_source_protocol_funcs(CJob *job, FILE *out);

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
  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "static unsigned char *%s%s_lib_write_nonnull_header(unsigned char *);\n\
static unsigned char *%s%s_lib_write_header(%s%s *, unsigned char *);\n\
static unsigned char *%s%s_lib_write_body(%s%s *, unsigned char *);\n\
static unsigned char *%s%s_lib_write_hb(%s%s *, unsigned char *);\n\
static void %s%s_lib_read_body(%s%s *, unsigned char *);\n\
static haris_uint32_t %s%s_lib_size(%s%s *, int, HarisStatus *);\n\n",
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0) 
      return CJOB_IO_ERROR;
  }
  return CJOB_SUCCESS;
}

/* The following buffer functions exist for every structure S:
   static HarisStatus _S_from_buffer(S *, unsigned char *, unsigned char **, int);
   static HarisStatus _S_to_buffer(S *, unsigned char *, unsigned char **);

   Finally, we have the following recursive function:
   static unsigned char *handle_child_buffer(unsigned char *, int);
*/
static CJobStatus write_buffer_static_prototypes(CJob *job, FILE *out)
{
  int i;
  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "static HarisStatus _%s%s_from_buffer(%s%s *, unsigned \
char *, unsigned char **, int);\n\
static HarisStatus _%s%s_to_buffer(%s%s *, unsigned char *, unsigned char **);\
\n\n", 
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "static unsigned char *handle_child_buffer(unsigned char *, \
int);\n\n") < 0) 
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

/* The following file functions exist for every structure S:
   static HarisStatus _S_from_file(S *, FILE *, int);
   static HarisStatus _S_to_file(S *, FILE *);
   
   Finally, we have the following recursive function:
   static int handle_child_file(FILE *, int);
*/
static CJobStatus write_file_static_prototypes(CJob *job, FILE *out)
{
  int i;
  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "static HarisStatus _%s%s_from_file(%s%s *, FILE *, int);\
\n\
static HarisStatus _%s%s_to_file(%s%s *, FILE *);\n\n", 
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
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
                    strct->children[i].type.strct.name) < 0)
          return CJOB_IO_ERROR;

        break;
    }
  }
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
                (type_name = strct->children[i].type.struct_list.name)) < 0) 
      return CJOB_IO_ERROR;
    break;
  default:
    if (fprintf(out, "  %s *alloced;\n", 
                (type_name = scalar_type_name(strct->children[i].tag))) < 0) 
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
                strct->children[i].type.struct_list.name,
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
    return CJO_IO_ERROR;
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
              strct->children[i].type.strct.name) < 0)
    return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}