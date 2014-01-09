#ifndef __CGENC_UTIL_H
#define __CGENC_UTIL_H

#include "cgen.h"

/* This file contains the tools for writing the so-called "utility library"
   of the generated code. The utility library is part of the "core library",
   and contains the functions pertinent to writing and reading scalar values
   to and from Haris messages. In particular, the library exposes two 
   functions, haris_lib_write_scalar and haris_lib_read_scalar, which 
   actually implement the reading and writing. (Additionally, the library
   exposes haris_write_uint24 and haris_read_uint24.) These functions are
   the backbone on which the entire generated library is built.
*/

CJobStatus write_utility_library(CJob *);

#endif