#include "cgen.h"
#include "cgenh.h"
#include "cgenc.h"
#include "cgenu.h"
#include "parse.h"

static CJob *new_cjob(void);
static CJobStatus run_cjob(CJob *);
static void destroy_cjob(CJob *);

static int allocate_string_stack(CJobStringStack *, int);
static int push_string(CJobStringStack *, char *);

static CJobStatus check_job(CJob *);
static CJobStatus compile(CJob *);
static CJobStatus output_to_file(CJob *job);
static FILE *open_source_file(const char *prefix, const char *suffix);

/* =============================PUBLIC INTERFACE============================= */


/* C COMMAND-LINE ARGUMENTS:
   -l : Select the language to be generated. For the C compiler, this option
   must be `-l c`.
   -o : Select the name of the output file (that is, if you do `-o hello`, output
   will be written to hello.c and hello.h). If this option is not given, then 
   `haris` will be selected.
   -n : Select name prefix. If no option is given, then the empty string will be
   used as a name prefix.
   -p : Select protocol. Possible protocols, at this time, are `buffer` and `file`.
   You must select at least one protocol.
   -O : Select optimization. Optimizations have not yet been implemented.
*/
CJobStatus cgen_main(int argc, char **argv)
{
  int i;
  CJob *job = new_cjob();
  Parser *parser = create_parser();
  FILE *input;
  CJobStatus result;
  if (!job || !parser) { result = CJOB_MEM_ERROR; goto Finish; }
  /* Loop through command-line arguments, adjusting job as necessary */
  for (i = 1; i < argc; i ++) {
    if (!strcmp(argv[i], "-l")) {
      if (strcmp(argv[i+1], "c")) {
        fprintf(stderr, "Fatal error: C compiler cannot generate code in \
language %s.\n", argv[i+1]);
        return CJOB_JOB_ERROR;
      }
      i ++;
    } else if (!strcmp(argv[i], "-o")) {
      job->output = argv[i+1];
      i++;
    } else if (!strcmp(argv[i], "-n")) {
      job->prefix = argv[i+1];
      i++;
    } else if (!strcmp(argv[i], "-p")) {
      if (!strcmp(argv[i+1], "buffer"))
        job->buffer_protocol = 1;
      else if (!strcmp(argv[i+1], "file")) 
        job->file_protocol = 1;
      else {
        fprintf(stderr, "Unrecognized protocol %s.\n", argv[i+1]);
        return CJOB_JOB_ERROR;
      }
      i++;
    } else if (!strcmp(argv[i], "-O")) {
      fprintf(stderr, "Optimizations are as of yet unimplemented.\n");
      return CJOB_JOB_ERROR;
    } else { /* Strings that aren't command line options are files that we
                are meant to parse and compile */
      input = fopen(argv[i], "r");
      if (!input) { result = CJOB_IO_ERROR; goto Finish; }
      if (!bind_parser(parser, input, argv[i])) { 
        result = CJOB_MEM_ERROR; goto Finish; 
      }
      if (!parse(parser)) {
        result = CJOB_PARSE_ERROR; goto Finish;
      }
      fclose(input);
    }
  }
  /* Make changes to job to reflect optional arguments */
  if (!job->output) job->output = "haris";
  if (!job->prefix) job->prefix = "";
  job->schema = parser->schema;
  /* Run the job */
  result = run_cjob(job);
  Finish:
  if (parser) destroy_parser(parser);
  if (job) destroy_cjob(job);
  return result;
}

CJobStatus add_header_string(CJobStrings *strings, char *s)
{
  if (!push_string(&strings->header_strings, s))
    return CJOB_MEM_ERROR;
  return CJOB_SUCCESS;
}

CJobStatus add_source_string(CJobStrings *strings, char *s)
{
  if (!push_string(&strings->source_strings, s))
    return CJOB_MEM_ERROR;
  return CJOB_SUCCESS;
}

