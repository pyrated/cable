#include <string.h>

#include <cable/core/error.h>
#include <cable/core/string.h>
#include <cable/core/allocator.h>

struct CblError {
    CblConcreteObject isa;
    CblString *domain;
    int code;
    CblString *reason;
};

static void finalizeCallback(CblError *error) {
    cblBailUnless(error);
    cblDisown(error->domain);
    cblDisown(error->reason);
}

static CblString *stringCallback(CblAllocator *alloc, CblError *error) {
    cblReturnUnless(error, NULL);
    CblMutableString *string = cblMutableStringNewCopy(alloc, error->domain);
    cblStringAppendCFormat(string, ": (%d)", error->code);
    if (error->reason) {
        cblStringAppendCString(string, " ");
        cblStringAppend(string, error->reason);
    }
    return string;
}

static int errorCompare(CblError *lhs, CblError *rhs) {
    cblReturnUnless(lhs && rhs, -1);
    int cmp = cblCompare(lhs->domain, rhs->domain);
    if (cmp != 0) {
        return cmp;
    }
    return lhs->code - rhs->code;
}

static CblClass ERROR_CLASS = {
        .name = "CblError",
        .finalizeCallback = (CblObjectFinalizeCallback)finalizeCallback,
        .hashCallback = NULL,
        .compareCallback = (CblObjectCompareCallback)errorCompare,
        .stringCallback = (CblObjectStringCallback)stringCallback,
};

CblClass * const CBL_ERROR_CLASS = &ERROR_CLASS;

CblError *cblErrorNew(CblAllocator *alloc, CblString *domain, int code) {
    return cblErrorNewWithReason(alloc, domain, code, NULL);
}

CblError *cblErrorNewWithErrno(CblAllocator *alloc, int code) {
    CblString *reason = cblStringNewWithCString(alloc, strerror(code));
    cblReturnUnless(reason, NULL);
    CblError *error = cblErrorNewWithReason(alloc, NULL, code, reason);
    cblDisown(reason);
    return error;
}

CblError *cblErrorNewWithReason(CblAllocator *alloc, CblString *domain, int code, CblString *reason) {
    CblError *error = cblAllocatorAllocate(alloc, sizeof(CblError));
    cblReturnUnless(error, NULL);
    cblInitialize(error, alloc, &ERROR_CLASS);
    error->domain = domain ?: cblStringNewWithCString(alloc, "Error");
    error->code = code;
    error->reason = reason;
    return error;
}

CblString *cblErrorGetDomain(CblError *error) {
    cblReturnUnless(error, NULL);
    return error->domain;
}

int cblErrorGetCode(CblError *error) {
    cblReturnUnless(error, 0);
    return error->code;
}

CblString *cblErrorGetReason(CblError *error) {
    cblReturnUnless(error, NULL);
    return error->reason;
}