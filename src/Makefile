OBJS = util.o cgen.o cgenc.o cgenc_buffer.o cgenc_core.o cgenc_file.o \
cgenc_util.o cgenc_fd.o cgenh.o hash.o lex.o parse.o schema.o main.o
RESULT = haris

TEST_FILES = test/simple.haris.c
TEST_HEADERS = $(TEST_FILES:.c=.h)

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Wconversion -Wsign-conversion -O3 -std=c99

HARIS_FLAGS = -p buffer -p file

all:	$(RESULT)

$(RESULT):	$(OBJS)
	$(CC) $(CFLAGS) -o$(RESULT) $(OBJS)

.SUFFIXES : 

%.o:	%.c %.h
	$(CC) $(CFLAGS) -c $< -o $@ 

test/%.haris.c: test/%.haris
	./haris -l c -o $< $(HARIS_FLAGS) $<

# The testing framework doesn't currently test the compiler code, which is 
# suitably simple for our purposes. Instead, we're sort of testing the
# "public interface" of the compiler, or the generated code. `make precheck`
# compiles all of the test schemas and writes the output files to the /test
# directory. This prepares the /test directory for the running of the unit
# tests; the /test directory, after precompilation, can be sent to any
# computer to run the tests without actually requiring the Haris compiler
# itself.
precheck: all $(TEST_FILES)

# Make the precheck (that is, all the code that is required to compile
# the tests), but also enter the test directory and run them.
check: precheck
	cd test && $(MAKE) -C check

clean:
	rm $(OBJS) $(TEST_FILES) $(TEST_HEADERS) $(RESULT)
