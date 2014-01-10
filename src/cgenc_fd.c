#include "cgenc_fd.h"

static CJobStatus write_fd_structures(CJob *);
static CJobStatus write_static_fd_funcs(CJob *);
static CJobStatus write_public_fd_funcs(CJob *, ParsedStruct *);

/* =============================PUBLIC INTERFACE============================= */

CJobStatus write_fd_protocol_funcs(CJob *job)
{
  CJobStatus result;
  int i;
  ParsedSchema *schema = job->schema;
  if ((result = write_fd_structures(job)) != CJOB_SUCCESS ||
      (result = write_static_fd_funcs(job)) != CJOB_SUCCESS)
    return result;
  for (i = 0; i < schema->num_structs; i ++) {
    if ((result = write_public_fd_funcs(job, &schema->structs[i])) 
        != CJOB_SUCCESS)
      return result;
  }
  return CJOB_SUCCESS;
}

/* =============================STATIC FUNCTIONS============================= */

static CJobStatus write_fd_structures(CJob *job)
{
  CJOB_FMT_HEADER_STRING(job, "#include <unistd.h>\n#include<errno.h>\n\n");
  CJOB_FMT_HEADER_STRING(job, 
"typedef struct {\n\
  int fd;\n\
  haris_uint32_t curr;\n\
  unsigned char buffer[1000];\n\
} HarisFdStream;\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_static_fd_funcs(CJob *job)
{
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus read_from_fd_stream(void *_stream,\n\
                                         haris_uint32_t count,\n\
                                         const unsigned char **dest)\n\
{\n\
  HarisFdStream *stream = (HarisFdStream*)_stream;\n\
  ssize_t result;\n\
  haris_uint32_t bytes_read = 0;\n\
  if (count == 0) return HARIS_SUCCESS;\n\
  HARIS_ASSERT(count + stream->curr <= HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  HARIS_ASSERT(count <= 1000, SIZE);\n\
  do {\n\
    result = read(stream->fd, stream->buffer + bytes_read, \n\
                  count - bytes_read);\n\
    if (result < 0) {\n\
      if (errno != EINTR)\n\
        return HARIS_INPUT_ERROR;\n\
      else \n\
        continue;\n\
    } else if (result == 0) { /* EOF: error */\n\
      return HARIS_INPUT_ERROR;\n\
    } else {\n\
      bytes_read += result;\n\
    }\n\
  } while (bytes_read < count);\n\
  *dest = stream->buffer;\n\
  stream->curr += count;\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus force_write_to_fd_stream(int fd, \n\
                                             const unsigned char *src,\n\
                                             haris_uint32_t count)\n\
{\n\
  ssize_t result;\n\
  haris_uint32_t bytes_written = 0;\n\
  if (count == 0) return HARIS_SUCCESS;\n\
  do {\n\
    result = write(fd, src + bytes_written, count - bytes_written);\n\
    if (result < 0) {\n\
      if (errno != EINTR)\n\
        return HARIS_INPUT_ERROR;\n\
      else\n\
        continue;\n\
    } else {\n\
      bytes_written += result;\n\
    }\n\
  } while (bytes_written < count);\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus write_to_fd_stream(void *_stream,\n\
                                        const unsigned char *src,\n\
                                        haris_uint32_t count)\n\
{\n\
  HarisFdStream *stream = (HarisFdStream*)_stream;\n\
  HarisStatus result;\n\
  haris_uint32_t copy_size;\n\
  HARIS_ASSERT(count <= 1000, SIZE);\n\
  if (count == 0) return HARIS_SUCCESS;\n\
  if (count + stream->curr > 1000) {\n\
    copy_size = 1000 - stream->curr;\n\
    memcpy(stream->buffer + stream->curr, src, copy_size);\n\
    if ((result = force_write_to_fd_stream(stream->fd, stream->buffer,\n\
                                           1000)) != HARIS_SUCCESS)\n\
      return result;\n\
    memcpy(stream->buffer, src + copy_size, count - copy_size);\n\
    stream->curr = count - copy_size;\n\
  } else {\n\
    memcpy(stream->buffer + stream->curr, src, count);\n\
    stream->curr += count;\n\
  }\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus _public_to_fd(void *ptr,\n\
                                   const HarisStructureInfo *info,\n\
                                   int fd,\n\
                                   haris_uint32_t *out_sz)\n\
{\n\
  HarisStatus result;\n\
  HarisFdStream fd_stream;\n\
  haris_uint32_t encoded_size = haris_lib_size(ptr, info, 0, &result);\n\
  if (encoded_size == 0) return result;\n\
  HARIS_ASSERT(encoded_size <= HARIS_MESSAGE_SIZE_LIMIT, SIZE);\n\
  fd_stream.fd = fd;\n\
  fd_stream.curr = 0;\n\
  if ((result = _haris_to_stream(ptr, info, &fd_stream,\n\
                                 write_to_fd_stream)) != HARIS_SUCCESS)\n\
    return result;\n\
  if ((result = force_write_to_fd_stream(fd, fd_stream.buffer, \n\
                                         fd_stream.curr)) != HARIS_SUCCESS)\n\
    return result;\n\
  if (out_sz) *out_sz = encoded_size;\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  CJOB_FMT_PRIV_FUNCTION(job,
"static HarisStatus _public_from_fd(void *ptr,\n\
                                     const HarisStructureInfo *info,\n\
                                     int fd,\n\
                                     haris_uint32_t *out_sz)\n\
{\n\
  HarisStatus result;\n\
  HarisFdStream fd_stream;\n\
  fd_stream.fd = fd;\n\
  fd_stream.curr = 0;\n\
  if ((result = _haris_from_stream(ptr, info, &fd_stream,\n\
                                   read_from_fd_stream, 0)) != HARIS_SUCCESS)\n\
    return result;\n\
  if (out_sz) *out_sz = fd_stream.curr;\n\
  return HARIS_SUCCESS;\n\
}\n\n");
  return CJOB_SUCCESS;
}

static CJobStatus write_public_fd_funcs(CJob *job, ParsedStruct *strct)
{
  const char *prefix = job->prefix, *name = strct->name;
  CJOB_FMT_PUB_FUNCTION(job,
"HarisStatus %s%s_to_fd(%s%s *strct, int fd, \n\
                          haris_uint32_t *out_sz)\n\
{\n\
  return _public_to_fd(strct, &haris_lib_structures[%d],\n\
                         fd, out_sz);\n}\n\n",
                        prefix, name, prefix, name, strct->schema_index);
  CJOB_FMT_PUB_FUNCTION(job,
"HarisStatus %s%s_from_fd(%s%s *strct, int fd,\n\
                            haris_uint32_t *out_sz)\n\
{\n\
  return _public_from_fd(strct, &haris_lib_structures[%d],\n\
                           fd, out_sz);\n}\n\n",
                        prefix, name, prefix, name, strct->schema_index);
  return CJOB_SUCCESS;
}
