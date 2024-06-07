#pragma once

#include <core.hh>
#include <syscall/handle.hh>
#include <except/except.hh>

struct UmExPushExceptionHandlerParams {
    PzExceptionHandler Handler;
    uptr StackContext;
};

DECL_SYSCALL(UmExCallNextHandler);
DECL_SYSCALL(UmExContinueExecution);
DECL_SYSCALL(UmExPushExceptionHandler);
DECL_SYSCALL(UmExPopExceptionHandler);