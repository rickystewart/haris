#include "schema.h"

static int field_length(ScalarTag);

static int realloc_struct_scalars(ParsedStruct *);
static int realloc_struct_children(ParsedStruct *);

static int compute_struct_inmem_size(ParsedStruct *);
static void compute_max_sizes(ParsedSchema *);
static void compute_embeddability(ParsedSchema *);
static int reachable_from_embedded_children(ParsedStruct *find, 
                                            ParsedStruct *root);

static int add_list_of_scalars_or_enums_field(ParsedStruct *, char *, int,
                                              ScalarTag, ParsedEnum *);

/* =============================PUBLIC INTERFACE============================= */

/* Dynamically allocate a parsed schema, returning a pointer to it. */
ParsedSchema *create_parsed_schema(void)
{
  const size_t arr_size = 8;
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
  ret->structs_alloc = ret->enums_alloc = (int)arr_size;
  return ret;
}

/* Destroy a parsed schema and all of its data. */
void destroy_parsed_schema(ParsedSchema *schema)
{
  int i, j;
  for (i = 0; i < schema->num_structs; i ++) {
    for (j = 0; j < schema->structs[i].num_scalars; j ++)
      free(schema->structs[i].scalars[j].name);
    free(schema->structs[i].scalars);
    for (j = 0; j < schema->structs[i].num_children; j ++)
      free(schema->structs[i].children[j].name);
    free(schema->structs[i].children);
  }
  for (i = 0; i < schema->num_enums; i ++) {
    for (j = 0; j < schema->enums[i].num_values; j ++)
      free(schema->enums[i].values[j]);
    free(schema->enums[i].values);
  }
  free(schema->structs);
  free(schema->enums);
  free(schema);
  return;
}

/* Finalize the given schema -- this entails precomputing the sizes of
   all structures, wherever that computation is possible. */
void finalize_schema(ParsedSchema *schema)
{
  compute_max_sizes(schema);
  compute_embeddability(schema);
}

/* Creates a new structure in the given schema with the given name, returning
   a pointer to it. Notably, this function DOES NOT perform error-checking, so
   if you do not perform error-checking yourself, you may end up with
   duplicate structure names. */
ParsedStruct *new_struct(ParsedSchema *schema, char *name)
{
  const size_t arr_size = 8;
  ParsedStruct *ret, *array;
  if (schema->num_structs == schema->structs_alloc) {
    array = realloc(schema->structs, 
                    (size_t)(schema->structs_alloc *= 2) 
                     * sizeof *array);
    if (!array) return NULL;
    schema->structs = array;
  } 
  ret = schema->structs + schema->num_structs;
  ret->schema_index = schema->num_structs;
  ret->name = util_strdup(name);
  ret->num_scalars = ret->num_children = 0;
  ret->offset = 0;
  ret->meta.max_size = 0;
  ret->scalars_alloc = ret->children_alloc = (int)arr_size;
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
  const size_t arr_size = 8;
  ParsedEnum *ret, *array;
  if (schema->num_enums == schema->enums_alloc) {
    array = realloc(schema->enums,
                    (size_t)(schema->enums_alloc *= 2) * sizeof *array);
    if (!array) return NULL;
    schema->enums = array;
  }
  ret = schema->enums + schema->num_enums;
  ret->name = util_strdup(name);
  ret->num_values = 0;
  ret->values_alloc = (int)arr_size;
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
  for (i = 0; i<strct->num_scalars; i++)
    if (!strcmp(strct->scalars[i].name, name))
      return 1;
  for (i = 0; i<strct->num_children; i++)
    if (!strcmp(strct->children[i].name, name))
      return 1;
  return 0;
}

/* Tests whether the given name is already the name of a value in the given
   enumeration. */
int enum_name_collide(ParsedEnum *enm, char *name)
{
  int i;
  for (i = 0; i<enm->num_values; i++)
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
  strct->scalars[i].name = util_strdup(name);
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
  strct->scalars[i].name = util_strdup(name);
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
  strct->children[i].name = util_strdup(name);
  if (!strct->children[i].name) return 0;
  strct->children[i].nullable = nullable;
  strct->children[i].tag = CHILD_STRUCT;
  strct->children[i].type.strct = field;
  strct->children[i].meta.embeddable = 1;
  strct->num_children++;
  return 1;
}

/* Adds a text field to the given structure. Fields may name-collide. */
int add_text_field(ParsedStruct *strct, char *name, int nullable)
{
  int i = strct->num_children;
  if (strct->num_children == strct->children_alloc)
    if (!realloc_struct_children(strct)) return 0;
  strct->children[i].name = util_strdup(name);
  if (!strct->children[i].name) return 0;
  strct->children[i].nullable = nullable;
  strct->children[i].tag = CHILD_TEXT;
  strct->children[i].meta.embeddable = 0;
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
  strct->children[i].name = util_strdup(name);
  if (!strct->children[i].name) return 0;
  strct->children[i].nullable = nullable;
  strct->children[i].tag = CHILD_STRUCT_LIST;
  strct->children[i].type.struct_list = field;
  strct->children[i].meta.embeddable = 0;
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
                    (size_t)(enm->values_alloc *= 2) * sizeof *array);
    if (!array) return 0;
    enm->values = array;
  }
  enm->values[i] = util_strdup(name);
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

