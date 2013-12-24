#ifndef __SCHEMA_H
#define __SCHEMA_H

#include <stdlib.h>
#include <string.h>

/* schema.h contains data definitions for the in-memory representation of
   the given user schema. The parser consumes tokens from the input file
   and constructs the schema piecemeal. When the parse has completed, a
   schema structure is the result; the schema has then been completely analyzed
   and can at that point be examined manually to generate source code in the
   target language. 

   The schema library is very simple and does no nonessential error-checking
   on its own. In particular, name collisions are generally not tested for,
   so you can quickly end up with an invalid schema if you do not manually
   check for name collisions with the struct_name_collide and enum_name_collide
   functions. The parser library uses a hash table of types to do this.

   If any schema library functions fail, it is generally because of a memory
   allocation error.
*/

typedef enum {
  SCALAR_UINT8, SCALAR_INT8, SCALAR_UINT16, SCALAR_INT16, SCALAR_UINT32, 
  SCALAR_INT32, SCALAR_UINT64, SCALAR_INT64, SCALAR_FLOAT32, SCALAR_FLOAT64,
  SCALAR_BOOL, SCALAR_ENUM
} ScalarTag;

typedef enum {
  CHILD_TEXT, CHILD_STRUCT, CHILD_SCALAR_LIST, CHILD_STRUCT_LIST
} ChildTag;

typedef struct _ParsedEnum ParsedEnum;
typedef struct _ParsedStruct ParsedStruct;

typedef struct {
  ScalarTag tag;
  ParsedEnum *enum_type; /* NULL if the scalar is not an enumeration */
} ScalarType;

typedef struct {
  char *name;
  int offset;
  ScalarType type;
} ScalarField;

typedef union {
  ParsedStruct *strct;
  ScalarType scalar_list;
  ParsedStruct *struct_list;
  /* No further information is needed if the child is a Text object */
} ChildType;

typedef struct {
  char *name;
  int nullable;
  ChildTag tag;
  ChildType type;
} ChildField;

struct _ParsedStruct {
  char *name;
  int schema_index;
  int offset;
  int num_scalars;
  int scalars_alloc;
  ScalarField *scalars;
  int num_children;
  int children_alloc;
  ChildField *children;
};

struct _ParsedEnum {
  char *name;
  int num_values;
  int values_alloc;
  char **values;
};

typedef struct {
  int num_structs;
  int structs_alloc;
  ParsedStruct *structs;
  int num_enums;
  int enums_alloc;
  ParsedEnum *enums;
} ParsedSchema;

ParsedSchema *create_parsed_schema(void);
void destroy_parsed_schema(ParsedSchema *);

ParsedStruct *new_struct(ParsedSchema *, char *);
ParsedEnum *new_enum(ParsedSchema *, char *);

int struct_name_collide(ParsedStruct *, char *);
int enum_name_collide(ParsedEnum *, char *);

int add_enum_field(ParsedStruct *, char *, ParsedEnum *);
int add_scalar_field(ParsedStruct *, char *, ScalarTag);
int add_struct_field(ParsedStruct *, char *, int, ParsedStruct *);
int add_text_field(ParsedStruct *, char *, int);
int add_list_of_enums_field(ParsedStruct *, char *, int, ParsedEnum *);
int add_list_of_scalars_field(ParsedStruct *, char *, int, ScalarTag);
int add_list_of_structs_field(ParsedStruct *, char *, int, ParsedStruct *);

int add_enumerated_value(ParsedEnum *, char *);

#endif
