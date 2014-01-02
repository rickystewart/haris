all:	class class_read_back

class:	class_main.c
	gcc -Wall -Wextra -pedantic -std=c99 class.c class_main.c -o class

class_read_back:	class_read_back.c
	gcc -Wall -Wextra -pedantic -std=c99 class.c class_read_back.c -o class_read_back

clean:
	rm class class_read_back