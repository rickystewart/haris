/* Common utility functions for testing files */

#include "class_common.h"

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
