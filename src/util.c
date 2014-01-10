#include "util.h"

/* Implementation of vasprintf from GNU/BSD */
int util_vasprintf(char **out, const char *fmt, va_list args)
{
  char *ret;
  int alloc_size;
  va_list args_copy;
  va_copy(args_copy, args);
  alloc_size = vsnprintf((char*)NULL, 0, fmt, args);
  if (alloc_size >= 0) { /* alloc_size contains the size of the string to 
  						    be allocated since vsnprintf always returns
  						    the number of bytes that would be printed 
  						    in case of unlimited buffer space */
    ret = (char*)malloc((size_t)alloc_size + 1);
    if (ret) {
      if (vsnprintf(ret, (size_t)alloc_size + 1, fmt, args_copy) < 0) {
      	free(ret);
      	alloc_size = -1;
      	ret = NULL;
      }
    }
    *out = ret;
  }
  va_end(args_copy);
  return alloc_size;
}

/* Implementation of asprintf from GNU/BSD; leverages vasprintf */
int util_asprintf(char **out, const char *fmt, ...)
{
  int alloc_size;
  va_list ap;
  va_start(ap, fmt);
  alloc_size = util_vasprintf(out, fmt, ap);
  va_end(ap);
  return alloc_size;
}

char *util_strdup(const char *s)
{
  char *ret = (char*)malloc(strlen(s) + 1);
  if (!ret) return ret;
  strcpy(ret, s);
  return ret;
}
