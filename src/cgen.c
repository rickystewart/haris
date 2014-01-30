#include "cgen.h"
#include "cgenh.h"
#include "cgenc.h"
#include "parse.h"

/* C COMPILER */

static CJob *new_cjob(void);
static CJobStatus run_cjob(CJob *);
static void destroy_cjob(CJob *);

static int allocate_string_stack(CJobStringStack *, int);
static int push_string(CJobStringStack *, char *);

static void usage(void);
static CJobStatus register_protocol(CJob *, char **, int);
static CJobStatus register_optimization(CJob *, char **, int);
static CJobStatus register_file_to_parse(char **, int, Parser *);

static CJobStatus check_job(CJob *);
static CJobStatus compile(CJob *);
static CJobStatus output_to_file(CJob *job);
static CJobStatus output_to_header_file(CJob *job, FILE *);
static CJobStatus output_to_source_file(CJob *job, FILE *);

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
   -p : Select protocol. Possible protocols, at this time, are `buffer`, 
   `file`, and `fd`. You must select at least one protocol.
   -O : Select optimization. Optimizations have not yet been implemented.
*/
CJobStatus cgen_main(int argc, char **argv)
{
  int i;
  CJob *job = new_cjob();
  Parser *parser = create_parser();
  CJobStatus result;
  if (!job || !parser) { result = CJOB_MEM_ERROR; goto Finish; }
  /* Loop through command-line arguments, adjusting job as necessary */
  for (i = 1; i < argc; i ++) {
    if (!strcmp(argv[i], "-l")) {
      if (i + 1 >= argc) goto ArgumentError;
      if (strcmp(argv[i+1], "c")) {
        fprintf(stderr, "Fatal error: C compiler cannot generate code in \
language %s.\n", argv[i+1]);
        result = CJOB_JOB_ERROR;
        goto Finish;
      }
      i ++;
    } else if (!strcmp(argv[i], "-h")) {
      usage();
      result = CJOB_SUCCESS;
      goto Finish;
    } else if (!strcmp(argv[i], "-o")) {
      if (i + 1 >= argc) goto ArgumentError;
      job->output = argv[i+1];
      i++;
    } else if (!strcmp(argv[i], "-n")) {
      if (i + 1 >= argc) goto ArgumentError;
      job->prefix = argv[i+1];
      i++;
    } else if (!strcmp(argv[i], "-p")) {
      if (i + 1 >= argc) goto ArgumentError;
      if ((result = register_protocol(job, argv, i)) != CJOB_SUCCESS)
        goto Finish;
      i++;
    } else if (!strcmp(argv[i], "-O")) {
      if (i + 1 >= argc) goto ArgumentError;
      if ((result = register_optimization(job, argv, i)) != CJOB_SUCCESS)
        goto Finish;
    } else { /* Strings that aren't command line options are files that we
                are meant to parse and compile */
      if ((result = register_file_to_parse(argv, i, parser)) != CJOB_SUCCESS)
        goto Finish;
    }
  }
  finalize_parser(parser);
  /* Make changes to job to reflect optional arguments */
  if (!job->output) job->output = "haris";
  if (!job->prefix) job->prefix = "";
  job->schema = parser->schema;
  /* Run the job */
  result = run_cjob(job);
  goto Finish;
  ArgumentError:
  fprintf(stderr, "Missing argument after tag %s\n", argv[i]);
  result = CJOB_JOB_ERROR;
  goto Finish;
  Finish:
  if (parser) destroy_parser(parser);
  if (job) destroy_cjob(job);
  return result;
}

CJobStatus add_header_string(CJob *job, char *s)
{
  if (!push_string(&job->strings.header_strings, s))
    return CJOB_MEM_ERROR;
  return CJOB_SUCCESS;
}

CJobStatus add_source_string(CJob *job, char *s)
{
  if (!push_string(&job->strings.source_strings, s))
    return CJOB_MEM_ERROR;
  return CJOB_SUCCESS;
}

