#pragma once

#include <defs.hh>
#include <core.hh>
#include <sched/scheduler.hh>
#include <syscall/handle.hh>

struct UmHandleAndNameParams {
    PzHandle *Handle;
    const PzString *Name;
};

struct UmCreateProcessParams {
    PzHandle *ProcessHandle;
    const PzProcessCreationParams *Params;
};

struct UmTerminateParams {
    PzHandle Handle;
    int ExitCode;
};

struct UmHandleOnlyParams {
    PzHandle Handle;
};

struct UmSuspendThreadParams {
    PzHandle Handle;
    int *SuspendedCount;
};

struct UmReleaseSemaphoreParams {
    PzHandle Handle;
    int Count;
};

struct UmCreateThreadParams {
    PzHandle *Handle;
    PzHandle ParentProcess;
    int (*StartAddress)(void *parameter);
    void *Parameter;
    int StackSize;
    int Priority;
};

struct UmResetTimerParams {
    PzHandle Handle;
    int Milliseconds;
};

DECL_SYSCALL(UmCreateThread);
DECL_SYSCALL(UmTerminateThread);
DECL_SYSCALL(UmSuspendThread);
DECL_SYSCALL(UmResumeThread);
DECL_SYSCALL(UmWaitForObject);
DECL_SYSCALL(UmReleaseMutex);
DECL_SYSCALL(UmReleaseSemaphore);
DECL_SYSCALL(UmSetEvent);
DECL_SYSCALL(UmResetEvent);
DECL_SYSCALL(UmSchYield);
DECL_SYSCALL(UmCreateMutex);
DECL_SYSCALL(UmCreateTimer);
DECL_SYSCALL(UmCreateSemaphore);
DECL_SYSCALL(UmCreateEvent);
DECL_SYSCALL(UmOpenMutex);
DECL_SYSCALL(UmOpenTimer);
DECL_SYSCALL(UmOpenSemaphore);
DECL_SYSCALL(UmOpenEvent);
DECL_SYSCALL(UmCreateProcess);
DECL_SYSCALL(UmTerminateProcess);
DECL_SYSCALL(UmExitThread);
DECL_SYSCALL(UmResetTimer);