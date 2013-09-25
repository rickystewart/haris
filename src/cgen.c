#include "cgen.h"

static CJobStatus check_job(CJob *);
static CJobStatus begin_compilation(CJob *);

static CJobStatus write_utility_library(CJob *);
static CJobStatus write_header_file(CJob *);
static CJobStatus write_source_file(CJob *);

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

static CJobStatus write_utility_library(CJob *job)
{
  FILE *util_c = fopen("util.c", "w"), util_h;
  if (!util_c) return CJOB_IO_ERROR;
  fprintf(util_c, "%s\n", cgen_util_c_contents);
  fclose(util_c);

  util_h = fopen("util.h", "w");
  if (!util_h) return CJOB_IO_ERROR;
  fprintf(util_h, "%s\n", cgen_util_h_contents);
  fclose(util_h);

  return CJOB_SUCCESS;
}

static CJobStatus write_header_file(CJob *job);
static CJobStatus write_source_file(CJob *job);
