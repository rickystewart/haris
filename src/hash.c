#include "hash.h"

static int hash_string(char *);

static int add_builtins_to_hash(TypeHash *);

static TypeHashBucket *new_bucket(char *);
static void add_bucket_at_index(TypeHash *, TypeHashBucket *, int);

static int add_scalar_to_hash(TypeHash *, char *, ScalarTag);
static int add_text_to_hash(TypeHash *hash);

/* =============================PUBLIC INTERFACE============================= */

/* Create a new typehash, and return a pointer to it, or NULL if the allocation
   fails. The typehash will come pre-loaded with all of the builtin types.
*/
TypeHash *create_typehash(void)
{
  int i;
  TypeHash *ret = (TypeHash*)malloc(sizeof *ret);
  if (!ret) return NULL;
  for (i = 0; i<HASH_SIZE; i++)
    ret->buckets[i] = NULL;
  if (add_builtins_to_hash(ret))
    return ret;
  else {
    destroy_typehash(ret);
    return NULL;
  }
}

/* Destroy a typehash and all of its buckets. */
void destroy_typehash(TypeHash *hash)
{
  int i;
  TypeHashBucket *bucket, *tmp;
  for (i = 0; i<HASH_SIZE; i++) {
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

/* Add a structure with the given name to the typehash. */
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

/* Add an enumerator with the given name to the typehash. */
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

/* Retrieve a type with the given name from the given hash, or NULL
   if the type does not exist. 
*/
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

/* Add all builtin types to the given hash statefully. */
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

/* Create a new bucket with the given name, which must be dynamically
   allocated. It is then suitable to be added to a TypeHash.
*/
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

/* Add the given bucket to the given hash at the given index into the buckets
   array.
*/
static void add_bucket_at_index(TypeHash *hash, TypeHashBucket *bucket, int i)
{
  bucket->next = hash->buckets[i];
  hash->buckets[i] = bucket;
  return;
}

/* Add a scalar type to the TypeHash. Utility function for use by 
   add_builtins_to_hash.
*/
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

/* Add the builtin text type to the TypeHash. Utility function for
   use by add_builtins_to_hash.
*/
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