CJobStatus add_public_function(CJobStrings *strings, char *s)
{
  if (!push_string(&strings->public_functions, s))
    return CJOB_MEM_ERROR;
  return CJOB_SUCCESS;
}

CJobStatus add_private_function(CJobStrings *strings, char *s)
{
  if (!push_string(&strings->private_functions, s))
    return CJOB_MEM_ERROR;
  return CJOB_SUCCESS;
}

char *strformat(const char *fmt, ...)
{
  char *ret;
  va_list ap;
  va_start(ap, fmt);
  if (vasprintf(&ret, fmt, ap) < 0) return NULL;
  va_end(ap);
  return ret;
}

const char *scalar_type_suffix(ScalarTag type)
{
  switch (type) {
  case SCALAR_UINT8:
  case SCALAR_ENUM:
  case SCALAR_BOOL:
    return "uint8";
  case SCALAR_INT8:
    return "int8";
  case SCALAR_UINT16:
    return "uint16";
  case SCALAR_INT16:
    return "int16";
  case SCALAR_UINT32:
    return "uint32";
  case SCALAR_INT32:
    return "int32";
  case SCALAR_UINT64:
    return "uint64";
  case SCALAR_INT64:
    return "int64";
  case SCALAR_FLOAT32:
    return "float32";
  case SCALAR_FLOAT64:
    return "float64";
  }
  return NULL;
}

const char *scalar_type_name(ScalarTag type)
{
  switch (type) {
  case SCALAR_UINT8:
  case SCALAR_ENUM:
  case SCALAR_BOOL:
    return "haris_uint8_t";
  case SCALAR_INT8:
    return "haris_int8_t";
  case SCALAR_UINT16:
    return "haris_uint16_t";
  case SCALAR_INT16:
    return "haris_int16_t";
  case SCALAR_UINT32:
    return "haris_uint32_t";
  case SCALAR_INT32:
    return "haris_int32_t";
  case SCALAR_UINT64:
    return "haris_uint64_t";
  case SCALAR_INT64:
    return "haris_int64_t";
  case SCALAR_FLOAT32:
    return "haris_float32";
  case SCALAR_FLOAT64:
    return "haris_float64";
  default:
    return NULL;
  }
}

int scalar_bit_pattern(ScalarTag type)
{
  switch (type) {
  case SCALAR_UINT8:
  case SCALAR_ENUM:
  case SCALAR_BOOL:
  case SCALAR_INT8:
    return 0;
  case SCALAR_UINT16:
  case SCALAR_INT16:
    return 1;
  case SCALAR_UINT32:
  case SCALAR_INT32:
  case SCALAR_FLOAT32:
    return 2;
  case SCALAR_UINT64:
  case SCALAR_INT64:
  case SCALAR_FLOAT64:
    return 3;
  }
  return 0;
}

int sizeof_scalar(ScalarTag type)
{
  switch (type) {
  case SCALAR_UINT8:
  case SCALAR_ENUM:
  case SCALAR_BOOL:
  case SCALAR_INT8:
    return 1;
  case SCALAR_UINT16:
  case SCALAR_INT16:
    return 2;
  case SCALAR_UINT32:
  case SCALAR_INT32:
  case SCALAR_FLOAT32:
    return 4;
  case SCALAR_UINT64:
  case SCALAR_INT64:
  case SCALAR_FLOAT64:
    return 8;
  }
  return 0;
}


/* =============================STATIC FUNCTIONS============================= */

static int init_string_stack(CJobStringStack *stack)
{
  static const int initial_stack_size = 16;
  stack->num_strings = 0;
  stack->strings = NULL;
  return allocate_string_stack(stack, initial_stack_size);
}

static int allocate_string_stack(CJobStringStack *stack, int sz)
{
  int i;
  char **ret = (char**)realloc(stack->strings, sz * sizeof(char*));
  if (!ret) return 0;
  for (i = stack->num_strings; i < sz; i++)
    ret[i] = NULL;
  stack->strings_alloc = sz;
  stack->strings = ret;
  return 1;
}

