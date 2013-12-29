#include "cgenc.h"
#include "cgenc_core.h"
#include "cgenc_file.h"
#include "cgenc_buffer.h"

static CJobStatus write_source_boilerplate(CJob *job);
static CJobStatus write_source_prototypes(CJob *job);
static CJobStatus write_source_protocol_funcs(CJob *job);

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_source_file(CJob *job)
{
  CJobStatus result;
  if ((result = write_source_boilerplate(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_prototypes(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_public_funcs(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_core_funcs(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_protocol_funcs(job)) != CJOB_SUCCESS)
    return result;
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

static CJobStatus write_source_boilerplate(CJob *job)
{
  CJOB_FMT_HEADER_STRING(job, "#include <stdio.h>\n\
#include <stdlib.h\n\
#include \"%s.h\"\n\n", job->output);
  return CJOB_SUCCESS;
}

/* The public-facing prototypes were written in the header file so we don't
   need to include them here. Instead, we only need to statically declare the
   core library functions and the protocol-specific functions. 
*/
static CJobStatus write_source_prototypes(CJob *job)
{
  CJobStatus result;
  if ((result = write_core_prototypes(job)) != CJOB_SUCCESS)
    return result;
  if (job->buffer_protocol)
    if ((result = write_buffer_static_prototypes(job)) != CJOB_SUCCESS)
      return result;
  if (job->file_protocol)
    if ((result = write_file_static_prototypes(job)) != CJOB_SUCCESS)
      return result;
  return CJOB_SUCCESS;
}

static CJobStatus write_source_protocol_funcs(CJob *job)
{
  CJobStatus result;
  if (job->buffer_protocol)
    if ((result = write_buffer_protocol_funcs(job)) != CJOB_SUCCESS)
      return result;
  if (job->file_protocol)
    if ((result = write_file_protocol_funcs(job)) != CJOB_SUCCESS)
      return result;
  return CJOB_SUCCESS;
}
