#include <cable/core/array.h>
#include <cable/core/data.h>
#include <cable/core/string.h>
#include <cable/core/allocator.h>

static const size_t DEFAULT_SIZE = 16;

static CblArrayContext OBJECT_CONTEXT = {
        .ownCallback = (CblArrayOwnCallback)cblOwnInOwner,
        .disownCallback = (CblArrayDisownCallback)cblDisownInOwner,
        .compareCallback = cblCompare,
        .stringCallback = cblGetString,
};

const CblArrayContext * const CBL_ARRAY_CONTEXT_OBJECTS = &OBJECT_CONTEXT;

static CblArrayContext ALLOC_CONTEXT = {
        .ownCallback = NULL,
        .disownCallback = (CblArrayDisownCallback)cblDeallocateInOwner,
        .compareCallback = NULL,
        .stringCallback = NULL
};

const CblArrayContext * const CBL_ARRAY_CONTEXT_ALLOCS = &ALLOC_CONTEXT;

struct CblArray {
    CblConcreteObject isa;
    CblArrayContext context;
    CblMutableData *buffer;
};

static CblMutableArray *createArgsArray(CblAllocator *alloc,
                                        const CblArrayContext *context,
                                        va_list args) {
    CblMutableArray *array = cblMutableArrayNew(alloc, context);
    cblReturnUnless(array, NULL);
    CblObject *obj = va_arg(args, CblObject *);
    while (obj) {
        cblArrayAppend(array, obj);
        obj = va_arg(args, CblObject *);
    }
    return array;
}

static void finalize(CblMutableArray *array) {
    cblArrayEmpty(array);
    cblDisown(array->buffer);
}

static inline size_t getDataVirtualLength(CblData *data) {
    return cblDataGetLength(data) / sizeof(void *);
}

static CblString *stringCallback(CblAllocator *alloc, CblMutableArray *array) {
    cblReturnUnless(array, NULL);
    const CblArrayContext *context = &array->context;
    cblReturnUnless(context->stringCallback, NULL);
    CblMutableString *string = cblMutableStringNewWithCString(alloc, "[");
    size_t length = getDataVirtualLength(array->buffer);
    const void **buffer = (const void **)cblDataGetBytePointer(array->buffer);
    for (size_t i = 0; i < length; ++i) {
        CblString *temp = context->stringCallback(alloc, buffer[i]);
        cblStringAppend(string, temp);
        cblDisown(temp);
        if (i < length - 1) {
            cblStringAppendCString(string, ", ");
        }
    }
    cblStringAppendCString(string, "]");
    return string;
}

static int arrayCompare(CblArray *lhs, CblArray *rhs) {
    cblReturnUnless(lhs && rhs, -1);
    size_t lhsLength = getDataVirtualLength(lhs->buffer);
    size_t rhsLength = getDataVirtualLength(rhs->buffer);
    if (lhsLength > rhsLength) {
        return 1;
    }
    if (lhsLength < rhsLength){
        return -1;
    }
    const void **lhsBuffer = (const void **)cblDataGetBytePointer(lhs->buffer);
    const void **rhsBuffer = (const void **)cblDataGetBytePointer(rhs->buffer);
    const CblArrayContext *context = &lhs->context;
    for (size_t i = 0; i < lhsLength; ++i) {
        if (!context->compareCallback) {
            if (lhsBuffer[i] != rhsBuffer[i]) {
                return -1;
            }
            continue;
        }
        int cmp = context->compareCallback(lhsBuffer[i], rhsBuffer[i]);
        if (cmp != 0) {
            return cmp;
        }
    }
    return 0;
}

static CblClass ARRAY_CLASS = {
        .name = "CblArray",
        .finalizeCallback = (CblObjectFinalizeCallback)finalize,
        .hashCallback = NULL,
        .compareCallback = (CblObjectCompareCallback)arrayCompare,
        .stringCallback = (CblObjectStringCallback)stringCallback
};

const CblClass * const CBL_ARRAY_CLASS = &ARRAY_CLASS;

CblArray *cblArrayNew(CblAllocator *alloc, const CblArrayContext *context, const void **elements, size_t length) {
    return cblMutableArrayNewWithElements(alloc, context, elements, length);
}

CblArray *cblArrayNewCopy(CblAllocator *alloc, CblArray *array) {
    return cblMutableArrayNewCopy(alloc, array);
}

CblArray *cblArrayNewArgs(CblAllocator *alloc, const CblArrayContext *context, ...) {
    va_list args;
    va_start(args, context);
    return cblMutableArrayNewArgsList(alloc, context, args);
    va_end(args);
}

CblArray *cblArrayNewArgsList(CblAllocator *alloc, const CblArrayContext *context, va_list args) {
    return cblMutableArrayNewArgsList(alloc, context, args);
}

CblMutableArray *cblMutableArrayNew(CblAllocator *alloc, const CblArrayContext *context) {
    return cblMutableArrayNewWithElements(alloc, context, NULL, 0);
}

