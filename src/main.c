#include "cgen.h"
#include "main.h"

static void version(void);
static void usage(void);
static int delegate_compiler(int argc, char **argv);

static int delegate_to_c(int argc, char **argv);

/* Take the appropriate action given the first command-line argument:
   1) If the argument is -h, call usage() and terminate.
   2) If the argument is -l, delegate the command to the correct compiler.
*/
int main(int argc, char **argv)
{
  if (argc <= 1) {
    fprintf(stderr, "Not enough command line arguments provided.\n\
Run `haris -h` for help.\n");
    return 1;
  }
  if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
    usage();
    return 0;
  } else if (!strcmp(argv[1], "-v") || !strcmp(argv[1], "--version")) {
    version();
    return 0;
  } else if (!strcmp(argv[1], "-l")) { 
    return delegate_compiler(argc, argv);
  } else {
    fprintf(stderr, 
"The first command line argument must be -h, -v, or -l; instead, received %s.\n\
Run `haris -h` for help.\n", argv[1]);
    return 1;
  }
  return 0;
}

static void version(void)
{
  fprintf(stdout, "haris 0.0.0\n");
}

static void usage(void)
{
  fprintf(stdout,
"Usage: haris -l [language] [options] file...\n\n\
The Haris tool is a collection of compilers. (The size of this collection, \
currently, is 1.) To show this message, run `haris -h`.\n\n\
To run a compiler, run `haris -l X` where X is an acceptable language code. \
Currently acceptable language codes at this point in time are:\n\
  c\n\n\
The compilers may each have different interfaces, and are expected to be \
called with different arguments and options. To receive help about how to \
use a particular compiler, run `haris -l X -h` where X is the applicable \
language code.\n");
}

/* Given the argc and argv parameters from main, delegate the command to the
   correct compiler (currently, only the C compiler is supported).

   argv[1] is assumed to be "-l" (this is something that should be confirmed
   by the caller).
*/
static int delegate_compiler(int argc, char **argv)
{
  if (argc <= 2) {
    fprintf(stderr, "No language code included after -l option.\n\
Run `haris -h` for help.\n");
    return 1;
  }
  if (!strcmp(argv[2], "c")) {
    return delegate_to_c(argc, argv);
  } else {
    fprintf(stderr, "Unrecognized language code %s.\n\
Run `haris -h` for help.\n", argv[2]);
    return 1;
  }
}

/* Delegate the command to the C compiler. This function's primary job is
   to work with the C compiler interface (the CJobStatus parameter type)
   and return the correct value (0 on success, and another number on error).

   The C compiler is meant to do its own error-reporting (since the C compiler
   knows better than we do about the errors it encounters), so this function
   should not do any output.
*/
static int delegate_to_c(int argc, char **argv)
{
  CJobStatus result = cgen_main(argc, argv);
  if (result == CJOB_SUCCESS) return 0;
  else return 1;
}
