#include "cgenh.h"
#include "cgen.h"

static CJobStatus write_header_boilerplate(CJob *job, FILE *out);
static CJobStatus write_header_macros(CJob *job, FILE *out);
static CJobStatus write_header_structures(CJob *job, FILE *out);
static CJobStatus write_header_prototypes(CJob *job, FILE *out);
static CJobStatus write_header_footer(CJob *job, FILE *out);
static CJobStatus write_child_field(CJob *job, FILE *out, ChildField *child);
static CJobStatus write_public_prototypes(CJob *job, FILE *out);
static CJobStatus write_buffer_prototypes(CJob *job, FILE *out);
static CJobStatus write_file_prototypes(CJob *job, FILE *out);

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_header_file(CJob *job, FILE *out)
{
  CJobStatus result;
  if ((result = write_header_boilerplate(job, out)) != CJOB_SUCCESS)
    return result;
  if ((result = write_header_macros(job, out)) != CJOB_SUCCESS)
    return result;
  if ((result = write_header_structures(job, out)) != CJOB_SUCCESS)
    return result;
  if ((result = write_header_prototypes(job, out)) != CJOB_SUCCESS)
    return result;
  if ((result = write_header_footer(job, out)) != CJOB_SUCCESS)
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
static CJobStatus write_header_boilerplate(CJob *job, FILE *out)
{
  int i;
  char *capital = strdup(job->prefix);
  if (!capital) return CJOB_IO_ERROR;
  for (i=0; capital[i]; i++)
    capital[i] = toupper(capital[i]);
  if (fprintf(out, "#ifndef __HARIS_%s_H\n"
              "#define __HARIS_%s_H\n\n", capital, capital) < 0) 
    return CJOB_IO_ERROR;
  free(capital);
  if (fprintf(out, "%s\n", 
              "#include <stdio.h>\n"
              "#include <stdlib.h>\n\n"
              "#include \"util.h\"\n\n") < 0) return CJOB_IO_ERROR;
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
static CJobStatus write_header_macros(CJob *job, FILE *out)
{
  int i, j;
  for (i=0; i < job->schema->num_enums; i++) {
    if (fprintf(out, "/* enum %s */\n", job->schema->enums[i].name) < 0)
      return CJOB_IO_ERROR;
    for (j=0; j < job->schema->enums[i].num_values; j++) {
      if (fprintf(out, "#define %s%s_%s %d\n", job->prefix, 
                  job->schema->enums[i].name, 
                  job->schema->enums[i].values[j], j) < 0)
        return CJOB_IO_ERROR;
    }
    if (fprintf(out, "\n") < 0) return CJOB_IO_ERROR;
  }
  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "#define %s%s_LIB_BODY_SZ %d\n\
#define %s%s_LIB_NUM_CHILDREN %d\n\n",
                job->prefix, job->schema->structs[i].name,
                job->schema->structs[i].offset,
                job->prefix, job->schema->structs[i].name, 
                job->schema->structs[i].num_children) < 0)
      return CJOB_IO_ERROR;
  }
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
static CJobStatus write_header_structures(CJob *job, FILE *out)
{
  CJobStatus result;
  int i, j;
  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "typedef struct _%s%s %s%s;\n", 
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "\n") < 0) return CJOB_IO_ERROR;

  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "struct _%s%s {\n", 
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
    for (j=0; j < job->schema->structs[i].num_scalars; j++) {
      if (fprintf(out, "  %s %s;\n",
                  scalar_type_name(job->schema->structs[i].scalars[j].type.tag),
                  job->schema->structs[i].scalars[j].name) < 0)
        return CJOB_IO_ERROR;
    }
    for (j=0; j < job->schema->structs[i].num_children; j++) {
      if ((result = write_child_field(job, out, 
                                      job->schema->structs[i].children + j)) !=
          CJOB_SUCCESS) return result;
    }
    if (fprintf(out, "};\n\n") < 0) return CJOB_IO_ERROR;
  }

  return CJOB_SUCCESS;
}

static CJobStatus write_header_prototypes(CJob *job, FILE *out)
{
  CJobStatus result;
  if ((result = write_public_prototypes(job, out)) != CJOB_SUCCESS)
    return result;
  if (job->buffer_protocol)
    if ((result = write_buffer_prototypes(job, out)) != CJOB_SUCCESS)
      return result;
  if (job->file_protocol)
    if ((result = write_file_prototypes(job, out)) != CJOB_SUCCESS)
      return result;
  return CJOB_SUCCESS;
}