CJobStatus add_public_function(CJob *job, char *s)
{
  if (!push_string(&job->strings.public_functions, s))
    return CJOB_MEM_ERROR;
  return CJOB_SUCCESS;
}

CJobStatus add_private_function(CJob *job, char *s)
{
  if (!push_string(&job->strings.private_functions, s))
    return CJOB_MEM_ERROR;
  return CJOB_SUCCESS;
}

char *strformat(const char *fmt, ...)
{
  char *ret;
  va_list ap;
  va_start(ap, fmt);
  if (util_vasprintf(&ret, fmt, ap) < 0) return NULL;
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

/* ********** MANAGING STRING STACKS ********** */

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
  char **ret = (char**)realloc(stack->strings, (unsigned)sz * sizeof(char*));
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
    if (!allocate_string_stack(stack, stack->strings_alloc * 2)) {
      /* Free the string we're trying to push if there's a memory allocation
         error; we would need to free the string anyway, so we might as well
         do it now */
      free(s);
      return 0;
    }
  stack->strings[stack->num_strings++] = s;
  return 1;
}

/* ********** CJOB MEMORY MANAGEMENT ********** */

static CJob *new_cjob(void)
{
  CJob *ret = (CJob *)malloc(sizeof *ret);
  if (!ret) goto Failure;
  ret->schema = NULL;
  ret->prefix = NULL;
  ret->output = NULL;
  ret->protocols.buffer = 0;
  ret->protocols.file = 0;
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

/* ********** PARSING COMMAND-LINE OPTIONS ********** */

static void usage(void)
{
  fprintf(stderr,
"Usage: haris -l c [-o <FNAME>] [-p <PREFIX>] [-O <OPT>] -p <PROTOCOL> \
<ARGUMENT_FILES>...\n\n\
The C compiler, by default, outputs C99-conforming C source code. \
The command line arguments and options that the compiler accepts are as \
follows:\n\
  -o : Write the output to a pair of source files whose names begin with\n\
       <FNAME>. For example, `-o haris` will result in a pair of source\n\
       code files called haris.h and haris.c.\n\
  -p : Prefix the global names in the generated library with <PREFIX>.\n\
       All globally exposed names the compiler generates that do not begin\n\
       with `haris` or `HARIS` will be prefixed with the name you give; this\n\
       can be used to provide a sort of namespacing.\n\
  -O : Use an optimization. Optimizations can decrease the size and speed of\n\
       the output code at the risk of no longer being standard-conforming.\n\
       By default, the compiler chooses to generate code that is slower but\n\
       well-defined under the standard. (No optimizations are currently\n\
       supported; when they are, this space will include a list of\n\
       optimizations.\n\
  -p : Choose a protocol. Acceptable protocols at this time are\n\
         file\n\
         buffer\n\
         fd\n\
       You must choose at least one protocol.\n\
In addition to these options, you must provide at least one <ARGUMENT_FILE>, \
which is the name of a .haris schema file to compile.\n");
}

/* At argv[i] is the "-p" switch; investigate argv[i+1] to determine
   what protocol the user would like to use. */
static CJobStatus register_protocol(CJob *job, char **argv, int i)
{
  if (!strcmp(argv[i+1], "buffer"))
    job->protocols.buffer = 1;
  else if (!strcmp(argv[i+1], "file")) 
    job->protocols.file = 1;
  else if (!strcmp(argv[i+1], "fd"))
    job->protocols.fd = 1;
  else {
    fprintf(stderr, "Unrecognized protocol %s.\n", argv[i+1]);
    return CJOB_JOB_ERROR;
  }
  return CJOB_SUCCESS;
}

/* At argv[i] is the "-O" switch; investigate argv[i+1] to determine
   what optimization the user would like to use. */
static CJobStatus register_optimization(CJob *job, char **argv, int i)
{
  (void)job;
  (void)argv;
  (void)i;
  fprintf(stderr, "Optimizations are as of yet unimplemented.\n");
  return CJOB_JOB_ERROR;
}

/* At argv[i] is the name of a file to open and parse. Run the given
   parser over that input file. */
static CJobStatus register_file_to_parse(char **argv, int i, 
                                         Parser *parser)
{
  CJobStatus result;
  FILE *input = fopen(argv[i], "r");
  if (!input) { 
    fprintf(stderr, "Could not open input file %s.\n", argv[i]);
    result = CJOB_IO_ERROR; 
    goto Finish; 
  }
  if (!bind_parser(parser, input, argv[i])) { 
    fprintf(stderr, "A memory error occured; please try again.\n");
    result = CJOB_MEM_ERROR; 
    goto Finish; 
  }
  if (!parse(parser)) {
    diagnose_parse_error(parser);
    result = CJOB_PARSE_ERROR; 
    goto Finish;
  }
  result = CJOB_SUCCESS;
  Finish:
  if (input) fclose(input);
  return result;
}

/* ********** COMPILATION ********** */

static CJobStatus check_job(CJob *job)
{
  int i;
  const ParsedStruct *strct;
  if (!job->schema || !job->prefix || !job->output)
    return CJOB_JOB_ERROR;
  else if (!job->protocols.buffer && !job->protocols.file) {
    fprintf(stderr, "No protocol selected.\n\
Run `haris -l c -h` for help.\n");
    return CJOB_JOB_ERROR;
  } else if (job->schema->num_structs == 0) {
    fprintf(stderr, "Compiled schema contains no structures.\n");
    return CJOB_SCHEMA_ERROR;
  }
  for (i = 0; i < job->schema->num_structs; i++) {
    strct = &job->schema->structs[i];
    if (strct->num_scalars == 0 && strct->num_children == 0) {
      fprintf(stderr, "Structure %s is empty.\n", 
              strct->name);
      return CJOB_SCHEMA_ERROR;
    } else if (strct->offset > 255) {
      fprintf(stderr, 
              "Structure %s has a body size of %d; 255 is the maximum.\n", 
              strct->name, strct->offset);
      return CJOB_SCHEMA_ERROR;
    } else if (strct->num_children > 63) {
      fprintf(stderr,
              "Structure %s has %d children; 63 is the maximum.\n",
              strct->name, strct->num_children);
      return CJOB_SCHEMA_ERROR;
    }
  }
  for (i = 0; i < job->schema->num_enums; i++) {
    if (job->schema->enums[i].num_values == 0) {
      fprintf(stderr, "Enum %s is empty.\n", job->schema->enums[i].name);
      return CJOB_SCHEMA_ERROR;
    }
  }
  return CJOB_SUCCESS;
}

/* Given a CJob, which is assumed to be valid, compile the CJob into 
   a pair of C files. This is where the magic actually happens:
   all of the strings that will make up the header and source files
   are generated and allocated, and the `output_to_file` function
   actually writes them out to disk. */
static CJobStatus compile(CJob *job)
{
  CJobStatus result;
  if ((result = write_header_file(job)) != CJOB_SUCCESS || /* cgenh.c */
      (result = write_source_file(job)) != CJOB_SUCCESS || /* cgenc.c */
      (result = output_to_file(job)) != CJOB_SUCCESS) /* cgen.c (this file) */
    goto Failure;
  return CJOB_SUCCESS;
  Failure:
  switch (result) {
  case CJOB_IO_ERROR:
    fprintf(stderr, "An I/O error occured. Please try again.\n");
    break;
  case CJOB_MEM_ERROR:
    fprintf(stderr, "A memory allocation error occured. Please try again.\n");
    break;
  default:
    fprintf(stderr, "An unexpected error occured; error code %d\n", result);
    break;
  }
  return result;
}

/* ********** OUTPUT ********** */
/* All of the compilation functions actually just allocate a bunch of 
   strings and store them in the CJob -- file output doesn't actually
   happen until the end. These are the functions that manipulate the
   strings and actually write them out to disk. */

static FILE *open_source_file(const char *prefix, const char *suffix)
{
  char *filename = (char*)malloc(strlen(prefix) + strlen(suffix) + 1); 
  FILE *out;
  if (!filename) return NULL;
  sprintf(filename, "%s%s", prefix, suffix);
  out = fopen(filename, "w");
  free(filename);
  return out;
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
  if ((result = output_to_header_file(job, header)) != CJOB_SUCCESS)
    return result;
  if ((result = output_to_source_file(job, source)) != CJOB_SUCCESS)
    return result;
  Cleanup:
  if (header) fclose(header);
  if (source) fclose(source);
  return result;
}

/* Consumes a file and a function definition as a C string and outputs the
   prototype. For example, if the definition is "int a() { return 0; }", then
   this function will write "int a();\n" to the file.

   The definition must have an '{' or this function will have undefined 
   behavior. Of course, the string is not a syntactically correct function 
   definition unless it does have one.
*/
static CJobStatus output_prototype(FILE *out, char *definition)
{
  int i;
  for (i = 0; definition[i] != '{'; i ++);
  for (i--; isspace(definition[i]); i --);
  CJOB_FPRINTF(out, "%.*s;\n", i + 1, definition);
  return CJOB_SUCCESS;
}

/* Write everything that goes in the header file to the given output FILE*. */
static CJobStatus output_to_header_file(CJob *job, FILE *out)
{
  int i;
  CJobStatus result;
  CJOB_FPRINTF(out, 
"#ifndef HARIS_H__ \n\
#define HARIS_H__ \n\n");
  for (i = 0; i < job->strings.header_strings.num_strings; i ++)
    CJOB_FPRINTF(out, "%s", job->strings.header_strings.strings[i]);
  CJOB_FPRINTF(out, "\n\n");
  for (i = 0; i < job->strings.public_functions.num_strings; i ++)
    if ((result = output_prototype(out, job->strings.public_functions.strings[i]))
        != CJOB_SUCCESS)
      return result;
  CJOB_FPRINTF(out, "#endif\n\n");
  return CJOB_SUCCESS;
}

/* If the given filename has one or more slashes (/) in it, then everything
   preceding the final slash is the directory path to the file. This function
   returns a pointer to the proper filename of the file, not including the
   directories. For example, if filename is passed as "path/to/hello.txt", 
   then this function returns a pointer to "hello.txt".

   This function returns a pointer within the string itself and as such the
   return value should not be freed.
*/
static const char *find_proper_filename(const char *filename)
{
  const char *buf;
  while((buf = strchr(filename, '/')) != NULL)
    filename = buf + 1;
  return filename;
}

/* Write everything that goes in the source file to the given output FILE*. */
static CJobStatus output_to_source_file(CJob *job, FILE *out)
{
  int i;
  CJobStatus result;
  CJOB_FPRINTF(out, 
"#include <stdio.h>\n\
#include <stddef.h>\n\
#include <stdlib.h>\n\n\
#include \"%s.h\"\n\n", find_proper_filename(job->output));
  for (i = 0; i < job->strings.private_functions.num_strings; i ++)
    if ((result = output_prototype(out, job->strings.private_functions.strings[i]))
        != CJOB_SUCCESS)
      return result;
  CJOB_FPRINTF(out, "\n\n");
  for (i = 0; i < job->strings.source_strings.num_strings; i ++) 
    CJOB_FPRINTF(out, "%s", job->strings.source_strings.strings[i]);
  for (i = 0; i < job->strings.private_functions.num_strings; i ++)
    CJOB_FPRINTF(out, "%s", job->strings.private_functions.strings[i]);
  for (i = 0; i < job->strings.public_functions.num_strings; i ++)
    CJOB_FPRINTF(out, "%s", job->strings.public_functions.strings[i]);
  CJOB_FPRINTF(out, "\n\n");
  return CJOB_SUCCESS;
}
