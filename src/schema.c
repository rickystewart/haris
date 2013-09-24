#include "schema.h"

static int field_length(ScalarTag);

static int realloc_struct_scalars(ParsedStruct *);
static int realloc_struct_children(ParsedStruct *);

static int add_list_of_scalars_or_enums_field(ParsedStruct *, char *, int,
                                              ScalarTag, ParsedEnum *);

/* =============================PUBLIC INTERFACE============================= */

ParsedSchema *create_parsed_schema(void)
{
  const int arr_size = 8;
  ParsedSchema *ret = (ParsedSchema*)malloc(sizeof *ret);
  if (!ret) return NULL;
  ret->structs = (ParsedStruct*)malloc(arr_size * sizeof *ret->structs);
  ret->enums = (ParsedEnum*)malloc(arr_size * sizeof *ret->enums);
  if (!ret->structs || !ret->enums) {
    free(ret->structs);
    free(ret->enums);
    free(ret);
    return NULL;
  }
  ret->num_structs = ret->num_enums = 0;
  ret->structs_alloc = ret->enums_alloc = arr_size;
  return ret;
}

void destroy_parsed_schema(ParsedSchema *p)
{
  int i, j;
  for (i=0; i<p->num_structs; i++) {
    for (j=0; j<p->structs[i].num_scalars; j++)
      free(p->structs[i].scalars[j].name);
    free(p->structs[i].scalars);
    for (j=0; j<p->structs[i].num_children; j++)
      free(p->structs[i].children[j].name);
    free(p->structs[i].children);
  }
  for (i=0; i<p->num_enums; i++) {
    for (j=0; j<p->enums[i].num_values; j++)
      free(p->enums[i].values[j]);
    free(p->enums[i].values);
  }
  free(p->structs);
  free(p->enums);
  free(p);
  return;
}

/* Creates a new structure in the given schema with the given name, returning
   a pointer to it. Notably, this function DOES NOT perform error-checking, so
   if you do not perform error-checking yourself, you may end up with
   duplicate structures. */
ParsedStruct *new_struct(ParsedSchema *schema, char *name)
{
  const int arr_size = 8;
  ParsedStruct *ret, *array;
  if (schema->num_structs == schema->structs_alloc) {
    array = realloc(schema->structs, 
                    (schema->structs_alloc *= 2) * sizeof *array);
    if (!array) return NULL;
    schema->structs = array;
  } 
  ret = schema->structs + schema->num_structs;
  ret->name = strdup(name);
  ret->num_scalars = ret->num_children = 0;
  ret->offset = 0;
  ret->scalars_alloc = ret->children_alloc = arr_size;
  ret->scalars = malloc(arr_size * sizeof *ret->scalars);
  ret->children = malloc(arr_size * sizeof *ret->children);
  if (!ret->scalars || !ret->children) {
    free(ret->scalars);
    free(ret->children);
    return NULL;
  }
  schema->num_structs++;
  return ret;
}

/* As above, this function does not perform error-checking over the name
   of the enumeration. */
ParsedEnum *new_enum(ParsedSchema *schema, char *name)
{
  const int arr_size = 8;
  ParsedEnum *ret, *array;
  if (schema->num_enums == schema->enums_alloc) {
    array = realloc(schema->enums,
                    (schema->enums_alloc *= 2) * sizeof *array);
    if (!array) return NULL;
    schema->enums = array;
  }
  ret = schema->enums + schema->num_enums;
  ret->name = strdup(name);
  ret->num_values = 0;
  ret->values_alloc = arr_size;
  ret->values = malloc(arr_size * sizeof *ret->values);
  if (!ret->values) return NULL;
  schema->num_enums++;
  return ret;
}

/* Tests whether the given name is already the name of a field in the given
   structure. */
int struct_name_collide(ParsedStruct *strct, char *name)
{
  int i;
  for (i=0; i<strct->num_scalars; i++)
    if (!strcmp(strct->scalars[i].name, name))
      return 1;
  for (i=0; i<strct->num_children; i++)
    if (!strcmp(strct->children[i].name, name))
      return 1;
  return 0;
}

/* Tests whether the given name is already the name of a value in the given
   enumeration. */
int enum_name_collide(ParsedEnum *enm, char *name)
{
  int i;
  for (i=0; i<enm->num_values; i++)
    if (!strcmp(enm->values[i], name))
      return 1;
  return 0;
}

/* Adds an enumerated field to the given structure. As above, performs no
   error-checking, so fields might name-collide. */
int add_enum_field(ParsedStruct *strct, char *name, ParsedEnum *field)
{
  int i = strct->num_scalars;
  if (strct->num_scalars == strct->scalars_alloc) 
    if (!realloc_struct_scalars(strct)) return 0;
  strct->scalars[i].name = strdup(name);
  if (!strct->scalars[i].name) return 0;
  strct->scalars[i].offset = strct->offset;
  strct->scalars[i].type.tag = SCALAR_ENUM;
  strct->scalars[i].type.enum_type = field;
  strct->offset += 1;
  strct->num_scalars++;
  return 1;
}

/* Adds a scalar field (NOT an enumeration) to the given structure. Fields
   may name-collide. */
