#include "cgenc_buffer.h"

static CJobStatus write_buffer_structures(CJob *);
static CJobStatus write_public_buffer_funcs(CJob *, ParsedStruct *);
static CJobStatus write_static_buffer_funcs(CJob *);

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_buffer_protocol_funcs(CJob *job)
{
  CJobStatus result;
  int i;
  ParsedSchema *schema = job->schema;
  if ((result = write_buffer_structures(job)) != CJOB_SUCCESS ||
      (result = write_static_buffer_funcs(job)) != CJOB_SUCCESS)
    return result;
  for (i = 0; i < schema->num_structs; i++) {
    if ((result = write_public_buffer_funcs(job, &schema->structs[i])) 
        != CJOB_SUCCESS) 
      return result;
  }
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

static CJobStatus write_buffer_structures(CJob *job)
{
  CJOB_FMT_HEADER_STRING(job,
"typedef struct {\n\
  unsigned char *buffer;\n\
  haris_uint32_t curr;\n\
  haris_uint32_t sz;\n\
} HarisBufferStream;\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_static_buffer_funcs(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus read_from_buffer_stream(void *_stream, \n\
                                           haris_uint32_t count,\n\
                                           const unsigned char **dest)\n\
{\n\
  HarisBufferStream *stream = (HarisBufferStream*)_stream;\n\
  HARIS_ASSERT(count + stream->curr <= stream->sz, INPUT);\n\
  HARIS_ASSERT(count + stream->curr <= HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  *dest = stream->buffer + stream->curr;\n\
  stream->curr += count;\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus write_to_buffer_stream(void *_stream,\n\
                                          const unsigned char *src,\n\
                                          haris_uint32_t count)\n\
{\n\
  HarisBufferStream *stream = (HarisBufferStream*)_stream;\n\
  /* No error checking necessary; the size of the structure is verified\n\
     before serialization begins */\n\
  memcpy(stream->buffer + stream->curr, src, count);\n\
  stream->curr += count;\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus _public_to_buffer_a(void *ptr,\n\
                                        const HarisStructureInfo *info,\n\
                                        unsigned char **out_buf,\n\
                                        haris_uint32_t *out_sz)\n\
{\n\
  HarisStatus result;\n\
  HarisBufferStream buffer_stream;\n\
  haris_uint32_t encoded_size = haris_lib_size(ptr, info, 0, &result);\n\
  if (encoded_size == 0) return result;\n\
  HARIS_ASSERT(encoded_size <= HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  buffer_stream.buffer = (unsigned char *)malloc(encoded_size);\n\
  HARIS_ASSERT(buffer_stream.buffer, MEM);\n\
  buffer_stream.curr = 0;\n\
  if ((result = _haris_to_stream(ptr, info, &buffer_stream, \n\
                                 write_to_buffer_stream)) != HARIS_SUCCESS)\n\
    return result;\n\
  *out_sz = encoded_size;\n\
  *out_buf = buffer_stream.buffer;\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus _public_to_buffer(void *ptr,\n\
                                      const HarisStructureInfo *info,\n\
                                      unsigned char *buf,\n\
                                      haris_uint32_t sz,\n\
                                      unsigned char **out_addr)\n\
{\n\
  HarisStatus result;\n\
  HarisBufferStream buffer_stream;\n\
  haris_uint32_t encoded_size = haris_lib_size(ptr, info, 0, &result);\n\
  if (encoded_size == 0) return result;\n\
  HARIS_ASSERT(encoded_size <= HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  HARIS_ASSERT(encoded_size <= sz, INPUT);\n\
  buffer_stream.buffer = buf;\n\
  buffer_stream.curr = 0;\n\
  if ((result = _haris_to_stream(ptr, info, &buffer_stream,\n\
                                 write_to_buffer_stream)) != HARIS_SUCCESS)\n\
    return result;\n\
  if (out_addr) *out_addr = buf + buffer_stream.curr;\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus _public_from_buffer(void *ptr,\n\
                                       const HarisStructureInfo *info,\n\
                                       unsigned char *buf,\n\
                                       haris_uint32_t sz,\n\
                                       unsigned char **out_addr)\n\
{\n\
  HarisStatus result;\n\
  HarisBufferStream buffer_stream;\n\
  buffer_stream.buffer = buf;\n\
  buffer_stream.sz = sz;\n\
  buffer_stream.curr = 0;\n\
  if ((result = _haris_from_stream(ptr, info, &buffer_stream, \n\
                                   read_from_buffer_stream, 0)) != HARIS_SUCCESS)\n\
    return result;\n\
  if (out_addr) *out_addr = buf + buffer_stream.curr;\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  return CJOB_SUCCESS;
}
   
static CJobStatus write_public_buffer_funcs(CJob *job, ParsedStruct *strct)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job, 
"HarisStatus %s%s_from_buffer(%s%s *strct, unsigned char *buf,\n\
                              haris_uint32_t sz,\n\
                              unsigned char **out_addr)\n\
{\n\
  return _public_from_buffer(strct, &haris_lib_structures[%d],\n\
                             buf, sz, out_addr);\n}\n\n", 
                        prefix, name, prefix, name, strct->schema_index);
  CJOB_FMT_PUB_FUNCTION(job, 
"HarisStatus %s%s_to_buffer_a(%s%s *strct, unsigned char **out_buf, \n\
                              haris_uint32_t *out_sz)\n\
{\n\
  return _public_to_buffer_a(strct, &haris_lib_structures[%d],\n\
                             out_buf, out_sz);\n}\n\n",
                        prefix, name, prefix, name, strct->schema_index);
  CJOB_FMT_PUB_FUNCTION(job,
"HarisStatus %s%s_to_buffer(%s%s *strct, unsigned char *buf,\n\
                            haris_uint32_t sz,\n\
                            unsigned char **out_addr)\n\
{\n\
  return _public_to_buffer(strct, &haris_lib_structures[%d],\n\
                           buf, sz, out_addr);\n}\n\n",
                        prefix, name, prefix, name, strct->schema_index);
  return CJOB_SUCCESS;
}