CblMutableArray *cblMutableArrayNewWithElements(CblAllocator *alloc,
                                                const CblArrayContext *context,
                                                const void **elements,
                                                size_t length)
{
    if (!context) {
        context = CBL_ARRAY_CONTEXT_OBJECTS;
    }
    if (!elements && length < DEFAULT_SIZE) {
        length = DEFAULT_SIZE;
    }
    CblMutableData *buffer = cblMutableDataNewWithBytes(alloc, (uint8_t *)elements, sizeof(void *) * length);
    cblReturnUnless(buffer, NULL);
    CblMutableArray *array = cblAllocatorAllocate(alloc, sizeof(CblMutableArray));
    if (!array) {
        cblAllocatorDeallocate(alloc, buffer);
        return NULL;
    }
    cblInitialize(array, alloc, &ARRAY_CLASS);
    array->context = *context;
    array->buffer = buffer;
    if (elements) {
        for (size_t i = 0; i < length; ++i) {
            if (context->ownCallback) {
                context->ownCallback(array, elements[i]);
            }
        }
    } else {
        cblDataSetLength(buffer, 0);
    }
    return array;
}

CblMutableArray *cblMutableArrayNewCopy(CblAllocator *alloc, CblArray *array) {
    size_t length = getDataVirtualLength(array->buffer);
    const void **buffer = (const void **)cblDataGetBytePointer(array->buffer);
    return cblMutableArrayNewWithElements(alloc, &array->context, buffer, length);
}

CblMutableArray *cblMutableArrayNewArgs(CblAllocator *alloc, const CblArrayContext *context, ...) {
    va_list args;
    va_start(args, context);
    return cblMutableArrayNewArgsList(alloc, context, args);
    va_end(args);
}


CblMutableArray *cblMutableArrayNewArgsList(CblAllocator *alloc, const CblArrayContext *context, va_list args) {
    return createArgsArray(alloc, context, args);
}

size_t cblArrayGetLength(CblArray *array) {
    cblReturnUnless(array, 0);
    return getDataVirtualLength(array->buffer);
}

const void *cblArrayGet(CblArray *array, size_t index) {
    cblReturnUnless(array && index < getDataVirtualLength(array->buffer), NULL);
    const void **buffer = (const void **)cblDataGetBytePointer(array->buffer);
    return buffer[index];
}

bool cblArrayForeach(CblArray *array, CblArrayForeachFunction foreachFunction, void *userData) {
    cblReturnUnless(array && foreachFunction, false);
    size_t length = getDataVirtualLength(array->buffer);
    const void **buffer = (const void **)cblDataGetBytePointer(array->buffer);
    for (size_t i = 0; i < length; ++i) {
        if (foreachFunction(array, buffer[i], i, userData)) {
            return true;
        }
    }
    return false;
}

const void *cblArraySet(CblMutableArray *array, size_t index, const void *value) {
    cblReturnUnless(array && value && index < getDataVirtualLength(array->buffer), NULL);
    const void **buffer = (const void **)cblDataGetBytePointer(array->buffer);
    const CblArrayContext *context = &array->context;
    if (context->disownCallback) {
        context->disownCallback(array, buffer[index]);
    }
    if (context->ownCallback) {
        context->ownCallback(array, value);
    }
    buffer[index] = value;
    return value;
}

size_t cblArrayInsert(CblMutableArray *array, size_t index, const void *value) {
    cblReturnUnless(array && value, CBL_NOT_FOUND);
    const CblArrayContext *context = &array->context;
    if (context->ownCallback) {
        context->ownCallback(array, value);
    }
    cblDataReplaceBytes(array->buffer, cblRangeNew(index * sizeof(void *), 0), (uint8_t *)&value, sizeof(void *));
    return index;
}

void cblArrayRemove(CblMutableArray *array, size_t index) {
    cblBailUnless(array && index < getDataVirtualLength(array->buffer));
    const void **buffer = (const void **)cblDataGetBytePointer(array->buffer);
    const CblArrayContext *context = &array->context;
    if (context->disownCallback) {
        context->disownCallback(array, buffer[index]);
    }
    cblDataReplaceBytes(array->buffer, cblRangeNew(index * sizeof(void *), sizeof(void *)), NULL, 0);
}

void cblArrayEmpty(CblMutableArray *array) {
    cblBailUnless(array);
    const CblArrayContext *context = &array->context;
    size_t length = getDataVirtualLength(array->buffer);
    const void **buffer = (const void **)cblDataGetBytePointer(array->buffer);
    for (size_t i = 0; i < length; ++i) {
        if (context->disownCallback) {
            context->disownCallback(array, buffer[i]);
        }
        buffer[i] = NULL;
    }
    cblDataSetLength(array->buffer, 0);
}

size_t cblArrayAppend(CblMutableArray *array, const void *value) {
    return cblArrayInsert(array, getDataVirtualLength(array->buffer), value);
}
