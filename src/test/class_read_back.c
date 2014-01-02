#include "class.h"

#include <stdio.h>
#include <stddef.h>

void print_Student(Student *student)
{
  printf("%s (ID %lu) has $%lu in debt.\n", 
         Student_get_name(student),
         (unsigned long)student->id,
         (unsigned long)student->debt_in_dollars);
}

void print_Class(Class *class)
{
  haris_uint32_t i;
  printf("===CLASS OF %u===\n", (unsigned)class->graduation_year);
  for (i = 0; i < Class_len_students(class); i ++)
    print_Student(Class_get_students(class) + i);
  printf("\n");
}

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
