#include "cgenc_buffer.h"

static CJobStatus write_public_buffer_funcs(CJob *, ParsedStruct *);
static CJobStatus write_static_buffer_funcs(CJob *, ParsedStruct *);
static CJobStatus write_child_buffer_handler(CJob *);

static CJobStatus write_static_from_buffer_function(CJob *, ParsedStruct *);
static CJobStatus write_static_to_buffer_function(CJob *, ParsedStruct *);

/* =============================PUBLIC INTERFACE============================= */

/* The following static buffer functions exist for every structure S:
   static HarisStatus _S_from_buffer(S *, unsigned char *, haris_uint32_t ind,
                                     haris_uint32_t sz,
                                     unsigned char **, int depth);
   static HarisStatus _S_from_buffer_posthead(S *, unsigned char *,
                                              haris_uint32_t, unsigned char **,
                                              int depth, int body_size, 
                                              int num_children)
   static unsigned char *_S_to_buffer(S *, unsigned char **);
   static unsigned char *_S_to_buffer_posthead(S *, unsigned char **)

   Finally, we have the following recursive function:
   static unsigned char *handle_child_buffer(unsigned char *, haris_uint32_t, 
                                             haris_uint32_t, haris_uint32_t *,
                                             int);
   static unsigned char *handle_child_buffer_struct_posthead(unsigned char *, 
                                             haris_uint32_t, haris_uint32_t, 
                                             haris_uint32_t *, int, 
                                             int body_size, int num_children);                                        
*/

