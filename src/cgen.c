#include "cgen.h"
#include "cgenh.h"
#include "cgenc.h"
#include "cgenu.h"

static CJobStatus check_job(CJob *);
static CJobStatus begin_compilation(CJob *);

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

const char *scalar_type_name(ScalarTag type)
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
    return "haris_float32";
  case SCALAR_FLOAT64:
    return "haris_float64";
  case SCALAR_BOOL:
    return "char";
  default:
    return NULL;
  }
}

FILE *open_source_file(const char *prefix, const char *suffix)
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

