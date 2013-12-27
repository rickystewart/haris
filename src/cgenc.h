#ifndef __CGENC_H
#define __CGENC_H

#include "cgen.h"

/* This file contains procedures for writing the source code file (i.e. the
   .c file) of a C compilation job. In fact, this file leverages
   cgenc_core.c (which writes out the "core library" of functions),
   as well as the protocol libraries, cgenc_buffer.c and cgenc_file.c,
   in order to write out the code. 
*/

CJobStatus write_source_file(CJob *, FILE *);

#endif
