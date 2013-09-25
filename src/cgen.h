#ifndef __CGEN_H
#define __CGEN_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "schema.h"
#include "cgen_utils.h"

typedef enum {
  CJOB_SUCCESS, CJOB_SCHEMA_ERROR, CJOB_JOB_ERROR, CJOB_IO_ERROR
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

CJob *new_cjob(void);
CJobStatus run_cjob(CJob *);
void destroy_cjob(CJob *);

#endif