/* Doubles the size of the scalar array of the given structure. */
static int realloc_struct_scalars(ParsedStruct *strct)
{
  ScalarField *array = realloc(strct->scalars,
                               (size_t)(strct->scalars_alloc *= 2) 
                                * sizeof *array);
  if (!array) return 0;
  strct->scalars = array;
  return 1;
}

/* Doubles the size of the child array of the given structure. */
static int realloc_struct_children(ParsedStruct *strct)
{
  ChildField *array = realloc(strct->children,
                              (size_t)(strct->children_alloc *= 2) 
                               * sizeof *array);
  if (!array) return 0;
  strct->children = array;
  return 1;
}

static void compute_max_sizes(ParsedSchema *schema)
{
  int i, changed;
  ParsedStruct *strct;
  for (;;) {
    changed = 0;
    for (i = 0; i < schema->num_structs; i ++) {
      strct = &schema->structs[i];
      if (strct->meta.max_size == 0) {
        if (compute_struct_inmem_size(strct)) {
          changed = 1;
        }
      }
    }
    if (!changed) break;
  }
}

/* All structure children are initialized by default to be marked embeddable;
   this function discerns which children are not actually embeddable, and
   edits the children to reflect that.

   A child is not actually embeddable if you can reach the parent structure
   by following the child through the graph of the child's embedded children.
   For example, in the following schema:
   @struct A
   @struct B
   struct A ( B? b )
   struct B ( A? a )
   ... we start at structure A, which has a single child (B). Since we can
   find A in the graph of B's embedded children (B.a has type A) we edit
   A.b to be unembedded. Then, proceeding to structure B, we don't find B
   in A's embedded children (since we marked A.b unembedded earlier) so
   it remains unembedded. */
static void compute_embeddability(ParsedSchema *schema)
{
  int i, j;
  ChildField *child;
  ParsedStruct *strct, *child_struct;
  for (i = 0; i < schema->num_structs; i ++) {
    strct = &schema->structs[i];
    for (j = 0; j < strct->num_children; j ++) {
      child = &strct->children[j];
      if (child->tag != CHILD_STRUCT || !child->meta.embeddable)
        continue;
      child_struct = child->type.strct;
      if (reachable_from_embedded_children(strct, child_struct))
        child->meta.embeddable = 0;
    }
  }
}

/* Tests whether find == root, or whether find can be found within the graph
   of root's embedded children. */
static int reachable_from_embedded_children(ParsedStruct *find, 
                                            ParsedStruct *root)
{
  int i;
  ChildField *child;
  ParsedStruct *child_struct;
  if (find == root) return 1;
  for (i = 0; i < root->num_children; i ++) {
    child = &root->children[i];
    if (child->tag != CHILD_STRUCT || !child->meta.embeddable)
      continue;
    child_struct = child->type.strct;
    if (reachable_from_embedded_children(find, child_struct))
      return 1;
  }
  return 0;
}

/* Compute the maximum encoded size of the given structure, if possible.
   A structure has a maximum encoded size IF 1) it has no children or 
   2) all of its children are structures that have a maximum encoded size.
   This is a helper function that finalize_schema leverages.

   This function returns 1 if the structure's inmem_size field was changed,
   and 0 otherwise. */
static int compute_struct_inmem_size(ParsedStruct *strct)
{
  size_t sz;
  int i, can_compute;
  StructMetadata *meta = &strct->meta;
  if (meta->max_size != 0) {
    return 0;
  } else if (strct->num_children == 0) {
    meta->max_size = (size_t)strct->offset + 2U;
    return 1;
  } else {
    sz = 0;
    can_compute = 1;
    for (i = 0; i < strct->num_children; i ++) {
      if (strct->children[i].tag != CHILD_STRUCT ||
          strct->children[i].type.strct->meta.max_size == 0) {
        can_compute = 0;
        break;
      } else {
        sz += strct->children[i].type.strct->meta.max_size;
      } 
    }
    if (can_compute) {
      meta->max_size = sz + (size_t)strct->offset + 2U;
      return 1;
    } else {
      return 0;
    }
  }
}

static int add_list_of_scalars_or_enums_field(ParsedStruct *strct, char *name, 
                                              int nullable, ScalarTag tag, 
                                              ParsedEnum *enm)
{
  int i = strct->num_children;
  if (strct->num_children == strct->children_alloc)
    if (!realloc_struct_children(strct)) return 0;
  strct->children[i].name = util_strdup(name);
  if (!strct->children[i].name) return 0;
  strct->children[i].nullable = nullable;
  strct->children[i].tag = CHILD_SCALAR_LIST;
  strct->children[i].type.scalar_list.tag = tag;
  strct->children[i].type.scalar_list.enum_type = enm;
  strct->num_children++;
  return 1;
}
