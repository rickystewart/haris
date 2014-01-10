OBJS = cgen.o cgenc.o cgenc_buffer.o cgenc_core.o cgenc_file.o cgenc_util.o \
cgenc_fd.o cgenh.o hash.o lex.o parse.o schema.o main.o
RESULT = haris

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -Wconversion -Wsign-conversion -O3 -std=c99

all:	$(OBJS)
	$(CC) $(CFLAGS) -o$(RESULT) $(OBJS)

%.o:	%.h %.c

clean:
	rm $(OBJS) $(RESULT)