static CJobStatus write_header_footer(CJob *job, FILE *out)
{
  (void)job;
  if (fprintf(out, "#endif\n\n") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static CJobStatus write_child_field(CJob *job, FILE *out, ChildField *child)
{
  switch (child->tag) {
  case CHILD_TEXT:
    if (fprintf(out, "  uint32_t _len_%s;\n  uint32_t _alloc_%s;\n\
  char _null_%s;\n  char *%s;\n", 
                child->name, child->name, child->name, child->name) < 0) 
      return CJOB_IO_ERROR;
    break;
  case CHILD_STRUCT:
    if (fprintf(out, "  %s%s *%s;\n", job->prefix, child->type.strct->name,
                child->name) < 0) return CJOB_IO_ERROR;
    break;
  case CHILD_SCALAR_LIST:
    if (fprintf(out, "  uint32_t _len_%s;\n  uint32_t _alloc_%s;\n\
  char _null_%s;\n  %s *%s;\n", 
                child->name, child->name, child->name,
                scalar_type_name(child->type.scalar_list.tag),
                child->name) < 0) return CJOB_IO_ERROR;
    break;
  case CHILD_STRUCT_LIST:
    if (fprintf(out, "  uint32_t _len_%s;\n  uint32_t _alloc_%s;\n\
  char _null_%s;\n  %s%s **%s;\n", 
                child->name, child->name, child->name, job->prefix, 
                child->type.struct_list->name, child->name) < 0) 
      return CJOB_IO_ERROR;
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
static CJobStatus write_public_prototypes(CJob *job, FILE *out)
{
  int i, j;
  const char *prefix, *strct_name, *child_name;
  for (i=0; i < job->schema->num_structs; i++) {
    prefix = job->prefix;
    strct_name = job->schema->structs[i].name;
    if (fprintf(out, "%s%s *%s%s_create(void);\n\
void %s%s_destroy(%s%s *);\n", 
                prefix, strct_name, prefix, strct_name,
                prefix, strct_name, prefix, strct_name) < 0)
      return CJOB_IO_ERROR;
    for (j=0; j < job->schema->structs[i].num_children; j++) {
      child_name = job->schema->structs[i].children[j].name;
      if (job->schema->structs[i].children[j].tag != CHILD_STRUCT) {
        if (fprintf(out, "HarisStatus %s%s_init_%s(%s%s *, haris_uint32_t);\n",
                    prefix, strct_name, child_name, prefix, strct_name) < 0)
          return CJOB_IO_ERROR;
      } else {
        if (fprintf(out, "HarisStatus %s%s_init_%s(%s%s *);\n",
                    prefix, strct_name, child_name, prefix, strct_name) < 0)
          return CJOB_IO_ERROR;
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
static CJobStatus write_buffer_prototypes(CJob *job, FILE *out)
{
  int i;
  const char *prefix, *name;
  for (i=0; i < job->schema->num_structs; i++) {
    prefix = job->prefix;
    name = job->Schema->structs[i].name;
    if (fprintf(out, "HarisStatus %s%s_from_buffer(%s%s *, \
unsigned char *, haris_uint32_t, unsigned char **);\n\
HarisStatus *%s%s_to_buffer(%s%s *, unsigned char **, \
haris_uint32_t *);\n", 
                prefix, name, prefix, name,
                prefix, name, prefix, name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "\n") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

/* The file prototypes for every structure S are 
   HarisStatus S_from_file(S *, FILE *);
   HarisStatus S_to_file(S *, FILE *);
*/
static CJobStatus write_file_prototypes(CJob *job, FILE *out)
{
  int i;
  const char *prefix, *name;
  for (i=0; i < job->schema->num_structs; i++) {
    prefix = job->prefix;
    name = job->schema->structs[i].name;
    if (fprintf(out, "HarisStatus %s%s_from_file(%s%s *, FILE *);\n\
HarisStatus %s%s_to_file(%s%s *, FILE *);\n"
                prefix, name, prefix, name,
                prefix, name, prefix, name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "\n") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}
