#ifndef __CGEN_H
#define __CGEN_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "schema.h"

#define CJOB_FPRINTF(...) do { if (fprintf(__VA_ARGS__) < 0) \
                                 return CJOB_IO_ERROR; } while (0)

/* Main entry point for the C compiler. The main point of interest here is 
   the cgen_main() function, which accepts as its input the `argv` and `argc` 
   that were passed to the main() function. This function processes the 
   command-line arguments, constructs a CJob, and runs the CJob if all the 
   command-line arguments could be successfully processed and the parsing 
   succeeded.

   In sum, a compiled Haris library is made up, roughly, of two parts:
   1) The core library. This body of code is largely unchanging, and
   contains functions that can be used to construct in-memory C structures,
   destroy these same in-memory C structures, and convert between these
   structures and small in-memory buffers. In short, the library contains
   the code that's necessary for a Haris runtime to work, no matter what
   protocol we're generating.
   2) The protocol library (libraries). This section builds off of the
   core library, and contains the code that will transmit Haris messages
   along the chosen protocol. 

   Information about the content and implementation of these libraries can
   be found in the relevant headers.
*/

typedef enum {
  CJOB_SUCCESS, CJOB_SCHEMA_ERROR, CJOB_SZ_ERROR, CJOB_JOB_ERROR, CJOB_IO_ERROR,
  CJOB_MEM_ERROR, CJOB_PARSE_ERROR
} CJobStatus;

typedef struct {
  ParsedSchema *schema; /* The schema to be compiled */
  const char *prefix;   /* Prefix all global names with this string */
  const char *output;   /* Write the output code to a file with this name */
  int buffer_protocol;  /* Set if we need to write the buffer protocol to the
                           output */
  int file_protocol;    /* Set if we need to write the file protocol to the 
                           output */
} CJob;

CJobStatus cgen_main(int, char **);

const char *scalar_type_suffix(ScalarTag);
int scalar_bit_pattern(ScalarTag type);
int sizeof_scalar(ScalarTag type);
const char *scalar_type_name(ScalarTag);

#endif
