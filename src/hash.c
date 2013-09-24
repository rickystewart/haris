#include "hash.h"

static int hash_string(char *);

static int add_builtins_to_hash(TypeHash *);

static TypeHashBucket *new_bucket(char *);
static void add_bucket_at_index(TypeHash *, TypeHashBucket *, int);

static int add_scalar_to_hash(TypeHash *, char *, ScalarTag);
static int add_text_to_hash(TypeHash *hash);

/* =============================PUBLIC INTERFACE============================= */

TypeHash *create_typehash(void)
{
  int i;
  TypeHash *ret = (TypeHash*)malloc(sizeof *ret);
  if (!ret) return NULL;
  for (i=0; i<HASH_SIZE; i++)
    ret->buckets[i] = NULL;
  if (add_builtins_to_hash(ret))
    return ret;
  else {
    destroy_typehash(ret);
    return NULL;
  }
}

void destroy_typehash(TypeHash *hash)
{
  int i;
  TypeHashBucket *bucket, *tmp;
  for (i=0; i<HASH_SIZE; i++) {
    bucket = hash->buckets[i];
    while (bucket) {
      tmp = bucket->next;
      free(bucket->name);
      free(bucket);
      bucket = tmp;
    }
  }
  free(hash);
  return;
}

int add_struct_to_hash(TypeHash *hash, char *name, ParsedStruct *strct)
{
  int i = hash_string(name);
  TypeHashBucket *bucket = new_bucket(name);
  if (!bucket) return 0;
  bucket->tu.tag = TYPE_STRUCT;
  bucket->tu.type.strct = strct;
  add_bucket_at_index(hash, bucket, i);
  return 1;
}

int add_enum_to_hash(TypeHash *hash, char *name, ParsedEnum *enm)
{
  int i = hash_string(name);
  TypeHashBucket *bucket = new_bucket(name);
  if (!bucket) return 0;
  bucket->tu.tag = TYPE_ENUM;
  bucket->tu.type.enm = enm;
  add_bucket_at_index(hash, bucket, i);
  return 1;
}

TypeHashBucket *get_type(TypeHash *hash, char *name)
{
  int i = hash_string(name);
  TypeHashBucket *bucket;
  for (bucket = hash->buckets[i]; bucket; bucket = bucket->next)
    if (!strcmp(name, bucket->name))
      return bucket;
  return NULL;
}

/* =============================STATIC FUNCTIONS============================= */

/* Bernstein hash function. This function was adapted from 
   http://www.cse.yorku.ca/~oz/hash.html .
   Please note that the hash function actually computes the index into the hash
   table rather than the hash proper. 
*/
static int hash_string(char *str)
{
  unsigned long hash = 5381;
  int c;

  while ((c = (unsigned char)(*str++)))
    hash = ((hash << 5) + hash) + c; 

  return hash % HASH_SIZE;
}

static int add_builtins_to_hash(TypeHash *hash)
{
  return add_scalar_to_hash(hash, "Uint8", SCALAR_UINT8) &&
    add_scalar_to_hash(hash, "Int8", SCALAR_INT8) &&
    add_scalar_to_hash(hash, "Uint16", SCALAR_UINT16) &&
    add_scalar_to_hash(hash, "Int16", SCALAR_INT16) &&
    add_scalar_to_hash(hash, "Uint32", SCALAR_UINT32) &&
    add_scalar_to_hash(hash, "Int32", SCALAR_INT32) &&
    add_scalar_to_hash(hash, "Uint64", SCALAR_UINT64) &&
    add_scalar_to_hash(hash, "Int64", SCALAR_INT64) &&
    add_scalar_to_hash(hash, "Bool", SCALAR_BOOL) &&
    add_scalar_to_hash(hash, "Float32", SCALAR_FLOAT32) &&
    add_scalar_to_hash(hash, "Float64", SCALAR_FLOAT64) &&
    add_text_to_hash(hash);
}

static TypeHashBucket *new_bucket(char *name)
{
  TypeHashBucket *bucket = (TypeHashBucket*)malloc(sizeof *bucket);
  if (!bucket) return NULL;
  bucket->name = strdup(name);
  if (!bucket->name) {
    free(bucket);
    return NULL;
  }
  return bucket;
}

static void add_bucket_at_index(TypeHash *hash, TypeHashBucket *bucket, int i)
{
  bucket->next = hash->buckets[i];
  hash->buckets[i] = bucket;
  return;
}

static int add_scalar_to_hash(TypeHash *hash, char *name, ScalarTag tag)
{
  int i = hash_string(name);
  TypeHashBucket *bucket = new_bucket(name);
  if (!bucket) return 0;
  bucket->tu.tag = TYPE_SCALAR_BUILTIN;
  bucket->tu.type.scalar_builtin = tag;
  add_bucket_at_index(hash, bucket, i);
  return 1;
}

static int add_text_to_hash(TypeHash *hash)
{
  char *name = "Text";
  TypeHashBucket *bucket;
  bucket = new_bucket(name);
  if (!bucket) return 0;
  bucket->tu.tag = TYPE_TEXT;
  add_bucket_at_index(hash, bucket, hash_string(name));
  return 1;
}

