#include "cgenh.h"
#include "cgen.h"

static CJobStatus write_header_boilerplate(CJob *);
static CJobStatus write_header_macros(CJob *);
static CJobStatus write_header_structures(CJob *);
static CJobStatus write_reflective_structures(CJob *);
static CJobStatus write_header_prototypes(CJob *);
static CJobStatus write_header_footer(CJob *);
static CJobStatus write_child_field(CJob *ChildField *child);
static CJobStatus write_public_prototypes(CJob *);
static CJobStatus write_buffer_prototypes(CJob *);
static CJobStatus write_file_prototypes(CJob *);

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_header_file(CJob *job)
{
  CJobStatus result;
  if ((result = write_header_boilerplate(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_header_macros(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_header_structures(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_header_prototypes(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_header_footer(job)) != CJOB_SUCCESS)
    return result;
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

/* For now, the "boilerplate" section of the header will just contain
   the "includes" that the source file is going to require. As we add more
   protocols, this function may need to be edited to reflect those changes.
   For example, running the socket protocol would require that we #include
   the header file for dealing with sockets.
*/
static CJobStatus write_header_boilerplate(CJob *job)
{
  CJobStatus result;
  int i;
  char *capital = strdup(job->prefix);
  if (!capital) return CJOB_IO_ERROR;
  for (i = 0; capital[i]; i++)
    capital[i] = toupper(capital[i]);
  CJOB_FMT_HEADER_STRING(job, "#ifndef __HARIS_%s_H\n\
#define __HARIS_%s_H\n\n", capital, capital);
  free(capital);
  CJOB_FMT_HEADER_STRING(job, "%s\n", 
"#include <stdio.h>\n\
#include <stdlib.h>\n\n");
  return CJOB_SUCCESS;
}

/* We need to define macros for every structure and enumeration in the
   schema. For an enumeration E with a value V (and assuming a prefix P), the 
   generated enumerated name is
   PE_V

   For every structure in C, we define macros to give us 1) the number of
   bytes in the body and 2) the number of children we expect to have
   in each of these structures. The number of bytes in the body is defined
   for a structure S as
   S_LIB_BODY_SZ
   and the number of children is defined as
   S_LIB_NUM_CHILDREN
*/
static CJobStatus write_header_macros(CJob *job)
{
  int i, j;
  char *prefix = job->prefix;
  for (i=0; i < job->schema->num_enums; i++) {
    CJOB_FMT_HEADER_STRING(job, "/* enum %s */\n", 
                                job->schema->enums[i].name);
    for (j=0; j < job->schema->enums[i].num_values; j++) {
      CJOB_FMT_HEADER_STRING(job, "#define %s%s_%s %d\n", 
                  job->prefix, 
                  job->schema->enums[i].name, 
                  job->schema->enums[i].values[j], j);
    }
    CJOB_FMT_HEADER_STRING(job, "\n");
  }
  for (i=0; i < job->schema->num_structs; i++) {
    CJOB_FMT_HEADER_STRING(job, "#define %s%s_LIB_BODY_SZ %d\n\
#define %s%s_LIB_NUM_CHILDREN %d\n\n",
                prefix, job->schema->structs[i].name,
                job->schema->structs[i].offset,
                prefix, job->schema->structs[i].name, 
                job->schema->structs[i].num_children);
  }
  return CJOB_SUCCESS;
}

static CJobStatus write_reflective_structures(CJob *job)
{
  CJOB_FMT_HEADER_STRING(job, "typedef enum {\n\
  HARIS_SCALAR_UINT8, HARIS_SCALAR_INT8, HARIS_SCALAR_UINT16,\n\
  HARIS_SCALAR_INT16, HARIS_SCALAR_UINT32, HARIS_SCALAR_INT32,\n\
  HARIS_SCALAR_UINT64, HARIS_SCALAR_INT64, HARIS_SCALAR_FLOAT32,\n\
  HARIS_SCALAR_FLOAT64, HARIS_SCALAR_BLANK\n\
} HarisScalarType;\n\n");
  CJOB_FMT_HEADER_STRING(job, "typedef enum {\n\
  HARIS_CHILD_TEXT, HARIS_CHILD_SCALAR_LIST, HARIS_CHILD_STRUCT_LIST,\n\
  HARIS_CHILD_STRUCT\n\
} HarisChildType;\n\n");
  CJOB_FMT_HEADER_STRING(job, "typedef struct {\n\
  void *         ptr;\n\
  haris_uint32_t len;\n\
  haris_uint32_t alloc;\n\
  char           null;\n\
} HarisListInfo;\n\n");
  CJOB_FMT_HEADER_STRING(job, "typedef struct HarisStructureInfo_ \
HarisStructureInfo;\n"));
  CJOB_FMT_HEADER_STRING(job, strdup("typedef struct {\n\
  size_t offset;\n\
  HarisScalarType type;\n\
} HarisScalar;\n\n");
  CJOB_FMT_HEADER_STRING(job, "typedef struct {\n\
  int nullable;\n\
  size_t offset;\n\
  HarisScalarType scalar_element;\n\
  HarisStructureInfo *struct_element;\n\
  HarisChildType child_type;\n\
} HarisChild;\n\n");
  CJOB_FMT_HEADER_STRING(job, "struct HarisStructureInfo_ {\n\
  int num_scalars;\n\
  HarisScalar *scalars;\n\
  int num_children;\n\
  HarisChild *children;\n\
  int body_size;\n\
  size_t size_of;\n\
};\n\n");
  return CJOB_SUCCESS;
}

/* We need to make two passes through the structure array to define 
   our structures. First, for every structure S (and assuming a prefix P), 
   we do
     typedef struct _PS PS;
   ... which has the dual effect of performing the typedef and "forward-
   declaring" the structure so that we don't get any nasty compile errors.
   Then, we loop through the structures again and define them. 
*/
static CJobStatus write_header_structures(CJob *job)
{
  CJobStatus result;
  int i, j;
  if ((result = write_reflective_structures(job, out)) != CJOB_SUCCESS)
    return result;
  for (i=0; i < job->schema->num_structs; i++) {
    CJOB_FMT_HEADER_STRING(job, "typedef struct haris_%s %s%s;\n", 
                job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name);
  }
  CJOB_FMT_HEADER_STRING(job, "\n");

  for (i=0; i < job->schema->num_structs; i++) {
    CJOB_FMT_HEADER_STRING(job, "struct haris_%s {\n  char _null;\n",
                           job->schema->structs[i].name);
    for (j=0; j < job->schema->structs[i].num_scalars; j++) {
      CJOB_FMT_HEADER_STRING(job, "  %s %s;\n",
                  scalar_type_name(job->schema->structs[i].scalars[j].type.tag),
                  job->schema->structs[i].scalars[j].name);
    }
    for (j=0; j < job->schema->structs[i].num_children; j++) {
      if ((result = write_child_field(job, out, 
                                      job->schema->structs[i].children + j)) !=
          CJOB_SUCCESS) return result;
    }
    CJOB_FMT_HEADER_STRING(job, "};\n\n");
  }

  return CJOB_SUCCESS;
}

static CJobStatus write_header_prototypes(CJob *job)
{
  CJobStatus result;
  if ((result = write_public_prototypes(job)) != CJOB_SUCCESS)
    return result;
  if (job->buffer_protocol)
    if ((result = write_buffer_prototypes(job)) != CJOB_SUCCESS)
      return result;
  if (job->file_protocol)
    if ((result = write_file_prototypes(job)) != CJOB_SUCCESS)
      return result;
  return CJOB_SUCCESS;
}

static CJobStatus write_header_footer(CJob *job)
{
  CJOB_FMT_HEADER_STRING(job, "#endif\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_child_field(CJob *job, ChildField *child)
{
  char *child_name = child->name;
  switch (child->tag) {
  case CHILD_TEXT:
  case CHILD_SCALAR_LIST:
  case CHILD_STRUCT_LIST:
    CJOB_FMT_HEADER_STRING(job, "  HarisListInfo _%s_info;\n", 
                           child_name);
    break;
  case CHILD_STRUCT:
    CJOB_FMT_HEADER_STRING(job, "  void *%s;\n", child_name);
    break;
  }
  return CJOB_SUCCESS;
}

/* The public prototypes for every structure S are
   S *S_create(void);
   void S_destroy(S *);
   HarisStatus S_init_F(S *, haris_uint64_t);
     ... for every list field F in S, and
   HarisStatus S_init_F(S *);
     ... for every structure field F in S.
*/
static CJobStatus write_public_prototypes(CJob *job)
{
  int i, j;
  const char *prefix, *strct_name, *child_name;
  for (i=0; i < job->schema->num_structs; i++) {
    prefix = job->prefix;
    strct_name = job->schema->structs[i].name;
    CJOB_FMT_HEADER_STRING(job, "%s%s *%s%s_create(void);\n\
void %s%s_destroy(%s%s *);\n", 
                prefix, strct_name, prefix, strct_name,
                prefix, strct_name, prefix, strct_name);
    for (j=0; j < job->schema->structs[i].num_children; j++) {
      child_name = job->schema->structs[i].children[j].name;
      if (job->schema->structs[i].children[j].tag != CHILD_STRUCT) {
        CJOB_FMT_HEADER_STRING(job, "HarisStatus %s%s_init_%s(%s%s *, haris_uint32_t);\n",
                    prefix, strct_name, child_name, prefix, strct_name);
      } else {
        CJOB_FMT_HEADER_STRING(job, "HarisStatus %s%s_init_%s(%s%s *);\n",
                    prefix, strct_name, child_name, prefix, strct_name);
      }
    }
  }
  return CJOB_SUCCESS;
}

/* The buffer prototypes for every structure S are
   HarisStatus S_from_buffer(S *, unsigned char *, haris_uint32_t, 
                             unsigned char **);
   HarisStatus S_to_buffer(S *, unsigned char **, haris_uint32_t *);
*/
static CJobStatus write_buffer_prototypes(CJob *job)
{
  int i;
  const char *prefix, *name;
  for (i=0; i < job->schema->num_structs; i++) {
    prefix = job->prefix;
    name = job->schema->structs[i].name;
    CJOB_FMT_HEADER_STRING(job, "HarisStatus %s%s_from_buffer(%s%s *, \
unsigned char *, haris_uint32_t, unsigned char **);\n\
HarisStatus *%s%s_to_buffer(%s%s *, unsigned char **, \
haris_uint32_t *);\n", 
                prefix, name, prefix, name,
                prefix, name, prefix, name);
  }
  CJOB_FMT_HEADER_STRING(job, "\n");
  return CJOB_SUCCESS;
}

/* The file prototypes for every structure S are 
   HarisStatus S_from_file(S *, FILE *);
   HarisStatus S_to_file(S *, FILE *);
*/
static CJobStatus write_file_prototypes(CJob *job)
{
  int i;
  const char *prefix, *name;
  for (i=0; i < job->schema->num_structs; i++) {
    prefix = job->prefix;
    name = job->schema->structs[i].name;
    CJOB_FMT_HEADER_STRING(job, "HarisStatus %s%s_from_file(%s%s *, FILE *);\n\
HarisStatus %s%s_to_file(%s%s *, FILE *);\n",
                prefix, name, prefix, name,
                prefix, name, prefix, name);
  }
  CJOB_FMT_HEADER_STRING(job, "\n");
  return CJOB_SUCCESS;
}
