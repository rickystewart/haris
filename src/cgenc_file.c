#include "cgenc_file.h"

static CJobStatus write_public_file_funcs(CJob *, ParsedStruct *, FILE *);
static CJobStatus write_child_file_handler(CJob *, FILE *);
static CJobStatus write_static_file_funcs(CJob *, ParsedStruct *, FILE *);

/* The following file functions exist for every structure S:
   static HarisStatus _S_from_file(S *, FILE *, haris_uint32_t *, int);
   static HarisStatus _S_to_file(S *, FILE *);
   
   Finally, we have the following recursive function:
   static int handle_child_file(FILE *, int);
*/
CJobStatus write_file_static_prototypes(CJob *job, FILE *out)
{
  const char *prefix = job->prefix, *name;
  int i;
  for (i=0; i < job->schema->num_structs; i++) {
    name = job->schema->structs[i].name;
    CJOB_FPRINTF(out, "static HarisStatus _%s%s_from_file(%s%s *, FILE *, \
haris_uint32_t *, int);\
\n\
static HarisStatus _%s%s_to_file(%s%s *, FILE *);\n\n", 
                prefix, name, prefix, name, prefix, name, prefix, name);
  }
  CJOB_FPRINTF(out, "static int handle_child_file(FILE *, int);\n\n");
  return CJOB_SUCCESS;
}

/* Writes the file protocol functions to the output source stream. They are
   HarisStatus S_from_file(S *, FILE *);
   HarisStatus S_to_file(S *, FILE *);
   static HarisStatus _S_from_file(S *, FILE *, haris_uint32_t *, int);
   static HarisStatus _S_to_file(S *, FILE *);
   static int handle_child_file(FILE *, int);
*/
CJobStatus write_file_protocol_funcs(CJob *job, FILE *out)
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

static CJobStatus write_public_file_funcs(CJob *job, ParsedStruct *strct,
                                          FILE *out)
{
  return CJOB_SUCCESS;
}

static CJobStatus write_child_file_handler(CJob *job, FILE *out)
{
  return CJOB_SUCCESS;
}

static CJobStatus write_static_file_funcs(CJob *job, ParsedStruct *strct,
                                          FILE *out)
{
  return CJOB_SUCCESS;
}
