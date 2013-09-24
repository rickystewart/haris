#ifndef __HASH_H
#define __HASH_H

#include <stdlib.h>
#include <string.h>
#include "schema.h"

/* The hash library contains a high-level structure for mapping type names
   to in-memory type representations. In particular, the parser uses
   this structure to fetch the structure or enumeration information 
   that belongs to a particular type name in constant time. */

#define HASH_SIZE 43

typedef enum {
  TYPE_SCALAR_BUILTIN, TYPE_ENUM, TYPE_STRUCT, TYPE_TEXT
} TypeTag;

typedef struct TypeHashBucket TypeHashBucket;

typedef union {
  ScalarTag scalar_builtin;
  ParsedEnum *enm;
  ParsedStruct *strct;
} TypeUnion;

typedef struct {
  TypeTag tag;
  TypeUnion type;
} TaggedTypeUnion;

struct TypeHashBucket {
  char *name;
  TaggedTypeUnion tu;
  TypeHashBucket *next;
};

typedef struct {
  TypeHashBucket *buckets[HASH_SIZE];
} TypeHash;

TypeHash *create_typehash(void);
void destroy_typehash(TypeHash *);

int add_struct_to_hash(TypeHash *, char *, ParsedStruct *);
int add_enum_to_hash(TypeHash *, char *, ParsedEnum *);

TypeHashBucket *get_type(TypeHash *, char *);

#endif
