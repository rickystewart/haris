#ifndef __CGEN_H
#define __CGEN_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include "schema.h"

#define CJOB_FPRINTF(...) do { if (fprintf(__VA_ARGS__) < 0) \
                                 return CJOB_IO_ERROR; } while (0)
#define CJOB_FMT_HEADER_STRING(job, ...) do \
                          { if (add_header_string(job, strformat(__VA_ARGS__)) \
                                 != CJOB_SUCCESS) \
                              return CJOB_MEM_ERROR; } while (0)
#define CJOB_FMT_SOURCE_STRING(job, ...) do \
                          { if (add_source_string(job, strformat(__VA_ARGS__)) \
                                 != CJOB_SUCCESS) \
                              return CJOB_MEM_ERROR; } while (0)
#define CJOB_FMT_PUB_FUNCTION(job, ...) do \
                          { if (add_public_function(job, strformat(__VA_ARGS__)) \
                                 != CJOB_SUCCESS) \
                              return CJOB_MEM_ERROR; } while (0)
#define CJOB_FMT_PRIV_FUNCTION(job, ...) do \
                          { if (add_private_function(job, strformat(__VA_ARGS__)) \
                                 != CJOB_SUCCESS) \
                              return CJOB_MEM_ERROR; } while (0)

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
  int num_strings;
  int strings_alloc;
  char **strings;
} CJobStringStack;

/* A structure that is used to organize the output. In fact, only one function
   actually writes the output to the output files; the rest of the functions
   store strings in this data structure, which are written out later. There
   are four stacks here; which stack you store a string in decides
   A) which file it is written to. Strings in the header_strings stack are
   written to the header file; the rest are written to the source file.
   B) What, if any, action should be taken with the content of the strings.
   If you store a string in the public_ or private_function stacks, then a 
   prototype will be adapted from the function definition you've given and
   written to the correct place in either the header file or the source file.

   The advantage of using an additional structure is to make it easier to
   extend the compiler or modify its behavior.
*/ 
typedef struct {
  CJobStringStack header_strings; /* Strings that will be copied verbatim into
                                     the header file */
  CJobStringStack source_strings; /* Strings to copy into the .c file */
  CJobStringStack public_functions; /* Functions that are part of the public
                                       interface of the library */
  CJobStringStack private_functions; /* Functions that are statically 
                                        defined */
} CJobStrings;

typedef struct {
  ParsedSchema *schema; /* The schema to be compiled */
  const char *prefix;   /* Prefix all global names with this string */
  const char *output;   /* Write the output code to a file with this name */
  int buffer_protocol;  /* Set if we need to write the buffer protocol to the
                           output */
  int file_protocol;    /* Set if we need to write the file protocol to the 
                           output */
  CJobStrings strings; /* The strings that we will copy into the result source
                          and header files; this is built up dynamically at
                          compile time */
} CJob;

CJobStatus cgen_main(int, char **);

CJobStatus add_header_string(CJobStrings *, char *);
CJobStatus add_source_string(CJobStrings *, char *);
CJobStatus add_public_function(CJobStrings *, char *);
CJobStatus add_private_function(CJobStrins *, char *);

char *strformat(const char *, ...);

const char *scalar_type_suffix(ScalarTag);
int scalar_bit_pattern(ScalarTag type);
int sizeof_scalar(ScalarTag type);
const char *scalar_type_name(ScalarTag);

#endif
