#ifndef CBL_CORE_STREAM_H_
#define CBL_CORE_STREAM_H_

#include <stdio.h>

#include <cable/core/object.h>
#include <cable/core/data.h>
#include <cable/core/error.h>

CBL_EXTERN_BEGIN

typedef struct CblStream CblStream;
CblClass * const CBL_STREAM_CLASS;

void cblStreamClose(CblAllocator *alloc, CblStream *stream, CblError **error);

void cblStreamFlush(CblAllocator *alloc, CblStream *stream, CblError **error);

typedef struct CblStream CblInputStream;

CblInputStream *cblInputStreamNew(CblAllocator *alloc, CblString *location, CblError **error);

CblInputStream *cblInputStreamNewFromCStream(CblAllocator *alloc, FILE *cstream);

CblInputStream *cblInputStreamNewFromData(CblAllocator *alloc, CblData *data);

size_t cblStreamReadBytes(CblAllocator *alloc, CblInputStream *stream, uint8_t *buffer, size_t length, CblError **error);

typedef struct CblStream CblOutputStream;

CblOutputStream *cblOutputStreamNew(CblAllocator *alloc, CblString *location, bool append, CblError **error);

CblOutputStream *cblOutputStreamNewFromCStream(CblAllocator *alloc, FILE *cstream);

CblOutputStream *cblOutputStreamNewFromData(CblAllocator *alloc, CblMutableData *data, bool append);

size_t cblStreamWriteBytes(CblAllocator *alloc, CblOutputStream *stream, const uint8_t *bytes, size_t length, CblError **error);

CBL_EXTERN_END

#endif // CBL_CORE_ERROR_H_