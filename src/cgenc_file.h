#ifndef __CGENC_FILE_H
#define __CGENC_FILE_H

#include "cgen.h"

CJobStatus write_file_static_prototypes(CJob *job, FILE *out);
CJobStatus write_file_protocol_funcs(CJob *, FILE *);

#endif