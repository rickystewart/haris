#include "class.h"
#include "class_common.h"

#include <stdio.h>
#include <stddef.h>

int main(void)
{
  HarisStatus result;
  FILE *fin = fopen("class.msg", "rb");
  Class *class = Class_create();
  if ((result = Class_from_file(class, fin, NULL)) != HARIS_SUCCESS) {
    fprintf(stderr, "There was an error in deserialization from the file.\n");
    exit(1);
  }
  printf("AFTER DESERIALIZATION:\n\n");
  print_Class(class);
  Class_destroy(class);
  fclose(fin);
}