int add_scalar_field(ParsedStruct *strct, char *name, ScalarTag field)
{
  int i = strct->num_scalars;
  if (field == SCALAR_ENUM) return 0; /* This function is not for adding
                                         enumerated fields */
  if (strct->num_scalars == strct->scalars_alloc) 
    if (!realloc_struct_scalars(strct)) return 0;
  strct->scalars[i].name = strdup(name);
  if (!strct->scalars[i].name) return 0;
  strct->scalars[i].offset = strct->offset;
  strct->scalars[i].type.tag = field;
  strct->scalars[i].type.enum_type = NULL;
  strct->offset += field_length(field);
  strct->num_scalars++;
  return 1;
}

/* Adds a structure field to the given structure. Fields may name-collide. */
int add_struct_field(ParsedStruct *strct, char *name, int nullable, 
                     ParsedStruct *field)
{
  int i = strct->num_children;
  if (strct->num_children == strct->children_alloc)
    if (!realloc_struct_children(strct)) return 0;
  strct->children[i].name = strdup(name);
  if (!strct->children[i].name) return 0;
  strct->children[i].nullable = nullable;
  strct->children[i].tag = CHILD_STRUCT;
  strct->children[i].type.strct = field;
  strct->num_children++;
  return 1;
}

/* Adds a text field to the given structure. Fields may name-collide. */
int add_text_field(ParsedStruct *strct, char *name, int nullable)
{
  int i = strct->num_children;
  if (strct->num_children == strct->children_alloc)
    if (!realloc_struct_children(strct)) return 0;
  strct->children[i].name = strdup(name);
  if (!strct->children[i].name) return 0;
  strct->children[i].nullable = nullable;
  strct->children[i].tag = CHILD_TEXT;
  strct->num_children++;
  return 1;
}

/* Adds a list of enumerated vales field to the given structure. Fields
   may name-collide. */
int add_list_of_enums_field(ParsedStruct *strct, char *name, int nullable, 
                            ParsedEnum *field)
{
  return add_list_of_scalars_or_enums_field(strct, name, nullable, 
                                            SCALAR_ENUM, field);
}

/* Adds a list of scalars (NOT enumerated values) field to the given 
   structure. Fields may name-collide. */
int add_list_of_scalars_field(ParsedStruct *strct, char *name, int nullable,
                              ScalarTag tag)
{
  if (tag == SCALAR_ENUM) return 0; /* This function is NOT for enumerations */
  return add_list_of_scalars_or_enums_field(strct, name, nullable, tag, NULL);
}

/* Adds a list of structures field to the given structure. Fields may
   name-collide. */
int add_list_of_structs_field(ParsedStruct *strct, char *name, int nullable,
                              ParsedStruct *field)
{
  int i = strct->num_children;
  if (strct->num_children == strct->children_alloc)
    if (!realloc_struct_children(strct)) return 0;
  strct->children[i].name = strdup(name);
  if (!strct->children[i].name) return 0;
  strct->children[i].nullable = nullable;
  strct->children[i].tag = CHILD_STRUCT_LIST;
  strct->children[i].type.struct_list = field;
  strct->num_children++;
  return 1;
}

/* Adds an enumerated value to the given enumeration. Performs no error-
   checking, so the names of values may collide. */
int add_enumerated_value(ParsedEnum *enm, char *name)
{
  char **array;
  int i = enm->num_values;
  if (enm->num_values == enm->values_alloc) {
    array = realloc(enm->values,
                    (enm->values_alloc *= 2) * sizeof *array);
    if (!array) return 0;
    enm->values = array;
  }
  enm->values[i] = strdup(name);
  if (!enm->values[i]) return 0;
  enm->num_values++;
  return 1;
}

/* =============================STATIC FUNCTIONS============================= */

/* Generates the length in bytes of a field with the given type. */
static int field_length(ScalarTag field)
{
  switch (field) {
  case SCALAR_UINT8:
  case SCALAR_INT8:
  case SCALAR_BOOL:
  case SCALAR_ENUM:
    return 1;
  case SCALAR_UINT16:
  case SCALAR_INT16:
    return 2;
  case SCALAR_UINT32:
  case SCALAR_INT32:
  case SCALAR_FLOAT32:
    return 4;
  case SCALAR_UINT64:
  case SCALAR_INT64:
  case SCALAR_FLOAT64:
    return 8;
  }
  return -1;
}

static int realloc_struct_scalars(ParsedStruct *strct)
{
  ScalarField *array = realloc(strct->scalars,
                               (strct->scalars_alloc *= 2) * sizeof *array);
  if (!array) return 0;
  strct->scalars = array;
  return 1;
}

static int realloc_struct_children(ParsedStruct *strct)
{
  ChildField *array = realloc(strct->children,
                              (strct->children_alloc *= 2) * sizeof *array);
  if (!array) return 0;
  strct->children = array;
  return 1;
}

static int add_list_of_scalars_or_enums_field(ParsedStruct *strct, char *name, 
                                              int nullable, ScalarTag tag, 
                                              ParsedEnum *enm)
{
  int i = strct->num_children;
  if (strct->num_children == strct->children_alloc)
    if (!realloc_struct_children(strct)) return 0;
  strct->children[i].name = strdup(name);
  if (!strct->children[i].name) return 0;
  strct->children[i].nullable = nullable;
  strct->children[i].tag = CHILD_SCALAR_LIST;
  strct->children[i].type.scalar_list.tag = tag;
  strct->children[i].type.scalar_list.enum_type = enm;
  strct->num_children++;
  return 1;
}
