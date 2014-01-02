#include "cgenc_file.h"

static CJobStatus write_public_file_funcs(CJob *, ParsedStruct *);
static CJobStatus write_child_file_handler(CJob *);
static CJobStatus write_static_file_funcs(CJob *);

/* =============================PUBLIC INTERFACE============================= */

/* Writes the file protocol functions to the output source stream. They are
   HarisStatus S_from_file(S *, FILE *);
   HarisStatus S_to_file(S *, FILE *);
   static HarisStatus _S_from_file(S *, FILE *, haris_uint32_t *, int);
   static HarisStatus _S_to_file(S *, FILE *);
   static int handle_child_file(FILE *, int);
*/
CJobStatus write_file_protocol_funcs(CJob *job)
{
  (void)job;
  fprintf(stderr, "The file protocol has not been implemented.\n");
  return CJOB_JOB_ERROR;
}

/* =============================STATIC FUNCTIONS============================= */

static CJobStatus write_public_file_funcs(CJob *job, ParsedStruct *strct)
{
  (void)job;
  (void)strct;
  return CJOB_SUCCESS;
}

static CJobStatus write_child_file_handler(CJob *job)
{
  (void)job;
  return CJOB_SUCCESS;
}

static CJobStatus write_static_file_funcs(CJob *job)
{
  (void)job;
  return CJOB_SUCCESS;
}
