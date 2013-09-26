#include "cgen.h"
#include "cgen_utils.h"

static CJobStatus check_job(CJob *);
static CJobStatus begin_compilation(CJob *);

static CJobStatus write_utility_library(CJob *);
static CJobStatus write_header_file(CJob *);
static CJobStatus write_source_file(CJob *);

static CJobStatus write_header_boilerplate(CJob *, FILE *);
static CJobStatus write_header_macros(CJob *, FILE *);
static CJobStatus write_header_structures(CJob *, FILE *);
static CJobStatus write_header_prototypes(CJob *, FILE *);
static CJobStatus write_header_footer(CJob *, FILE *);

static CJobStatus write_child_field(CJob *, FILE *, ChildField *);

static CJobStatus write_core_prototypes(CJob *, FILE *);
static CJobStatus write_buffer_prototypes(CJob *, FILE *);
static CJobStatus write_file_prototypes(CJob *, FILE *);

static const char *scalar_type_name(ScalarTag);

static FILE *open_source_file(const char *prefix, const char *suffix);

/* =============================PUBLIC INTERFACE============================= */

CJob *new_cjob(void)
{
  CJob *ret = (CJob *)malloc(sizeof *ret);
  if (!ret) return NULL;
  ret->schema = NULL;
  ret->prefix = NULL;
  ret->output = NULL;
  ret->buffer_protocol = 0;
  ret->file_protocol = 0;
  return ret;
}

void destroy_cjob(CJob *job) 
{
  free(job);
  return;
}

/* Run the given CJob (which will entail processing the attached schema and
   writing the resultant generated code to the output files). We enforce
   the invariant that a CJob MUST have a schema, prefix string, and output
   string. This function will return with an error code if any of these
   are missing. Further, at least one protocol must be selected, the schema
   must have at least one structure, every structure in the schema must
   have at least one field, and every enumeration in the schema must have at
   least one value. If the CJob meets all of the requirements, the
   code will be generated and written by this function.

   If you do not wish to add a prefix to your generated names, make the prefix
   an empty string.
*/
CJobStatus run_cjob(CJob *job)
{
  CJobStatus result;
  if ((result = check_job(job)) != CJOB_SUCCESS) return result;
  return begin_compilation(job);
}

/* =============================STATIC FUNCTIONS============================= */

static CJobStatus check_job(CJob *job)
{
  int i;
  if (!job->schema || !job->prefix || !job->output || 
      (!job->buffer_protocol && !job->file_protocol))
    return CJOB_JOB_ERROR;
  if (job->schema->num_structs == 0)
    return CJOB_SCHEMA_ERROR;
  for (i = 0; i < job->schema->num_structs; i++) {
    if (job->schema->structs[i].num_scalars == 0 &&
        job->schema->structs[i].num_children == 0)
      return CJOB_SCHEMA_ERROR;
    if (job->schema->structs[i].offset > 256 ||
        job->schema->structs[i].num_children > 63)
      return CJOB_SZ_ERROR;
  }
  for (i = 0; i < job->schema->num_enums; i++) {
    if (job->schema->enums[i].num_values == 0)
      return CJOB_SCHEMA_ERROR;
  }
  return CJOB_SUCCESS;
}

/* Compilation proceeds through these steps:
   1) Construct the utility library. At this point in time, the utility library
   is a pair of small literal source code files that do not need to be changed. 
   In this step, we simply need to copy the utility files to disk.
   2) Construct the target header file. This job is split into three parts: 
   the macro (enumeration) definitions, the structure definitions, and the
   function prototypes. The "core" function prototypes (those that deal 
   primarily with object allocation and destruction) will need to be
   implemented anyway, but some of the prototypes will be conditional
   on the protocol that we've chosen.
   3) Construct the target source file. This job is split into three parts:
   the static function prototypes, the "core" function definitions, and the
   protocol function definitions, which depend on the protocol we've chosen.
*/
static CJobStatus begin_compilation(CJob *job)
{
  CJobStatus result;
  if ((result = write_utility_library(job)) != CJOB_SUCCESS) return result;
  if ((result = write_header_file(job)) != CJOB_SUCCESS) return result;
  if ((result = write_source_file(job)) != CJOB_SUCCESS) return result;
  return CJOB_SUCCESS;
}

/* The utility library is an unchanging pair of source files stored in a
   pair of normal C strings over in cgen_utils.h.
*/
static CJobStatus write_utility_library(CJob *job)
{
  int i;
  FILE *util_c = fopen("util.c", "w"), *util_h;
  (void)job;
  if (!util_c) return CJOB_IO_ERROR;
  for (i=0; cgen_util_c_contents[i]; i++) 
    if (fprintf(util_c, "%s", cgen_util_c_contents[i]) < 0) 
      return CJOB_IO_ERROR;
  fclose(util_c);

  util_h = fopen("util.h", "w");
  if (!util_h) return CJOB_IO_ERROR;
  for (i=0; cgen_util_h_contents[i]; i++)
    if (fprintf(util_h, "%s", cgen_util_h_contents[i]) < 0) 
      return CJOB_IO_ERROR;
  fclose(util_h);

  return CJOB_SUCCESS;
}

