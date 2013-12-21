#ifndef __CGENC_BUFFER_H
#define __CGENC_BUFFER_H

#include "cgen.h"

CJobStatus write_buffer_static_prototypes(CJob *job, FILE *out);
CJobStatus write_buffer_protocol_funcs(CJob *job, FILE *out);

#endif