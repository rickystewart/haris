#ifndef __CGENC_CORE_H
#define __CGENC_CORE_H

#include "cgen.h"

/* This file contains procedures for writing the core functions of a C
   compilation job.  In particular, this package writes the following
   portions of the .c file:
     1) The public constructors for the structures.
     2) The public destructors for the structures.
     3) The public initializers for the structure members.
     4) All the "core" functions:
        lib_write_nonnull_header
        lib_write_header
        lib_write_body
        lib_write_hb
        lib_read_body
        lib_size
   The functions in group 4 are static, and so are a part of the "private" 
   interface for the generated code. This file writes the prototypes and the 
   definitions for all of these functions.
*/

CJobStatus write_core_prototypes(CJob *job);
CJobStatus write_source_public_funcs(CJob *job);
CJobStatus write_source_core_funcs(CJob *job);

#endif
