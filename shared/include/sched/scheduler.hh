#pragma once

#include <obj/thread.hh>
#include <obj/timer.hh>

#define KERNEL_CALL_STACK_SIZE    32768
#define DEFAULT_THREAD_STACK_SIZE 32768
#define PZ_KPROC (PsGetKernelProcess())
#define PZ_CPROC (PsGetCurrentProcess())

struct PzProcessCreationParams
{
    const PzString *ProcessName;
    const PzString *ExecutablePath;
    bool InheritHandles;
    PzHandle StandardInput, StandardOutput, StandardError;
};

extern "C" void HalSwitchContextKernel();
extern "C" void HalSwitchContextUser();

PZ_KERNEL_EXPORT PzProcessObject *PsGetCurrentProcess();
PZ_KERNEL_EXPORT PzThreadObject *PsGetCurrentThread();
PZ_KERNEL_EXPORT PzProcessObject *PsGetKernelProcess();
PZ_KERNEL_EXPORT PzHandle PsOpenCurrentProcess(bool as_user);
PZ_KERNEL_EXPORT PzHandle PsOpenCurrentThread(bool as_user);
PZ_KERNEL_EXPORT PzHandle PsOpenKernelProcess(bool as_user);
PZ_KERNEL_EXPORT PzProcessObject *PsCreateProcessObj(const PzString *name);
PZ_KERNEL_EXPORT PzStatus PsCreateThread(
    PzHandle *handle,
    bool usermode, PzHandle parent_process,
    int (*start)(void *param), void *param,
    int stack_size, int priority);
PZ_KERNEL_EXPORT PzStatus PsTerminateThread(PzHandle thread, int exit_code);
PZ_KERNEL_EXPORT PzStatus PsSuspendThread(PzHandle thread, int *suspended_count);
PZ_KERNEL_EXPORT PzStatus PsResumeThread(PzHandle thread, int *suspended_count);
PZ_KERNEL_EXPORT PzStatus PsWaitForObject(PzHandle object);
PZ_KERNEL_EXPORT PzStatus PsReleaseMutex(PzHandle mutex);
PZ_KERNEL_EXPORT PzStatus PsReleaseSemaphore(PzHandle semaphore, int count);
PZ_KERNEL_EXPORT PzStatus PsSetEvent(PzHandle event);
PZ_KERNEL_EXPORT PzStatus PsResetEvent(PzHandle event);
PZ_KERNEL_EXPORT PzStatus SchYield();
PZ_KERNEL_EXPORT PzStatus PsResetTimer(PzHandle handle, int ms);
PZ_KERNEL_EXPORT PzStatus PsCreateMutex(PzHandle *handle, PzProcessObject *process, const PzString *name);
PZ_KERNEL_EXPORT PzStatus PsCreateTimer(PzHandle *handle, PzProcessObject *process, const PzString *name);
PZ_KERNEL_EXPORT PzStatus PsCreateSemaphore(PzHandle *handle, PzProcessObject *process, const PzString *name);
PZ_KERNEL_EXPORT PzStatus PsCreateEvent(PzHandle *handle, PzProcessObject *process, const PzString *name);
PZ_KERNEL_EXPORT PzStatus PsOpenMutex(PzHandle *handle, PzProcessObject *process, const PzString *name);
PZ_KERNEL_EXPORT PzStatus PsOpenTimer(PzHandle *handle, PzProcessObject *process, const PzString *name);
PZ_KERNEL_EXPORT PzStatus PsOpenSemaphore(PzHandle *handle, PzProcessObject *process, const PzString *name);
PZ_KERNEL_EXPORT PzStatus PsOpenEvent(PzHandle *handle, PzProcessObject *process, const PzString *name);
PZ_KERNEL_EXPORT PzStatus PsCreateProcess(
    bool as_kernel, PzHandle *process_handle, const PzProcessCreationParams *params);
PZ_KERNEL_EXPORT PzStatus PsTerminateProcess(PzHandle process_handle, int exit_code);
PZ_KERNEL_EXPORT void PsSetLogScheduler(bool log);
void PsExitThread();
void PsCreateKernelProcess();
void SchInitializeScheduler(int (*init_thread)(void *param), void *init_param);
bool PiActivateTimer(PzTimerObject *timer);
bool PiDeactivateTimer(PzTimerObject *timer);