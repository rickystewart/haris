#include "class.h"
#include "class_common.h"

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* class_main.c: A file that constructs a "class" object (as described
   in the class.haris schema), serializes it to a byte array, reads it
   back from the byte array, and then re-serializes it to a file.

   The file class_read_back.c reads the class object back from the file.
*/

const char *students[] = { "John Smith", "Jane Smith", "Smith Jones",
                           "James Spader", "John Krasinski", "Steve Carell"
};

void print_haris_message(unsigned char *message, haris_uint32_t sz)
{
  haris_uint32_t i;
  printf("Haris message with length %llu:\n", (unsigned long long)sz);
  for (i = 0; i < sz; i ++) 
    printf("%x ", (int)message[i]);
  printf("\n\n");
}

int main(void)
{  
  unsigned char *haris_message = NULL;
  haris_uint32_t i, sz;
  HarisStatus result;

  /* ******* SERIALIZATION ******* */

  /* Allocate our in-memory class here; the generated API provides this
     function */
  Class *class = Class_create(); 
  FILE *fout = fopen("class.msg", "wb");
  class->graduation_year = 2018;
  /* Initialize our students array */
  Class_init_students(class, sizeof students / sizeof students[0]);
  for (i = 0; i < Class_len_students(class); i ++) {
    Student *student = Class_get_students(class) + i;
    student->id = i;
    /* Every student has 50k debt nowadays */
    student->debt_in_dollars = 50000 + i * 100;
    /* Allocate room for the `name` field of the student. */
    Student_init_name(student, strlen(students[i]) + 1);
    /* Copy the name into the string we just allocated */
    strcpy(Student_get_name(student), students[i]);
  }
  printf("BEFORE SERIALIZATION:\n");
  print_Class(class);
  /* Serialize! After this function is called, the `haris_message` pointer will
     point to the serialized binary message, and `sz` will be the length of
     the message in bytes. */
  if ((result = Class_to_buffer(class, &haris_message, &sz)) != HARIS_SUCCESS) {
    fprintf(stderr, "There was an error in serialization.\n");
    exit(1);
  }
  /* Print the message that was just encoded! */
  print_haris_message(haris_message, sz);
  /* Destroy the in-memory class we just created. We will reproduce it later. */
  Class_destroy(class);

  /* ******* DE-SERIALIZATION ******* */
  /* Create a new class (nothing to do with the old one, which is gone) */
  class = Class_create();
  /* Deserialize! This will populate the fields of the Class structure with
     the information we wrote to the buffer. */
  if ((result = Class_from_buffer(class, haris_message, sz, NULL)) 
      != HARIS_SUCCESS) {
    fprintf(stderr, "There was an error in deserialization.\n");
    exit(1);
  }
  printf("AFTER DESERIALIZATION:\n");
  print_Class(class);
  /* ******* SERIALIZATION TO FILE ******* */
  printf("\nThe class will now be written to the output file.\n\
Run class_read_back to test the file deserialization protocol.\n\n");
  if ((result = Class_to_file(class, fout, NULL)) != HARIS_SUCCESS) {
    fprintf(stderr, "There was an error in file serialization.\n");
    exit(1);
  }
  fclose(fout);
  Class_destroy(class);
  free(haris_message);
} 