CJobStatus write_buffer_protocol_funcs(CJob *job)
{
  CJobStatus result;
  int i;
  ParsedSchema *schema = job->schema;
  for (i = 0; i < schema->num_structs; i++) {
    if ((result = write_public_buffer_funcs(job, &schema->structs[i])) 
        != CJOB_SUCCESS) return result;
    if ((result = write_static_buffer_funcs(job, &schema->structs[i]))
        != CJOB_SUCCESS) return result;
  }
  if ((result = write_child_buffer_handler(job)) != CJOB_SUCCESS)
    return result;
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

/* There are two handler functions:
   - static HarisStatus handle_child_buffer(unsigned char * buf,
haris_uint32_t ind, haris_uint32_t sz, haris_uint32_t *out_ind, int depth);
   - static HarisStatus handle_child_buffer_struct_posthead(unsigned char *buf,
haris_uint32_t ind, haris_uint32_t sz, haris_uint32_t *out_ind, int depth,
int num_children, int body_size);
   These functions skip over children of structures that don't exist within
   the schema and are thus impossible to parse. For example, if we encounter
   a structure with 5 children, but we are only aware of 3 children, then we
   will call these functions twice to skip to the next part of the message.
*/
static CJobStatus write_child_buffer_handler(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, "static HarisStatus handle_child_buffer(unsigned char *buf,\
 haris_uint32_t ind, haris_uint32_t sz, haris_uint32_t *out_ind, int depth)\n\
{\n\
  HarisStatus result;\n\
  int num_children, body_size, el_size;\n\
  haris_uint32_t i;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  HARIS_ASSERT(ind < sz, INPUT);\n\
  HARIS_ASSERT(ind < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  if (buf[ind] == 0x80 || buf[ind] == 0x0) { *out_ind = ind + 1; \
return HARIS_SUCCESS; }\n\
  HARIS_ASSERT((ind + 1 < sz) && (ind + 1 < HARIS_MESSAGE_SIZE_LIMIT),\
               INPUT);\n\
  if ((buf[ind] & 0xC0) == 0x40) {\n\
    num_children = buf[ind] & 0x7F;\n\
    body_size = buf[ind + 1];\n\
    return handle_child_buffer_struct_posthead(buf, ind + 2, sz, out_ind, \
depth, num_children, body_size);\n\
  } else if ((buf[ind] & 0xE0) == 0xC0) {\n\
    HARIS_ASSERT((ind + 3 < sz) && (ind + 3 < HARIS_MESSAGE_SIZE_LIMIT),\
 INPUT);\n\
    i = haris_read_uint24(buf + ind + 1);\n\
    el_size = buf[ind] & 0x3;\n\
    *out_ind = ind + i * el_size + 4;\n\
    return HARIS_SUCCESS;\n\
  } else {\n\
    HARIS_ASSERT((ind + 5 < sz) && (ind + 5 < HARIS_MESSAGE_SIZE_LIMIT),\
 INPUT);\n\
    haris_uint32_t x;\n\
    i = haris_read_uint24(buf + ind + 1);\n\
    num_children = buf[ind + 4] & 0x7F;\n\
    body_size = buf[ind + 5];\n\
    *out_ind = ind + 6;\n\
    for (x = 0; x < i; x++)\n\
      if ((result = handle_child_buffer_struct_posthead(buf, *out_ind, sz, \n\
           out_ind, depth + 1, num_children, body_size)) != HARIS_SUCCESS)\n\
        return result;\n\
    return HARIS_SUCCESS;\n\
  }\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, "static HarisStatus handle_child_buffer_struct_posthead(\
unsigned char *buf,\nharis_uint32_t ind, haris_uint32_t sz, \
haris_uint32_t *out_ind, int depth, int num_children, int body_size)\n\
{\n\
  HarisStatus result;\n\
  haris_uint32_t i;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  HARIS_ASSERT(ind + body_size < sz, INPUT);\n\
  HARIS_ASSERT(ind + body_size < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  *out_ind = ind + body_size;\n\
  for (i = 0; i < num_children; i++)\n\
    if ((result = handle_child_buffer(buf, *out_ind, sz, out_ind, \n\
         depth + 1)) != HARIS_SUCCESS)\n\
      return result;\n\
  return HARIS_SUCCESS;\n}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_static_buffer_funcs(CJob *job, ParsedStruct *strct)
{
  CJobStatus result;
  if ((result = write_static_from_buffer_function(job, strct, out))
      != CJOB_SUCCESS) return result;
  if ((result = write_static_to_buffer_function(job, strct, out))
      != CJOB_SUCCESS) return result;
  return CJOB_SUCCESS;
}

static CJobStatus write_static_from_buffer_function(CJob *job, 
                                                    ParsedStruct *strct)
{
  int i, scalar_sz;
  const char *prefix = job->prefix, *name = strct->name, *child_name;
  CJOB_FMT_PRIV_FUNCTION(job, "static HarisStatus _%s%s_from_buffer(%s%s *strct, \
unsigned char *buf, haris_uint32_t ind, haris_uint32_t sz, haris_uint32_t *\
out_ind, int depth)\n{\n\
  HarisStatus result;\n\
  int num_children, body_size;\n\
  haris_uint32_t i;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  HARIS_ASSERT(ind < sz, INPUT);\n\
  HARIS_ASSERT(ind < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  HARIS_ASSERT(!(buf[ind] & 0x80), STRUCTURE);\n\
  if (buf[ind] & 0x40) { strct->_null = 1; *out_ind = ind + 1; \
return HARIS_SUCCESS; }\n\
  HARIS_ASSERT((ind + 1 < sz) && (ind + 1 < HARIS_MESSAGE_SIZE_LIMIT),\n\
               INPUT);\n\
  body_size = buf[ind + 1];\n\
  num_children = buf[ind] & 0x7F;\n\
  HARIS_ASSERT(body_size >= %s%s_LIB_BODY_SZ && \n\
               num_children >= %s%s_LIB_NUM_CHILDREN, STRUCTURE);\n\
  HARIS_ASSERT(ind + body_size < sz, INPUT);\n\
  HARIS_ASSERT(ind + body_size < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  return _%s%s_from_buffer_posthead(strct, buf, ind + 2, sz, out_ind, \
depth, num_children, body_size);\n}\n\n",
              prefix, name, prefix, name, prefix, name, prefix, name, 
              prefix, name);
  CJOB_FMT_PRIV_FUNCTION(job, "static HarisStatus _%s%s_from_buffer_posthead(\
%s%s *strct, unsigned char *buf, haris_uint32_t ind, haris_uint32_t sz, \
haris_uint32_t *out_ind, int depth, int num_children, int body_size)\n{\n\
  HarisStatus result;\n\
  haris_uint32_t i;\n\
  %s%s_lib_read_body(strct, buf + ind);\n\
  *out_ind = ind + body_size;\n", prefix, name, prefix, name, prefix, name);
  for (i = 0; i < strct->num_children; i++) {
  	child_name = strct->children[i].name;
    switch (strct->children[i].tag) {
    case CHILD_STRUCT:
      CJOB_FPRINTF(out, "  if ((result = %s%s_init_%s(strct)) != \
HARIS_SUCCESS) return result;\n\
  if ((result = _%s%s_from_buffer(strct->%s, buf, *out_ind, sz, out_ind, \
depth+1)) != \n\
      HARIS_SUCCESS) return result;\n", 
              prefix, name, strct->children[i].name, prefix,
              strct->children[i].type.strct->name, child_name);
      break;
    case CHILD_TEXT:
      CJOB_FPRINTF(out, "  HARIS_ASSERT(*out_ind + 4 < sz, INPUT);\n\
  HARIS_ASSERT(*out_ind + 4 < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  HARIS_ASSERT(buf[*out_ind] == 0xC0, STRUCTURE);\n\
  i = haris_read_uint24(*out_ind + 1);\n\
  HARIS_ASSERT(*out_ind + 4 + i < sz, INPUT);\n\
  HARIS_ASSERT(*out_ind + 4 + i < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  if ((result = %s%s_init_%s(strct, i)) != HARIS_SUCCESS) return result;\n\
  (void)memcpy(strct->%s, but + *out_ind + 4, i);\n\
  *out_ind += 4 + i;\n", prefix, name, child_name, child_name);
      break;
    case CHILD_SCALAR_LIST:
      scalar_sz = sizeof_scalar(strct->children[i].type.scalar_list.tag);
      CJOB_FPRINTF(out, "  HARIS_ASSERT(*out_ind + 4 < sz, INPUT);\n\
  HARIS_ASSERT(*out_ind + 4 < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  HARIS_ASSERT(buf[*out_ind] == 0x%X, STRUCTURE);\n\
  i = haris_read_uint24(*out_ind + 1);\n\
  if ((result = %s%s_init_%s(strct, i)) != HARIS_SUCCESS) return result;\n\
  HARIS_ASSERT(*out_ind + 4 + i * %d < sz, INPUT);\n\
  HARIS_ASSERT(*out_ind + 4 + i * %d < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  *out_ind += 4;\n\
  {\n\
    haris_uint32_t x;\n\
    for (x = 0; x < i; x++)\n\
      strct->%s[x] = haris_read_%s(buf + *out_ind + x * %d);\n\
  }\n\
  *out_ind += x * sizeof *strct->%s;\n", 0xC0 | 
              scalar_bit_pattern(strct->children[i].type.scalar_list.tag),
              prefix, name, child_name, scalar_sz, scalar_sz, child_name,
              scalar_type_suffix(strct->children[i].type.scalar_list.tag),
              scalar_sz, child_name);
      break;
    case CHILD_STRUCT_LIST:
      CJOB_FPRINTF(out, "  HARIS_ASSERT(*out_ind + 6 < sz, INPUT);\n\
  HARIS_ASSERT(*out_ind + 6 < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  HARIS_ASSERT(buf[*out_ind] == 0xE0, STRUCTURE);\n\
  i = haris_read_uint24(*out_ind + 1);\n\
  if ((result = %s%s_init_%s(strct, i)) != HARIS_SUCCESS) return result;\n\
  HARIS_ASSERT(buf[*out_ind + 4] & 0xC0 == 0x40, STRUCTURE);\n\
  {\n\
    haris_uint32_t x, body_size = buf[*out_ind + 5], \
num_children = buf[*out_ind + 4] & 0x3F;\n\
    *out_ind += 6;\n\
    for (x = 0; x < i; x++)\n\
      if ((result = _%s%s_from_buffer_posthead(strct->%s[x], buf, *out_ind, \
                                               sz, out_ind, depth + 1, \
                                               body_size, num_children)) \
           != HARIS_SUCCESS)\n\
        return result;\n\
  }\n", prefix, name, child_name, prefix, 
        strct->children[i].type.struct_list->name, child_name);
        break;
    }
  }
  CJOB_FPRINTF(out, "  for (i = 0; i < num_children - %s%s_LIB_NUM_CHILDREN; \
i++)\n\
    if ((result = handle_child_buffer(buf, *out_ind, sz, out_ind, depth + 1)) \
        != HARIS_SUCCESS)\n\
      return result;\n  return HARIS_SUCCESS;\n}\n\n", prefix, name);
  return CJOB_SUCCESS;
}

static CJobStatus write_static_to_buffer_function(CJob *job, 
                                                  ParsedStruct *strct, 
                                                  FILE *out)
{
  int i;
  const char *prefix = job->prefix, *name = strct->name, *fname;
  CJOB_FPRINTF(out, "static unsigned char *_%s%s_to_buffer(%s%s *strct, \
unsigned char *addr)\n{\n\
  addr = %s%s_lib_write_header(strct, addr);\n\
  if (strct->_null) return addr;\n\
  return _%s%s_to_buffer_posthead(strct, addr);\n}\n\n", 
              prefix, name, prefix, name, prefix, name,
              prefix, name);
  CJOB_FPRINTF(out, "static unsigned char *_%s%s_to_buffer_posthead(\
%s%s *strct, unsigned char *addr)\n{\n\
  addr = %s%s_lib_write_body(strct, addr)", prefix, name, prefix, name, 
               prefix, name);
  for (i = 0; i < strct->num_children; i++) {
    fname = strct->children[i].name;
    switch (strct->children[i].tag) {
    case CHILD_STRUCT:
      CJOB_FPRINTF(out, "  addr = _%s%s_to_buffer(strct->%s, addr);\n",
                  prefix, strct->children[i].type.strct->name, 
                  strct->children[i].name);
      break;
    case CHILD_TEXT:
      if (strct->children[i].nullable) {
        CJOB_FPRINTF(out, "  if (strct->_null_%s) *(addr++) = 0x80;\n\
  else {\n\
    *addr = 0xC0;\n\
    haris_write_uint24(addr + 1, strct->_len_%s);\n\
    (void)memcpy(addr + 4, strct->%s, strct->_len_%s);\n\
    addr += 4 + strct->_len_%s;\n}\n", 
                    fname, fname, fname, fname, fname);
      } else {
        CJOB_FPRINTF(out, "  *addr = 0xC0;\n\
  haris_write_uint24(addr + 1, strct->_len_%s);\n\
  (void)memcpy(addr + 4, strct->%s, strct->_len_%s);\n\
  addr += 4 + strct->_len_%s;\n}\n", 
                    fname, fname, fname, fname);
      }
      break;
    case CHILD_SCALAR_LIST:
      if (strct->children[i].nullable) {
        CJOB_FPRINTF(out, "  if (strct->_null_%s) *(addr++) = 0x80;\n\
  else", fname);
      } 
      CJOB_FPRINTF(out, "  {\n\
    haris_uint32_t x;\n\
    *addr = 0x%x;\n\
    haris_write_uint24(addr + 1, strct->_len_%s);\n\
    addr += 4;\n\
    for (x = 0; x < strct->_len_%s; x++, addr += %d) {\n\
      haris_write_%s(addr, strct->%s[x]);\n\
    }\n  }\n", 0xC0 | 
                scalar_bit_pattern(strct->children[i].type.scalar_list.tag),
                fname, fname, 
                sizeof_scalar(strct->children[i].type.scalar_list.tag),
                scalar_type_suffix(strct->children[i].type.scalar_list.tag),
                fname);
      break;
    case CHILD_STRUCT_LIST:
      if (strct->children[i].nullable) {
        CJOB_FPRINTF(out, "  if (strct->_null_%s) *(addr++) = 0x80;\n\
  else", fname);
      }
      CJOB_FPRINTF(out, "  {\n\
    haris_uint32_t x;\n\
    *addr = 0xE0;\n\
    haris_write_uint24(addr + 1, strct->_len_%s);\n\
    addr = %s%s_lib_write_nonnull_header(addr + 4);\n\
    for (x = 0; x < strct->_len_%s; x++)\n\
      _%s%s_to_buffer_posthead(strct->%s[x], addr);\n  }\n", 
                  fname, prefix, strct->children[i].type.strct->name, 
                  fname, prefix, strct->children[i].type.strct->name,
                  fname);
      break;
    }
  }
  CJOB_FPRINTF(out, "  return addr;\n}\n\n");
  return CJOB_SUCCESS;
}

/* This function handles the _from_buffer and _to_buffer public functions.
   These functions both primarily call their static helper functions, though
   they do some error checking to ensure that the structure produces a 
   valid message.
*/
   
static CJobStatus write_public_buffer_funcs(CJob *job, ParsedStruct *strct,
                                            FILE *out)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FPRINTF(out, "HarisStatus %s%s_from_buffer(%s%s *strct, \
unsigned char *buf, haris_uint32_t sz, unsigned char **out_addr)\n\
{\n\
  HarisStatus result;\n\
  if ((result = _%s%s_from_buffer(strct, buf, 0, sz, out_addr, 0)) \n\
               != HARIS_SUCCESS) return result;\n\
  HARIS_ASSERT(!strct->_null, STRUCTURE);\n\
  return HARIS_SUCCESS;\n}\n\n", 
              prefix, name, prefix, name, prefix, name);
  CJOB_FPRINTF(out, "HarisStatus %s%s_to_buffer(%s%s *strct, \
unsigned char **out_buf, haris_uint32_t *out_sz)\n\
{\n\
  unsigned char *unused;\n\
  HarisStatus result;\n\
  HARIS_ASSERT(!strct->_null, STRUCTURE);\n\
  *out_sz = %s%s_lib_size(strct, 0, &result);\n\
  if (!*out_sz) return result;\n\
  *out_buf = (unsigned char *)malloc(sz);\n\
  HARIS_ASSERT(*out_buf, MEM);\n\
  (void)_%s%s_to_buffer(strct, *out_buf);\n\
  return HARIS_SUCCESS;\n}\n\n",
              prefix, name, prefix, name, prefix, name, prefix, name);
  return CJOB_SUCCESS;
}
