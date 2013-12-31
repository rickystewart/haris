#include "cgenc.h"
#include "cgenc_core.h"
#include "cgenc_file.h"
#include "cgenc_buffer.h"

static CJobStatus write_source_protocol_funcs(CJob *job);

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_source_file(CJob *job)
{
  CJobStatus result;
  if ((result = write_source_public_funcs(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_core_funcs(job)) != CJOB_SUCCESS)
    return result;
  if ((result = write_source_protocol_funcs(job)) != CJOB_SUCCESS)
    return result;
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

static CJobStatus write_source_protocol_funcs(CJob *job)
{
  CJobStatus result;
  if (job->protocols.buffer)
    if ((result = write_buffer_protocol_funcs(job)) != CJOB_SUCCESS)
      return result;
  if (job->protocols.file)
    if ((result = write_file_protocol_funcs(job)) != CJOB_SUCCESS)
      return result;
  return CJOB_SUCCESS;
}
