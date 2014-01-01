#include "cgenc_buffer.h"

static CJobStatus write_child_buffer_handler(CJob *);

static CJobStatus write_public_buffer_funcs(CJob *, ParsedStruct *);
static CJobStatus write_static_buffer_funcs(CJob *);

static CJobStatus write_static_from_buffer_functions(CJob *);
static CJobStatus write_static_to_buffer_functions(CJob *);

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
        != CJOB_SUCCESS) 
      return result;
  }
  if ((result = write_static_buffer_funcs(job)) != CJOB_SUCCESS) 
    return result;
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
  CJOB_FMT_PRIV_FUNCTION(job, 
"static HarisStatus handle_child_buffer(unsigned char *buf, haris_uint32_t *out_ind,\n\
                                        haris_uint32_t sz, int depth)\n\
{\n\
  HarisStatus result;\n\
  int num_children, body_size, el_size;\n\
  haris_uint32_t i, ind = *out_ind;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  HARIS_ASSERT(ind < sz, INPUT);\n\
  HARIS_ASSERT(ind < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  if (buf[ind] == 0x80 || buf[ind] == 0x0) { *out_ind = ind + 1; \
return HARIS_SUCCESS; }\n\
  HARIS_ASSERT((ind + 1 < sz) && (ind + 1 < HARIS_MESSAGE_SIZE_LIMIT),\
               INPUT);\n\
  if ((buf[ind] & 0xC0) == 0x40) {\n\
    num_children = buf[ind] & 0x3F;\n\
    body_size = buf[ind + 1];\n\
    *out_ind = ind + 2;\n\
    return handle_child_buffer_struct_posthead(buf, out_ind, sz, \
depth, num_children, body_size);\n\
  } else if ((buf[ind] & 0xE0) == 0xC0) {\n\
    HARIS_ASSERT(ind + 3 < sz, INPUT);\n\
    HARIS_ASSERT(ind + 3 < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
    haris_read_uint24(buf + ind + 1, &i);\n\
    el_size = buf[ind] & 0x3;\n\
    *out_ind = ind + i * el_size + 4;\n\
    return HARIS_SUCCESS;\n\
  } else {\n\
    haris_uint32_t x;\n\
    HARIS_ASSERT(ind + 5 < sz, INPUT);\n\
    HARIS_ASSERT(ind + 5 < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
    haris_read_uint24(buf + ind + 1, &i);\n\
    num_children = buf[ind + 4] & 0x7F;\n\
    body_size = buf[ind + 5];\n\
    *out_ind = ind + 6;\n\
    for (x = 0; x < i; x++)\n\
      if ((result = handle_child_buffer_struct_posthead(buf, out_ind, sz, \n\
           depth + 1, num_children, body_size)) != HARIS_SUCCESS)\n\
        return result;\n\
    return HARIS_SUCCESS;\n\
  }\n}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static HarisStatus handle_child_buffer_struct_posthead(unsigned char *buf,\n\
                                                        haris_uint32_t *out_ind,\n\
                                                        haris_uint32_t sz, \n\
                                                        int depth, int num_children,\n\
                                                        int body_size)\n\
{\n\
  HarisStatus result;\n\
  haris_uint32_t ind = *out_ind;\n\
  int i;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  HARIS_ASSERT(ind + body_size < sz, INPUT);\n\
  HARIS_ASSERT(ind + body_size < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  *out_ind = ind + body_size;\n\
  for (i = 0; i < num_children; i++)\n\
    if ((result = handle_child_buffer(buf, out_ind, sz,\n\
         depth + 1)) != HARIS_SUCCESS)\n\
      return result;\n\
  return HARIS_SUCCESS;\n}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_static_buffer_funcs(CJob *job)
{
  CJobStatus result;
  if ((result = write_static_from_buffer_functions(job))
      != CJOB_SUCCESS) return result;
  if ((result = write_static_to_buffer_functions(job))
      != CJOB_SUCCESS) return result;
  return CJOB_SUCCESS;
}

static CJobStatus write_static_from_buffer_functions(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static HarisStatus haris_from_buffer(void *ptr,\n\
  const HarisStructureInfo *info, unsigned char *buf,\n\
  haris_uint32_t *out_ind, haris_uint32_t sz, int depth)\n\
{\n\
  int num_children, body_size;\n\
  haris_uint32_t ind = *out_ind;\n\
  HARIS_ASSERT(depth <= HARIS_DEPTH_LIMIT, DEPTH);\n\
  HARIS_ASSERT(ind < sz, INPUT);\n\
  HARIS_ASSERT(ind < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  HARIS_ASSERT(!(buf[ind] & 0x80), STRUCTURE); \n\
  if (!buf[ind]) { \n\
    *(char*)ptr = 1; \n\
    *out_ind = ind + 1; \n\
    return HARIS_SUCCESS;\n\
  }\n\
  HARIS_ASSERT(ind + 1 < sz, INPUT);\n\
  HARIS_ASSERT(ind + 1 < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  num_children = buf[ind];\n\
  body_size = buf[ind + 1];\n\
  HARIS_ASSERT(body_size >= info->body_size &&\n\
               num_children >= info->num_children, STRUCTURE);\n\
  HARIS_ASSERT(ind + body_size < sz, INPUT);\n\
  HARIS_ASSERT(ind + body_size < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  *out_ind = ind + 2;\n\
  return haris_from_buffer_posthead(ptr, info, buf, sz, out_ind, depth, num_children, body_size);\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job, 
"static HarisStatus haris_from_buffer_posthead(void *ptr,\n\
  const HarisStructureInfo *info, unsigned char *buf,\n\
  haris_uint32_t sz, haris_uint32_t *out_ind, int depth, int num_children,\n\
  int body_size)\n\
{\n\
  HarisStatus result;\n\
  int i;\n\
  haris_uint32_t x, ind;\n\
  const HarisChild *child;\n\
  HarisListInfo *list_info;\n\
  haris_lib_read_body(ptr, info, buf + *out_ind);\n\
  *out_ind += body_size;\n\
  for (i = 0; i < info->num_children; i ++) {\n\
    ind = *out_ind;\n\
    child = &info->children[i];\n\
    list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
    HARIS_ASSERT(ind + 1 < sz, INPUT);\n\
    HARIS_ASSERT(ind + 1 < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
    if (!(buf[ind]&0x40)) {\n\
      HARIS_ASSERT(child->nullable, STRUCTURE);\n\
      if (child->child_type != HARIS_CHILD_STRUCT) \n\
        list_info->null = 1;\n\
      else \n\
        *(void**)list_info = NULL;\n\
      *out_ind = ind + 1;\n\
      continue;\n\
    }\n\
    switch (child->child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_CHILD_SCALAR_LIST:\n\
    {\n\
      haris_uint32_t len, el_size;\n\
      HARIS_ASSERT(ind + 4 < sz, INPUT);\n\
      HARIS_ASSERT(ind + 4 < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
      HARIS_ASSERT(buf[ind] == (child->child_type == HARIS_CHILD_TEXT ? 0xC0 : \n\
        0xC0 | haris_lib_scalar_bit_patterns[child->scalar_element]), STRUCTURE);\n\
      haris_read_uint24(buf + ind + 1, (void*)&len);\n\
      el_size = (child->child_type == HARIS_CHILD_TEXT) ? 1 : \n\
        haris_lib_message_scalar_sizes[child->scalar_element];\n\
      HARIS_ASSERT(ind + 4 + len * el_size < sz, INPUT);\n\
      HARIS_ASSERT(ind + 4 + len * el_size < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
      if ((result = haris_lib_init_list_mem(ptr, info, i, len)) != HARIS_SUCCESS)\n\
        return result;\n\
      ind += 4;\n\
      for (x = 0; x < len; x ++, ind += el_size) {\n\
        haris_lib_read_scalar(buf + ind, \n\
                              (char*)list_info->ptr + \n\
                               x * haris_lib_in_memory_scalar_sizes[child->scalar_element],\n\
                              child->child_type == HARIS_CHILD_TEXT ? \n\
                              HARIS_SCALAR_UINT8 : child->scalar_element);\n\
      }\n\
      *out_ind = ind;\n\
      break;\n\
    }\n\
    case HARIS_CHILD_STRUCT_LIST:\n\
    {\n\
      haris_uint32_t len, body_size, num_children;\n\
      HARIS_ASSERT(ind + 6 < sz, INPUT);\n\
      HARIS_ASSERT(ind + 6 < HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
      HARIS_ASSERT(buf[ind] == 0xE0, STRUCTURE);\n\
      haris_read_uint24(buf + ind + 1, (void*)&len);\n\
      if ((result = haris_lib_init_list_mem(ptr, info, i, len)) != HARIS_SUCCESS)\n\
        return result;\n\
      HARIS_ASSERT((buf[ind+4] & 0xC0) == 0x40, STRUCTURE);\n\
      num_children = buf[ind + 4] & 0x3F;\n\
      body_size = buf[ind + 5];\n\
      *out_ind = ind + 6;\n\
      for (x = 0; x < len; x ++) {\n\
        if ((result = haris_from_buffer_posthead((char*)list_info->ptr + \n\
                                                x * child->struct_element->size_of, \n\
                                               child->struct_element, buf, sz, \n\
                                               out_ind, depth + 1, num_children, \n\
                                               body_size)) != HARIS_SUCCESS) \n\
          return result;\n\
      }\n\
      break;\n\
    }\n\
    case HARIS_CHILD_STRUCT:\n\
    {\n\
      if ((result = haris_lib_init_struct_mem(ptr, info, i)) != HARIS_SUCCESS)\n\
        return result;\n\
      if ((result = haris_from_buffer(list_info->ptr, child->struct_element, \n\
                                      buf, out_ind, sz, depth + 1)) \n\
          != HARIS_SUCCESS)\n\
        return result;\n\
      break;\n\
    }\n\
    }\n\
  }\n\
  for (; i < num_children; i ++) {\n\
    if ((result = handle_child_buffer(buf, out_ind, sz, depth + 1))\n\
        != HARIS_SUCCESS)\n\
      return result;\n\
  }\n\
  return HARIS_SUCCESS;\n}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_static_to_buffer_functions(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job, 
"static unsigned char *haris_to_buffer(void *ptr,\n\
  const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  buf = haris_lib_write_header(ptr, info, buf);\n\
  if (*(char*)ptr) return buf;\n\
  return haris_to_buffer_posthead(ptr, info, buf);\n\
}\n\
\n\
");
  CJOB_FMT_PRIV_FUNCTION(job,
"static unsigned char *haris_to_buffer_posthead(void *ptr, \n\
  const HarisStructureInfo *info, unsigned char *buf)\n\
{\n\
  int i, el_size;\n\
  const HarisChild *child;\n\
  HarisListInfo *list_info;\n\
  haris_uint32_t x;\n\
  buf = haris_lib_write_body(ptr, info, buf);\n\
  for (i = 0; i < info->num_children; i ++) {\n\
    child = &info->children[i];\n\
    list_info = (HarisListInfo*)((char*)ptr + child->offset);\n\
    switch (child->child_type) {\n\
    case HARIS_CHILD_TEXT:\n\
    case HARIS_CHILD_SCALAR_LIST:\n\
      el_size = (child->child_type == HARIS_CHILD_TEXT ? 1 : \n\
        haris_lib_message_scalar_sizes[child->scalar_element]);\n\
      if (list_info->null) {\n\
        *buf = 0x80;\n\
        buf++;\n\
        continue;\n\
      }\n\
      *buf = 0xC0 | (child->child_type == HARIS_CHILD_TEXT ? 0 : \n\
                     haris_lib_scalar_bit_patterns[child->scalar_element]);\n\
      haris_write_uint24(buf + 1, &list_info->len);\n\
      buf += 4;\n\
      for (x = 0; x < list_info->len; x ++, buf += el_size)\n\
        haris_lib_write_scalar(buf, \n\
                               (char*)list_info->ptr + \n\
                               x * haris_lib_in_memory_scalar_sizes[child->scalar_element],\n\
                               child->child_type == HARIS_CHILD_TEXT ? \n\
                               HARIS_SCALAR_UINT8 : child->scalar_element);\n\
      break;\n\
    case HARIS_CHILD_STRUCT_LIST:\n\
      if (list_info->null) {\n\
        *buf = 0x80;\n\
        buf++;\n\
        continue;\n\
      }\n\
      *buf = 0xE0;\n\
      haris_write_uint24(buf + 1, &list_info->len);\n\
      buf = haris_lib_write_nonnull_header(child->struct_element, buf + 4);\n\
      for (x = 0; x < list_info->len; x ++)\n\
        buf = haris_to_buffer_posthead((char*)list_info->ptr + \n\
                                         x * child->struct_element->size_of,\n\
                                       child->struct_element, buf);\n\
      break;\n\
    case HARIS_CHILD_STRUCT:\n\
      buf = haris_to_buffer(list_info->ptr, child->struct_element, buf);\n\
      break;\n\
    }\n\
  }\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  return CJOB_SUCCESS;
}

/* This function handles the _from_buffer and _to_buffer public functions.
   These functions both primarily call their static helper functions, though
   they do some error checking to ensure that the structure produces a 
   valid message.
*/
   
static CJobStatus write_public_buffer_funcs(CJob *job, ParsedStruct *strct)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job, 
"HarisStatus %s%s_from_buffer(%s%s *strct, unsigned char *buf, haris_uint32_t sz,\n\
                              unsigned char **out_addr)\n\
{\n\
  HarisStatus result;\n\
  haris_uint32_t ind = 0;\n\
  if ((result = haris_from_buffer(strct, &haris_lib_structures[%d],\n\
                                  buf, &ind, sz, 0)) != HARIS_SUCCESS)\n\
    return result;\n\
  *out_addr = buf + ind;\n\
  HARIS_ASSERT(!strct->_null, STRUCTURE);\n\
  return HARIS_SUCCESS;\n}\n\n", 
              prefix, name, prefix, name, strct->schema_index);
  CJOB_FMT_PUB_FUNCTION(job, 
"HarisStatus %s%s_to_buffer(%s%s *strct, unsigned char **out_buf, \n\
                            haris_uint32_t *out_sz)\n\
{\n\
  HarisStatus result;\n\
  HARIS_ASSERT(!strct->_null, STRUCTURE);\n\
  *out_sz = haris_lib_size(strct, &haris_lib_structures[%d], 0, &result);\n\
  if (!*out_sz) return result;\n\
  *out_buf = (unsigned char *)malloc(*out_sz);\n\
  HARIS_ASSERT(*out_buf, MEM);\n\
  (void)haris_to_buffer(strct, &haris_lib_structures[%d], *out_buf);\n\
  return HARIS_SUCCESS;\n}\n\n",
              prefix, name, prefix, name, strct->schema_index,
              strct->schema_index);
  return CJOB_SUCCESS;
}
