#ifndef __UTIL_H
#define __UTIL_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>

/* Implements various helper functions (usually the stuff that I had thought
   mistakenly was included in the C standard, and then found out too late I
   had to implement myself).
*/

int util_asprintf(char **, const char *, ...);
int util_vasprintf(char **, const char *, va_list);

char *util_strdup(const char *s);

#endif
