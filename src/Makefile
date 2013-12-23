OBJS = cgen.o cgenc.o cgenc_buffer.o cgenc_core.o cgenc_file.o cgenh.o cgenu.o \
hash.o lex.o parse.o schema.o
DEPS = $(OBJS:.o=.d)
RESULT = haris

CC = gcc
CCFLAGS = -Wall -pedantic -O3

all:	$(OBJS)
	$(CC) $(CCFLAGS) -o$(RESULT) $(OBJS)

%.d: %.c

-include $(DEPS)

clean:
	rm $(OBJS) *.d