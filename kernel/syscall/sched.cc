#include <core.hh>
#include <sched/scheduler.hh>
#include <syscall/sched.hh>

DECL_SYSCALL(UmExitThread)
{
    PsExitThread();
    return STATUS_SUCCESS;
}

DECL_SYSCALL(UmCreateThread)
{
    auto prm = (UmCreateThreadParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmCreateThreadParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Handle, sizeof(PzHandle), true))
        return STATUS_INVALID_ARGUMENT;

    return PsCreateThread(
        prm->Handle, true,
        prm->ParentProcess, prm->StartAddress, 
        prm->Parameter, prm->StackSize,
        prm->Priority);
}

DECL_SYSCALL(UmTerminateThread)
{
    auto prm = (UmTerminateParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmTerminateParams), false))
        return STATUS_INVALID_ARGUMENT;

    return PsTerminateThread(prm->Handle, prm->ExitCode);
}

DECL_SYSCALL(UmTerminateProcess)
{
    auto prm = (UmTerminateParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmTerminateParams), false))
        return STATUS_INVALID_ARGUMENT;

    return PsTerminateProcess(prm->Handle, prm->ExitCode);
}

DECL_SYSCALL(UmCreateProcess)
{
    auto prm = (UmCreateProcessParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmCreateProcessParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->ProcessHandle, sizeof(PzHandle), true) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Params, sizeof(PzProcessCreationParams), false) ||
        !PzValidateString(true, prm->Params->ExecutablePath, false) ||
        !PzValidateString(true, prm->Params->ProcessName, false))
        return STATUS_INVALID_ARGUMENT;

    return PsCreateProcess(false, prm->ProcessHandle, prm->Params);
}

#define HANDLE_ONLY(x, y) \
DECL_SYSCALL(x) \
{ \
    auto prm = (UmHandleOnlyParams *)params; \
    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmHandleOnlyParams), false)) \
        return STATUS_INVALID_ARGUMENT; \
    return y(prm->Handle); \
}

#define HANDLE_NAME(x, y, allowNullName) \
DECL_SYSCALL(x) \
{ \
    auto prm = (UmHandleAndNameParams *)params; \
    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmHandleAndNameParams), false) || \
        !MmVirtualProbeMemory(true, (uptr)prm->Handle, sizeof(PzHandle), true) || \
        (!allowNullName || prm->Name) && !PzValidateString(true, prm->Name, false)) \
        return STATUS_INVALID_ARGUMENT; \
    return y(prm->Handle, PZ_CPROC, prm->Name); \
}

#define SUSPEND_RESUME(x) \
DECL_SYSCALL(Um##x##Thread) \
{ \
    auto prm = (UmSuspendThreadParams *)params; \
    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmSuspendThreadParams), false) || \
        !MmVirtualProbeMemory(true, (uptr)prm->SuspendedCount, sizeof(int), true)) \
        return STATUS_INVALID_ARGUMENT; \
    return Ps##x##Thread(prm->Handle, prm->SuspendedCount); \
}

SUSPEND_RESUME(Resume)
SUSPEND_RESUME(Suspend)
HANDLE_ONLY(UmWaitForObject, PsWaitForObject)
HANDLE_ONLY(UmReleaseMutex, PsReleaseMutex)
HANDLE_ONLY(UmSetEvent, PsSetEvent)
HANDLE_ONLY(UmResetEvent, PsResetEvent)

DECL_SYSCALL(UmSchYield)
{
    SchYield();
    return STATUS_SUCCESS;
}

DECL_SYSCALL(UmReleaseSemaphore)
{
    auto prm = (UmReleaseSemaphoreParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmReleaseSemaphoreParams), false))
        return STATUS_INVALID_ARGUMENT;

    return PsReleaseSemaphore(prm->Handle, prm->Count);
}

HANDLE_NAME(UmCreateMutex, PsCreateMutex, true)
HANDLE_NAME(UmCreateTimer, PsCreateTimer, true)
HANDLE_NAME(UmCreateSemaphore, PsCreateSemaphore, true)
HANDLE_NAME(UmCreateEvent, PsCreateEvent, true)
HANDLE_NAME(UmOpenMutex, PsOpenMutex, false)
HANDLE_NAME(UmOpenTimer, PsOpenTimer, false)
HANDLE_NAME(UmOpenSemaphore, PsOpenSemaphore, false)
HANDLE_NAME(UmOpenEvent, PsOpenEvent, false)

DECL_SYSCALL(UmResetTimer)
{
    auto prm = (UmResetTimerParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmResetTimerParams), false))
        return STATUS_INVALID_ARGUMENT;

    return PsResetTimer(prm->Handle, prm->Milliseconds);
}