static CJobStatus write_header_file(CJob *job)
{
  CJobStatus result;
  FILE *out = open_source_file(job->prefix, ".h");
  if (!out) return CJOB_IO_ERROR;
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

static CJobStatus write_source_file(CJob *job);

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

/* The only "macros" we need to define are the enumerated values. For an
   enumeration E with a value V (and assuming a prefix P), the generated
   enumerated name is
   PE_V
   All enumerated values are unsigned.
*/
static CJobStatus write_header_macros(CJob *job, FILE *out)
{
  int i, j;
  for (i=0; i < job->schema->num_enums; i++) {
    if (fprintf(out, "/* enum %s */\n", job->schema->enums[i].name) < 0)
      return CJOB_IO_ERROR;
    for (j=0; j < job->schema->enums[i].num_values; j++) {
      if (fprintf(out, "#define %s%s_%s %dU\n", job->prefix, 
                  job->schema->enums[i].name, 
                  job->schema->enums[i].values[j], j) < 0)
        return CJOB_IO_ERROR;
    }
    if (fprintf(out, "\n") < 0) return CJOB_IO_ERROR;
  }
  return CJOB_SUCCESS;
}

/* We need to make two passes through the structure array to define 
   our structures. First, for every structure S (and assuming a prefix P), 
   we do
     typedef struct PS PS;
   ... which has the dual effect of performing the typedef and "forward-
   declaring" the structure so that we don't get any nasty compile errors.
   Then, we loop through the structures again and define them. 
*/
static CJobStatus write_header_structures(CJob *job, FILE *out)
{
  CJobStatus result;
  int i, j;
  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "typedef struct %s%s %s%s;\n", 
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "\n") < 0) return CJOB_IO_ERROR;

  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "struct %s%s {\n", 
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
  if ((result = write_core_prototypes(job, out)) != CJOB_SUCCESS)
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
    if (fprintf(out, "  char *%s;\n", child->name) < 0) return CJOB_IO_ERROR;
    break;
  case CHILD_STRUCT:
    if (fprintf(out, "  %s%s *%s;\n", job->prefix, child->type.strct->name,
                child->name) < 0) return CJOB_IO_ERROR;
    break;
  case CHILD_SCALAR_LIST:
    if (fprintf(out, "  %s *%s;\n", 
                scalar_type_name(child->type.scalar_list.tag),
                child->name) < 0) return CJOB_IO_ERROR;
    break;
  case CHILD_STRUCT_LIST:
    if (fprintf(out, "  %s%s **%s;\n", job->prefix, 
                child->type.struct_list->name, child->name) < 0) 
      return CJOB_IO_ERROR;
    break;
  }
  return CJOB_SUCCESS;
}

/* The core prototypes for every structure S are
   S *S_create(void);
   void S_destroy(S *);
     and
   int S_init_F(S *, haris_uint64_t);
     ... for every list field F in S.
*/
static CJobStatus write_core_prototypes(CJob *job, FILE *out)
{
  int i, j;
  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "%s%s *%s%s_create(void);\n", 
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
    if (fprintf(out, "void %s%s_destroy(%s%s *);\n", 
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
    for (j=0; j < job->schema->structs[i].num_children; j++) {
      if (job->schema->structs[i].children[j].tag != CHILD_STRUCT)
        if (fprintf(out, "int %s%s_init_%s(%s%s *, haris_uint32_t);\n", 
                    job->prefix, job->schema->structs[i].name,
                    job->schema->structs[i].children[j].name,
                    job->prefix, job->schema->structs[i].name) < 0)
          return CJOB_IO_ERROR;
    }
  }
  return CJOB_SUCCESS;
}

/* The buffer prototypes for every structure S are
   HarisStatus S_from_buffer(unsigned char *, S **, unsigned char **);
   HarisStatus S_to_buffer(S *, unsigned char **, haris_uint32_t *);
*/
static CJobStatus write_buffer_prototypes(CJob *job, FILE *out)
{
  int i;
  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "%s%s *%s%s_from_buffer(unsigned char *, \
unsigned char **);\n", job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
    if (fprintf(out, "unsigned char *%s%s_to_buffer(%s%s *, \
haris_uint64_t *);\n", 
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "\n") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

/* The file prototypes for every structure S are 
   S *S_from_file(FILE *);
   int S_to_file(S *, FILE *);
*/
static CJobStatus write_file_prototypes(CJob *job, FILE *out)
{
  int i;
  for (i=0; i < job->schema->num_structs; i++) {
    if (fprintf(out, "%s%s *%s%s_from_file(FILE *);\n", 
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
    if (fprintf(out, "int %s%s_to_file(%s%s *, FILE *);\n", 
                job->prefix, job->schema->structs[i].name,
                job->prefix, job->schema->structs[i].name) < 0)
      return CJOB_IO_ERROR;
  }
  if (fprintf(out, "\n") < 0) return CJOB_IO_ERROR;
  return CJOB_SUCCESS;
}

static const char *scalar_type_name(ScalarTag type)
{
  switch (type) {
  case SCALAR_UINT8:
  case SCALAR_ENUM:
    return "haris_uint8_t";
  case SCALAR_INT8:
    return "haris_int8_t";
  case SCALAR_UINT16:
    return "haris_uint16_t";
  case SCALAR_INT16:
    return "haris_int16_t";
  case SCALAR_UINT32:
    return "haris_uint32_t";
  case SCALAR_INT32:
    return "haris_int32_t";
  case SCALAR_UINT64:
    return "haris_uint64_t";
  case SCALAR_INT64:
    return "haris_int64_t";
  case SCALAR_FLOAT32:
    return "float";
  case SCALAR_FLOAT64:
    return "double";
  case SCALAR_BOOL:
    return "char";
  default:
    return NULL;
  }
}

static FILE *open_source_file(const char *prefix, const char *suffix)
{
  char *filename = (char*)malloc(strlen(prefix) + strlen(suffix) + 1); 
  FILE *out;
  if (!filename) return NULL;

  *filename = '\0';
  (void)strcat(filename, prefix);
  (void)strcat(filename, suffix);
  out = fopen(filename, "w");
  free(filename);
  return out;
}
