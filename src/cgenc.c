#include "cgenc.h"

static CJobStatus write_source_boilerplate(CJob *job, FILE *out);
static CJobStatus write_source_prototypes(CJob *job, FILE *out);
static CJobStatus write_source_public_funcs(CJob *job, FILE *out);
static CJobStatus write_source_core_funcs(CJob *job, FILE *out);
static CJobStatus write_source_protocol_funcs(CJob *job, FILE *out);

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

static CJobStatus write_source_public_funcs(CJob *job, FILE *out);
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