/* Error-checking: CJOB_MEM_ERROR if s is NULL */
static int push_string(CJobStringStack *stack, char *s)
{
  if (!s) return CJOB_MEM_ERROR;
  if (stack->num_strings == stack->strings_alloc)
    if (!allocate_string_stack(stack, stack->strings_alloc * 2))
      return 0;
  stack->strings[stack->num_strings++] = s;
  return 1;
}

static CJob *new_cjob(void)
{
  CJob *ret = (CJob *)malloc(sizeof *ret);
  if (!ret) goto Failure;
  ret->schema = NULL;
  ret->prefix = NULL;
  ret->output = NULL;
  ret->buffer_protocol = 0;
  ret->file_protocol = 0;
  if (!init_string_stack(&ret->strings.header_strings) ||
      !init_string_stack(&ret->strings.source_strings) ||
      !init_string_stack(&ret->strings.public_functions) ||
      !init_string_stack(&ret->strings.private_functions))
    goto Failure;
  return ret;
  Failure:
  free(ret);
  return NULL;
}

static void destroy_cjob(CJob *job) 
{
  free(job);
  return;
}

/* Run the given CJob (which will entail processing the attached schema and
   writing the resultant generated code to the output files). We enforce
   the invariant that a CJob MUST have a schema, prefix string, and output
   string. This function will return with an error code if any of these
   are missing. Further, at least one protocol must be selected, the schema
   must have at least one structure, every structure in the schema must
   have at least one field, and every enumeration in the schema must have at
   least one value. If the CJob meets all of the requirements, the
   code will be generated and written by this function.

   If you do not wish to add a prefix to your generated names, make the prefix
   an empty string.
*/
static CJobStatus run_cjob(CJob *job)
{
  CJobStatus result;
  if ((result = check_job(job)) != CJOB_SUCCESS) return result;
  return compile(job);
}

static FILE *open_source_file(const char *prefix, const char *suffix)
{
  char *filename = (char*)malloc(strlen(prefix) + strlen(suffix) + 1); 
  FILE *out;
  if (!filename) return NULL;

  *filename = '\0';
  (void)strcat(filename, prefix);
  (void)strcat(filename, suffix);
  out = fopen(filename, "w");
  free(filename);
  return out;
}

static CJobStatus check_job(CJob *job)
{
  int i;
  if (!job->schema || !job->prefix || !job->output || 
      (!job->buffer_protocol && !job->file_protocol))
    return CJOB_JOB_ERROR;
  if (job->schema->num_structs == 0)
    return CJOB_SCHEMA_ERROR;
  for (i = 0; i < job->schema->num_structs; i++) {
    if (job->schema->structs[i].num_scalars == 0 &&
        job->schema->structs[i].num_children == 0)
      return CJOB_SCHEMA_ERROR;
    if (job->schema->structs[i].offset > 256 ||
        job->schema->structs[i].num_children > 63)
      return CJOB_SZ_ERROR;
  }
  for (i = 0; i < job->schema->num_enums; i++) {
    if (job->schema->enums[i].num_values == 0)
      return CJOB_SCHEMA_ERROR;
  }
  return CJOB_SUCCESS;
}

static CJobStatus compile(CJob *job)
{
  CJobStatus result;
  if ((result = write_utility_lib_header(job))
      != CJOB_SUCCESS ||
      (result = write_header_file(job)) 
      != CJOB_SUCCESS ||
      (result = write_utility_lib_source(job))
      != CJOB_SUCCESS ||
      (result = write_source_file(job))
      != CJOB_SUCCESS)
    goto Cleanup;
  return output_to_file(job);
}

static CJobStatus output_to_file(CJob *job)
{
  CJobStatus result;
  FILE *header = open_source_file(job->output, ".h"), 
       *source = open_source_file(job->output, ".c");
  if (!header || !source) { 
    result = CJOB_IO_ERROR;
    goto Cleanup;
  }
  /* WIP */
  Cleanup:
  if (header) fclose(header);
  if (source) fclose(source);
  return result;
}

