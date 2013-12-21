#ifndef __CGENC_H
#define __CGENC_H

"include cgen.h"

/* This file contains procedures for writing the source code file (i.e. the
   .c file) of a C compilation job.
*/

CJobStatus write_source_file(CJob *, FILE *);

#endif
