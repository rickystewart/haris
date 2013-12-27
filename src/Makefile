OBJS = cgen.o cgenc.o cgenc_buffer.o cgenc_core.o cgenc_file.o cgenh.o cgenu.o \
hash.o lex.o parse.o schema.o
RESULT = haris

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Wconversion -Wsign-conversion -O3

all:	$(OBJS)
	$(CC) $(CFLAGS) -o$(RESULT) $(OBJS)

clean:
	rm $(OBJS) 