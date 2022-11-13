#pragma once

#include <defs.hh>
#include <x86/context.hh>
#include <core.hh>

#define EXCEPTION_DIVIDE_BY_ZERO      1
#define EXCEPTION_ACCESS_VIOLATION    2
#define EXCEPTION_PRIV_INSTRUCTION    3
#define EXCEPTION_ILLEGAL_INSTRUCTION 4
#define EXCEPTION_FPU_ERROR           5
#define EXCEPTION_OTHER               6

#define EXCEPTION_RECORD_EXT_MAX 8

struct PzExceptionInfo {
    u32 Code;
    u32 Flags;
    uptr InstructionAddress;
    int ExtensionCount;
    uptr Extension[EXCEPTION_RECORD_EXT_MAX];
    PzThreadContext SavedContext;
    uptr StackContext;
};

typedef void (*PzExceptionHandler)(PzExceptionInfo info);

struct PzExceptionRegistration {
#ifdef DEBUGGER_INCLUDE
    DEBUGGER_TARGET_PTR Previous;
    DEBUGGER_TARGET_PTR Handler;
#else
    PzExceptionRegistration *Previous;
    PzExceptionHandler Handler;
#endif
    uptr StackContext;
};

struct PzStackEnvironment {
    u32 Esp, Ebp, Esi, Edi, Ebx, Eip;
};

void ExCallNextHandler(CpuInterruptState *user_state);
void ExContinueExecution();
void ExPushExceptionHandler(PzExceptionHandler handler, uptr stack_context);
void ExPopExceptionHandler();
void ExHandleUserCpuException(CpuInterruptState *state);