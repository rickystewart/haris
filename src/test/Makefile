CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g

all:	class class_read_back

class:	class_main.c
	$(CC) $(CFLAGS) class.c class_main.c class_common.c -o class

class_read_back:	class_read_back.c
	$(CC) $(CFLAGS) class.c class_read_back.c class_common.c -o class_read_back

clean:
	rm class class_read_back class.msg