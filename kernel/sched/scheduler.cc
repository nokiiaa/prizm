#include <sched/scheduler.hh>
#include <obj/process.hh>
#include <lib/list.hh>
#include <lib/util.hh>
#include <core.hh>
#include <debug.hh>
#include <x86/gdt.hh>
#include <x86/apic.hh>
#include <x86/i8259a.hh>
#include <obj/timer.hh>
#include <obj/event.hh>
#include <obj/semaphore.hh>
#include <obj/mutex.hh>
#include <id/generator.hh>
#include <processor.hh>

extern "C" void HalFloatingPointSave(void *destination);

static u8 InitThreadFxState[512];

#define CURRENT_QUEUE PzGetCurrentProcessor()->Queue
#define CURRENT_THREAD CURRENT_QUEUE.ThreadList.First->Value

static bool SchStartup = true;
static PzProcessObject *SystemKernelProcess;

PzThreadObject *PsGetCurrentThread()
{
    return CURRENT_QUEUE.ThreadList.Length ? CURRENT_THREAD : nullptr;
}

PzHandle PsOpenCurrentProcess(bool as_user)
{
    PzHandle handle = INVALID_HANDLE_VALUE;
    ObCreateHandle(as_user ? PZ_CPROC : PZ_KPROC, 0, &handle, PZ_CPROC);
    return handle;
}

PzHandle PsOpenCurrentThread(bool as_user)
{
    PzHandle handle = INVALID_HANDLE_VALUE;
    ObCreateHandle(as_user ? PZ_CPROC : PZ_KPROC, 0, &handle, PZ_CPROC);
    return handle;
}

PzHandle PsOpenKernelProcess(bool as_user)
{
    PzHandle handle = INVALID_HANDLE_VALUE;
    ObCreateHandle(as_user ? PZ_CPROC : PZ_KPROC, 0, &handle, PZ_CPROC);
    return handle;
}

void SchKernelThreadInit(PzThreadObject *thread, int (*start)(void *param), void *param)
{
    DbgPrintStr(
        "[SchThreadInit] Thread %p id %i successfully started, start=%p, param=%p, &thread=%p\r\n",
        thread, thread->Id, start, param, &thread);

    int exit_code = start(param);

    DbgPrintStr("[SchThreadInit] Thread id %i is done\r\n", thread->Id);

    int old = PzRaiseIrql(DISPATCH_LEVEL);
    CURRENT_QUEUE.NumberOfActiveThreads--;
    thread->Flags |= THREAD_TERMINATING;
    thread->ExitCode = exit_code;
    PzLowerIrql(old);

    SchYield();
}

void PsExitThread()
{
    int old = PzRaiseIrql(DISPATCH_LEVEL);
    CURRENT_QUEUE.NumberOfActiveThreads--;
    PsGetCurrentThread()->Flags |= THREAD_TERMINATING;
    PzLowerIrql(old);

    SchYield();
}

PzStatus PiTerminateThread(bool dereference, PzThreadObject *thread, int exit_code)
{
    ObAcquireObject(thread);

    CURRENT_QUEUE.NumberOfActiveThreads--;
    thread->Flags |= THREAD_TERMINATING;
    thread->ExitCode = exit_code;
    thread->Signaled = true;

    ObReleaseObject(thread);

    if (dereference)
        ObDereferenceObject(thread);

    if (thread == PsGetCurrentThread())
        SchYield();

    return STATUS_SUCCESS;
}

PzStatus PsTerminateThread(PzHandle thread, int exit_code)
{
    PzThreadObject *object;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_THREAD, nullptr, thread, (ObPointer *)&object))
        return STATUS_INVALID_ARGUMENT;

    return PiTerminateThread(true, object, exit_code);
}

PzStatus PsSuspendThread(PzHandle thread, int *suspended_count)
{
    PzThreadObject *object;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_THREAD, nullptr, thread, (ObPointer *)&object))
        return STATUS_INVALID_ARGUMENT;

    ObAcquireObject(object);
    CURRENT_QUEUE.NumberOfActiveThreads--;
    object->Flags |= THREAD_SUSPENDED;
    *suspended_count = object->SuspendedCount++;
    ObDereferenceObject(object);
    ObReleaseObject(object);

    if (object == PsGetCurrentThread())
        SchYield();

    return STATUS_SUCCESS;
}

PzStatus PsResumeThread(PzHandle thread, int *suspended_count)
{
    PzThreadObject *object;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_THREAD, nullptr, thread, (ObPointer *)&object))
        return STATUS_INVALID_ARGUMENT;

    ObAcquireObject(object);
    CURRENT_QUEUE.NumberOfActiveThreads++;
    object->Flags &= ~THREAD_SUSPENDED;
    *suspended_count = object->SuspendedCount--;
    ObDereferenceObject(object);
    ObReleaseObject(object);

    if (object == PsGetCurrentThread())
        SchYield();

    return STATUS_SUCCESS;
}

void PiDecrementSemaphore(PzSemaphoreObject *semaphore)
{
    ObAcquireObject(semaphore);
    /* Semaphores are signaled if their count is higher than 0, and non-signaled otherwise */
    semaphore->Signaled = --semaphore->Count > 0;
    ObReleaseObject(semaphore);
}

PzStatus PsWaitForObject(PzHandle obj_handle)
{
    PzThreadObject *object = PsGetCurrentThread();

    union {
        ObPointer lock_obj;
        PzMutexObject *mutex;
        PzSemaphoreObject *semaphore;
        PzTimerObject *timer;
        PzEventObject *event;
        PzProcessObject *process;
        PzThreadObject *thread;
    };

    if (!ObReferenceObjectByHandle(PZ_OBJECT_ANY, nullptr, obj_handle, (ObPointer *)&lock_obj))
        return STATUS_INVALID_HANDLE;

    bool signaled;

    ObAcquireObject(lock_obj);
    int type = ObGetObjectType(lock_obj);

    switch (type)
    {
    case PZ_OBJECT_MUTEX:
        signaled = mutex->Signaled;
        break;

    case PZ_OBJECT_SEMAPHORE:
        signaled = semaphore->Signaled;
        break;

    case PZ_OBJECT_TIMER:
        signaled = timer->Signaled;
        break;

    case PZ_OBJECT_EVENT:
        signaled = event->Signaled;
        break;

    case PZ_OBJECT_PROCESS:
        signaled = process->Signaled;
        break;

    case PZ_OBJECT_THREAD:
        signaled = thread->Signaled;
        break;

    default:
        ObReleaseObject(lock_obj);
        ObDereferenceObject(lock_obj);
        return STATUS_INVALID_ARGUMENT;
    }

    if (signaled) {
        /* Semaphores are signaled if their count is higher than 0, and non-signaled otherwise */
        if (type == PZ_OBJECT_SEMAPHORE)
            semaphore->Signaled = --semaphore->Count > 0;
        else if (type == PZ_OBJECT_MUTEX)
            mutex->Signaled = false;

        ObReleaseObject(lock_obj);
        ObDereferenceObject(lock_obj);
        return STATUS_SUCCESS;
    }

    ObAcquireObject(object);
    CURRENT_QUEUE.NumberOfActiveThreads--;
    object->Flags |= THREAD_WAITING;
    object->WaitObject = lock_obj;
    ObReleaseObject(object);

    ObReleaseObject(lock_obj);
    SchYield();

    ObAcquireObject(lock_obj);

    if (type == PZ_OBJECT_SEMAPHORE)
        semaphore->Signaled = --semaphore->Count > 0;
    else if (type == PZ_OBJECT_MUTEX)
        mutex->Signaled = false;

    ObReleaseObject(lock_obj);

    return STATUS_SUCCESS;
}

PzStatus PsReleaseMutex(PzHandle mutex)
{
    PzMutexObject *mutex_obj;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_MUTEX, nullptr, mutex, (ObPointer *)&mutex_obj))
        return STATUS_INVALID_ARGUMENT;

    //DbgPrintStr("PsReleaseMutex\r\n");
    mutex_obj->Signaled = true;

    ObDereferenceObject(mutex_obj);
    return STATUS_SUCCESS;
}

PzStatus PsReleaseSemaphore(PzHandle semaphore, int count)
{
    PzSemaphoreObject *sema_obj;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_SEMAPHORE, nullptr, semaphore, (ObPointer *)&sema_obj))
        return STATUS_INVALID_ARGUMENT;

    ObAcquireObject(sema_obj);
    sema_obj->Count += count;
    sema_obj->Signaled = true;
    ObReleaseObject(sema_obj);

    ObDereferenceObject(sema_obj);
    return STATUS_SUCCESS;
}

PzStatus PsSetEvent(PzHandle event)
{
    PzEventObject *event_obj;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_EVENT, nullptr, event, (ObPointer *)&event_obj))
        return STATUS_INVALID_ARGUMENT;

    event_obj->Signaled = true;
    ObDereferenceObject(event_obj);

    return STATUS_SUCCESS;
}

PzStatus PsResetEvent(PzHandle event)
{
    PzEventObject *event_obj;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_EVENT, nullptr, event, (ObPointer *)&event_obj))
        return STATUS_INVALID_ARGUMENT;

    event_obj->Signaled = false;
    ObDereferenceObject(event_obj);

    return STATUS_SUCCESS;
}

PzProcessObject *PsGetCurrentProcess()
{
    PzThreadObject *thread = PsGetCurrentThread();

    if (!thread)
        return nullptr;

    return thread->ParentProcess;
}

PzProcessObject *PsCreateProcessObj(const PzString *name)
{
    PzProcessObject *process;

    if (!ObCreateNamedObject(ObGetObjectDirectory(PZ_OBJECT_PROCESS),
        (ObPointer *)&process, PZ_OBJECT_PROCESS, PzDuplicateString(name), 0, false))
        return nullptr;

    process->Id = IdGenerateUniqueId(NAMESPACE_THREAD_PROCESS);

    return process;
}

#include <io/manager.hh>
#include <ldr/peldr.hh>
#include <obj/process.hh>

PzStatus PsCreateProcess(
    bool as_kernel,
    PzHandle *process_handle,
    const PzProcessCreationParams *params)
{
    const PzString *exec_path = params->ExecutablePath;
    PzString *p_name = PzDuplicateString(params->ProcessName);
    PzProcessObject *process = PsCreateProcessObj(params->ProcessName);

    if (!process)
        return STATUS_INVALID_ARGUMENT;

    process->IsUserMode = true;

    if (params->InheritHandles) {
        /* Pass on inheritable handles of the current process */
        ENUM_LIST(h, PsGetCurrentProcess()->HandleTable) {
            if (h->Value.Flags & HANDLE_INHERIT) {
                ObReferenceObject(h->Value.ObjectPointer);

                process->HandleTable.Add(
                    PzHandleTableEntry {
                        h->Value.Id,
                        h->Value.Flags,
                        h->Value.ObjectPointer
                    });
            }
        }
    }

    /* Validate standard I/O handles and inherit them into the process */
    PzHandleTableEntry entry;

#define CHECK_STD_HANDLE(name, access) \
    process->name = params->name; \
    if (params->name != INVALID_HANDLE_VALUE) { \
        ObGetHandleInformation(params->name, &entry); \
        if (!(entry.Flags & HANDLE_INHERIT) || !(entry.Flags & access)) { \
                ObDereferenceObject(process); \
                return STATUS_INVALID_HANDLE; \
        } \
        process->HandleTable.Add(PzHandleTableEntry{ entry.Id, entry.Flags, entry.ObjectPointer }); \
    }

    CHECK_STD_HANDLE(StandardInput,  HANDLE_READ)
    CHECK_STD_HANDLE(StandardOutput, HANDLE_WRITE)
    CHECK_STD_HANDLE(StandardError,  HANDLE_WRITE)
#undef CHECK_STD_HANDLE

    ObCreateHandle(as_kernel ? PZ_KPROC : nullptr, 0, process_handle, process);
    PzHandle pzdll_mod;
    PzString pzdll_path = PZ_CONST_STRING("SystemDir\\pzdll.dll");
    PzString pzdll_name = PZ_CONST_STRING("pzdll.dll");

    /* Map pzdll.dll into newly created process */
    if (PzStatus ldr = LdrLoadImageFile(*process_handle, &pzdll_path, &pzdll_name, &pzdll_mod)) {
        ObCloseHandle(*process_handle);
        ObDereferenceObject(process);
        return ldr;
    }

    process->UserThreadExit = LdrGetSymbolAddress(pzdll_mod, "PzThreadExit");

    /* Map the main module file into newly created process */
    PzHandle mod_handle;

    if (PzStatus ldr = LdrLoadImageFile(*process_handle, exec_path,
        p_name, &mod_handle)) {
        ObCloseHandle(*process_handle);
        ObDereferenceObject(process);
        return ldr;
    }

    /* Create process's main thread at the executable's entry point */
    PzModuleObject *mod;
    ObReferenceObjectByHandle(PZ_OBJECT_MODULE, nullptr, mod_handle, (ObPointer*)&mod);

    PzHandle thread_handle;

    if (PzStatus ct = PsCreateThread(&thread_handle, true, *process_handle,
        mod->EntryPointAddress, mod->BaseAddress, 4096, THREAD_PRIORITY_CRITICAL)) {
        ObCloseHandle(*process_handle);
        ObDereferenceObject(process);
        return ct;
    }

    ObDereferenceObject(mod);
    ObDereferenceObject(process);
    ObCloseHandle(mod_handle);
    return STATUS_SUCCESS;
}

PzStatus PsTerminateProcess(PzHandle process_handle, int exit_code)
{
    PzProcessObject *process_obj;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_PROCESS, nullptr,
        process_handle, (ObPointer *)&process_obj))
        return STATUS_INVALID_ARGUMENT;

    ObAcquireObject(process_obj);
    PzAcquireSpinlock(&process_obj->Threads.Spinlock);
    bool current_thread = false;

    ENUM_LIST(thread, process_obj->Threads) {
        if (thread->Value == CURRENT_THREAD)
            current_thread = true;
        else
            PiTerminateThread(false, thread->Value, exit_code);
    }

    PzReleaseSpinlock(&process_obj->Threads.Spinlock);
    process_obj->ExitCode = exit_code;
    process_obj->Signaled = true;
    ObReleaseObject(process_obj);
    ObDereferenceObject(process_obj);

    if (current_thread) {
        // When you think about it at 1 am, this line of code is so sad...
        PiTerminateThread(false, PsGetCurrentThread(), exit_code);
    }

    return STATUS_SUCCESS;
}

PzStatus CreateNamedObjectAndHandle(
    int obj_type, PzHandle *handle,
    PzProcessObject *process, const PzString *name)
{
    ObPointer obj;

    if (name ?
        !ObCreateNamedObject(ObGetObjectDirectory(obj_type),
        (ObPointer *)&obj, obj_type, name, 0, false) :
        !ObCreateUnnamedObject(ObGetObjectDirectory(obj_type),
            (ObPointer *)&obj, obj_type, 0, false))
        return STATUS_FAILED;

    if (!ObCreateHandle(process, 0, handle, obj)) {
        ObDereferenceObject(obj);
        return STATUS_FAILED;
    }

    return STATUS_SUCCESS;
}

PzStatus CreateHandleToNamedObject(
    int obj_type, PzHandle *handle,
    PzProcessObject *process, const PzString *name)
{
    ObPointer object;

    if (ObReferenceObjectByName(obj_type, ObGetObjectDirectory(obj_type), name, &object, false, nullptr)) {
        if (ObCreateHandle(process, 0, handle, object))
            return STATUS_SUCCESS;
        else {
            ObDereferenceObject(object);
            return STATUS_FAILED;
        }
    }

    return STATUS_PATH_NOT_FOUND;
}

bool PiActivateTimer(PzTimerObject *timer)
{
    auto *queue = &CURRENT_QUEUE;
    PzAcquireSpinlock(&queue->ActiveTimers.Spinlock);
    queue->ActiveTimers.Add(timer);
    PzReleaseSpinlock(&queue->ActiveTimers.Spinlock);
    return true;
}

bool PiDeactivateTimer(PzTimerObject *timer)
{
    auto *queue = &CURRENT_QUEUE;
    PzAcquireSpinlock(&queue->ActiveTimers.Spinlock);
    queue->ActiveTimers.RemoveValue(timer);
    PzReleaseSpinlock(&queue->ActiveTimers.Spinlock);
    return true;
}

PzStatus PsResetTimer(PzHandle handle, int ms)
{
    PzTimerObject *timer;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_TIMER, nullptr,
        handle, (ObPointer *)&timer))
        return STATUS_INVALID_ARGUMENT;

    ObAcquireObject(timer);
    timer->TimeLeft = ms;
    timer->Signaled = false;
    ObReleaseObject(timer);

    ObDereferenceObject(timer);
    return STATUS_SUCCESS;
}

PzStatus PsCreateMutex(PzHandle *handle, PzProcessObject *process, const PzString *name)
{
    return CreateNamedObjectAndHandle(PZ_OBJECT_MUTEX, handle, process, name);
}

PzStatus PsCreateTimer(PzHandle *handle, PzProcessObject *process, const PzString *name)
{
    return CreateNamedObjectAndHandle(PZ_OBJECT_TIMER, handle, process, name);
}

PzStatus PsCreateSemaphore(PzHandle *handle, PzProcessObject *process, const PzString *name)
{
    return CreateNamedObjectAndHandle(PZ_OBJECT_SEMAPHORE, handle, process, name);
}

PzStatus PsCreateEvent(PzHandle *handle, PzProcessObject *process, const PzString *name)
{
    return CreateNamedObjectAndHandle(PZ_OBJECT_EVENT, handle, process, name);
}

PzStatus PsOpenMutex(PzHandle *handle, PzProcessObject *process, const PzString *name)
{
    return CreateHandleToNamedObject(PZ_OBJECT_MUTEX, handle, process, name);
}

PzStatus PsOpenTimer(PzHandle *handle, PzProcessObject *process, const PzString *name)
{
    return CreateHandleToNamedObject(PZ_OBJECT_TIMER, handle, process, name);
}

PzStatus PsOpenSemaphore(PzHandle *handle, PzProcessObject *process, const PzString *name)
{
    return CreateHandleToNamedObject(PZ_OBJECT_SEMAPHORE, handle, process, name);
}

PzStatus PsOpenEvent(PzHandle *handle, PzProcessObject *process, const PzString *name)
{
    return CreateHandleToNamedObject(PZ_OBJECT_EVENT, handle, process, name);
}

PzProcessObject *PsGetKernelProcess()
{
    return SystemKernelProcess;
}

void AllocateQuantaForThread(PzThreadObject *thread)
{
    switch (thread->Priority) {
    case THREAD_PRIORITY_IDLE:
        thread->RemainingQuanta = 1;
        break;

    case THREAD_PRIORITY_LOW:
        thread->RemainingQuanta = 1;/*2;*/
        break;

    case THREAD_PRIORITY_NORMAL:
        thread->RemainingQuanta = 1;/*4;*/
        break;

    case THREAD_PRIORITY_HIGH:
        thread->RemainingQuanta = 1;
        break;

    case THREAD_PRIORITY_CRITICAL:
        thread->RemainingQuanta = 1;
        break;
    }
}

PzStatus PsCreateThread(
    PzHandle *handle,
    bool usermode, PzHandle parent_process,
    int (*start)(void *param), void *param,
    int stack_size, int priority)
{
    PzProcessObject *process_obj;

    if (stack_size == 0)
        stack_size = DEFAULT_THREAD_STACK_SIZE;

    if (usermode) {
        if (!parent_process)
            ObReferenceObject(process_obj = PsGetCurrentProcess());
        else if (!ObReferenceObjectByHandle(
            PZ_OBJECT_PROCESS, nullptr, parent_process, (ObPointer *)&process_obj))
            return STATUS_FAILED;
    }
    else
        ObReferenceObject(process_obj = PZ_KPROC);

    u8 *fx_region = (u8 *)MmVirtualAllocateMemory(nullptr, 512, PAGE_READWRITE, nullptr);

    if (!fx_region) {
        ObDereferenceObject(process_obj);
        return STATUS_ALLOCATION_FAILED;
    }

    void *kstack = usermode ?
        MmVirtualAllocateMemory(
            nullptr, KERNEL_CALL_STACK_SIZE, PAGE_READWRITE, nullptr) :
        nullptr;

    if (usermode && !kstack) {
        MmVirtualFreeMemory(fx_region, 512);
        ObDereferenceObject(process_obj);
        return STATUS_ALLOCATION_FAILED;
    }

    void *ustack =
        usermode ?
        MmVirtualAllocateUserMemory(parent_process, nullptr, stack_size, PAGE_READWRITE) :
        MmVirtualAllocateMemory(nullptr, stack_size, PAGE_READWRITE, nullptr);

    if (!ustack) {
        if (usermode)
            MmVirtualFreeMemory(kstack, KERNEL_CALL_STACK_SIZE);

        MmVirtualFreeMemory(fx_region, 512);
        ObDereferenceObject(process_obj);
        return STATUS_ALLOCATION_FAILED;
    }

    PzThreadObject *thread;
    if (!ObCreateUnnamedObject(ObGetObjectDirectory(PZ_OBJECT_THREAD),
        (ObPointer *)&thread, PZ_OBJECT_THREAD, 0, false)) {

        if (usermode)
            MmVirtualFreeUserMemory(parent_process, ustack, 0);
        else
            MmVirtualFreeMemory(ustack, stack_size);

        MmVirtualFreeMemory(fx_region, 512);

        if (usermode)
            MmVirtualFreeMemory(kstack, KERNEL_CALL_STACK_SIZE);

        ObDereferenceObject(process_obj);
        return STATUS_FAILED;
    }

    thread->ControlBlock.Eax = 0;
    thread->ControlBlock.Ecx = 0;
    thread->ControlBlock.Edx = 0;
    thread->ControlBlock.Ebx = 0;
    thread->ControlBlock.Esi = 0;
    thread->ControlBlock.Edi = 0;
    thread->ControlBlock.Ebp = 0;

    thread->UserStack = (void *)(thread->ControlBlock.Esp = uptr(ustack));
    thread->UserStackSize = stack_size;

    /* Allocate stack used for kernel interrupts and syscalls */
    if (usermode) {
        thread->KernelStack = kstack;
        thread->KernelStackSize = KERNEL_CALL_STACK_SIZE;
    }

    /* Push three arguments on the stack (cdecl)
       for ThreadInit and a dummy return address (though the function will never return) */
    thread->ControlBlock.Esp = (uptr)thread->UserStack + stack_size;

    if (usermode) {
        PzPushAddressSpace(process_obj->Cr3);
        thread->ControlBlock.Esp -= 4; *(u32 *)thread->ControlBlock.Esp = (u32)param;
        thread->ControlBlock.Esp -= 4; *(u32 *)thread->ControlBlock.Esp = process_obj->UserThreadExit;
        thread->ControlBlock.Eip = u32(start);
        PzPopAddressSpace();
    }
    else {
        thread->ControlBlock.Esp -= 4; *(u32 *)thread->ControlBlock.Esp = (u32)param;
        thread->ControlBlock.Esp -= 4; *(u32 *)thread->ControlBlock.Esp = (u32)start;
        thread->ControlBlock.Esp -= 4; *(u32 *)thread->ControlBlock.Esp = (u32)thread;
        thread->ControlBlock.Esp -= 4; *(u32 *)thread->ControlBlock.Esp = 0;
        thread->ControlBlock.Eip = u32(SchKernelThreadInit);
    }

    thread->ControlBlock.Eflags = 1 << 9 | 1 << 1;
    thread->SuspendedCount = 0;
    thread->Flags = 0;
    thread->Id = IdGenerateUniqueId(NAMESPACE_THREAD_PROCESS);
    thread->IsUserMode = usermode;
    thread->Priority = priority;

    HalFloatingPointSave(
        thread->ControlBlock.FxSaveRegion = fx_region);

    AllocateQuantaForThread(thread);

    static u16 KernelModeSegments[] = {
        0x08 | 0, 0x10 | 0, 0x10 | 0, 0x18 | 0, 0x10 | 0, 0x10 | 0
    },
    UserModeSegments[] = {
        0x20 | 3, 0x28 | 3, 0x28 | 3, 0x30 | 3, 0x28 | 3, 0x28 | 3
    };

    u16 *segment_init = usermode ? UserModeSegments : KernelModeSegments;
    thread->ControlBlock.Cs = segment_init[0];
    thread->ControlBlock.Ds = segment_init[1];
    thread->ControlBlock.Es = segment_init[2];
    thread->ControlBlock.Fs = segment_init[3];
    thread->ControlBlock.Gs = segment_init[4];
    thread->ControlBlock.Ss = segment_init[5];

    PzEnterCriticalRegion();

    auto *thread_node = thread->SchThreadListNode = CURRENT_QUEUE.ThreadList.Add(thread);

    if (!thread_node) {
        ObDereferenceObject(thread);
        ObDereferenceObject(process_obj);
        PzLeaveCriticalRegion();
        return STATUS_FAILED;
    }

    thread->ParentProcess = process_obj;

    DbgPrintStr("[SchCreateThread] Thread (%p) created @ %p.\r\n", thread, start);

    if (!ObCreateHandle(!usermode ? PZ_KPROC : PZ_CPROC,
        0, handle, thread)) {
        ObDereferenceObject(thread);
        ObDereferenceObject(process_obj);
        PzLeaveCriticalRegion();
        return STATUS_FAILED;
    }

    ObDereferenceObject(thread);
    ObReferenceObject(thread);
    process_obj->Threads.Add(thread);
    CURRENT_QUEUE.NumberOfActiveThreads++;
    PzLeaveCriticalRegion();

    return STATUS_SUCCESS;
}

void PsCreateKernelProcess()
{
    const PzString KernelProcessName = PZ_CONST_STRING("KernelIdle");
    SystemKernelProcess = PsCreateProcessObj(&KernelProcessName);
}

void SchSwitchTask(CpuInterruptState *state);

extern "C" void HalLoadFs(int value);

int PsIdleThread(void *param)
{
    for (;;);
}

void SchInitializeScheduler(int (*init_thread)(void *param), void *init_param)
{
    PsCreateKernelProcess();
    SchStartup = true;
    PzHandle handle;

    HalFloatingPointSave(InitThreadFxState);

    PsCreateThread(&handle, false, 0, PsIdleThread, 0, 0, THREAD_PRIORITY_IDLE);
    PsCreateThread(&handle, false, 0, init_thread, init_param, 0, THREAD_PRIORITY_IDLE);

    HalGdtSetDataSegment(3, (u32)&CURRENT_QUEUE.FsSpace, sizeof(PzThreadContext), 0, 0);

    HalLoadFs(0x18);

    PzInstallIrqHandler(0, SchSwitchTask);
    /*HalApicStartTimer(0, 10);
    HalApicMapIoApic();*/

    // Set PIT frequency
    u16 freq = 7159092 / (6 * 100);
    HalPortOut8(0x43, 0x34); /* Channel 0, rate generator */
    HalPortOut8(0x40, freq & 0xFF);
    HalPortOut8(0x40, freq >> 8);

    DbgSchedulerEnabled = true;
    PzEnableInterrupts();
}

PzStatus SchYield()
{
    CURRENT_QUEUE.SoftwareInducedTick = true;
#ifdef __GNUC__
    asm("int $0x20");
#else
    #error TODO: msvc inline assembly for this function
#endif
    return STATUS_SUCCESS;
}

bool LogScheduler = false;

void PsSetLogScheduler(bool log)
{
    int old = PzRaiseIrql(DISPATCH_LEVEL);
    LogScheduler = log;
    PzLowerIrql(old);
}

void SchSwitchTask(CpuInterruptState *state)
{
    if (!CURRENT_QUEUE.SoftwareInducedTick) {
        ENUM_LIST(tn, CURRENT_QUEUE.ActiveTimers) {
            if ((tn->Value->TimeLeft -= 10) <= 0) {
                tn->Value->TimeLeft = 0;
                tn->Value->Signaled = true;
            }
        }
    }

    if (!CURRENT_THREAD->WaitObject && CURRENT_THREAD->RemainingQuanta != 0)
        if (--CURRENT_THREAD->RemainingQuanta > 0)
            return;

    if (LogScheduler)
        DbgPrintStr("\r\nScheduler tick. cs=0x%04x Number of active threads=%i\r\n",
            state->Cs, CURRENT_QUEUE.NumberOfActiveThreads);

    if (!SchStartup) {
        bool terminated = false, first = true;

    thread_enum:
        ENUM_LIST(tn, CURRENT_QUEUE.ThreadList) {
            if (tn->Value->Flags & THREAD_TERMINATING) {
                auto &threads = tn->Value->ParentProcess->Threads;

                if (!PzIsSpinlockAcquired(OBJECT_HEADER(tn->Value)->RefLock) &&
                    !PzIsSpinlockAcquired(threads.Spinlock) &&
                    !PzIsSpinlockAcquired(CURRENT_QUEUE.ThreadList.Spinlock)) {
                    CURRENT_QUEUE.ThreadList.Remove(tn);

                    ObDereferenceObject(tn->Value);
                    threads.RemoveValue(tn->Value);

                    if (first) {
                        terminated = true;
                        first = false;
                        goto thread_enum;
                    }
                    else {
                        tn = tn->Previous;
                        continue;
                    }
                }
                else if (first) {
                    terminated = true;
                    first = false;
                    goto thread_enum;
                }
            }

            bool signaled = false;
            union {
                ObPointer wait_obj;
                PzMutexObject *mutex;
                PzSemaphoreObject *semaphore;
                PzTimerObject *timer;
                PzEventObject *event;
                PzProcessObject *process;
                PzThreadObject *thread;
            };

            if (wait_obj = tn->Value->WaitObject) {
                int type = ObGetObjectType(wait_obj);
                switch (type) {
                case PZ_OBJECT_TIMER:
                    signaled = timer->Signaled;
                    break;

                case PZ_OBJECT_SEMAPHORE:
                    signaled = semaphore->Signaled;
                    break;

                case PZ_OBJECT_MUTEX:
                    signaled = mutex->Signaled;
                    break;

                case PZ_OBJECT_EVENT:
                    signaled = event->Signaled;
                    break;

                case PZ_OBJECT_PROCESS:
                    signaled = process->Signaled;
                    break;

                case PZ_OBJECT_THREAD:
                    signaled = thread->Signaled;
                    break;
                }

                if (signaled) {
                    ObDereferenceObject(wait_obj);
                    tn->Value->WaitObject = nullptr;
                    tn->Value->Flags &= ~THREAD_WAITING;
                    CURRENT_QUEUE.NumberOfActiveThreads++;
                }
                else
                    tn->Value->Flags |= THREAD_WAITING;
            }

            first = false;
        }

        if (!terminated) {
            PzThreadObject *current_thread = CURRENT_THREAD;

            current_thread->ControlBlock.Eax = state->Eax;
            current_thread->ControlBlock.Ecx = state->Ecx;
            current_thread->ControlBlock.Edx = state->Edx;
            current_thread->ControlBlock.Ebx = state->Ebx;
            current_thread->ControlBlock.Ebp = state->Ebp;
            current_thread->ControlBlock.Esi = state->Esi;
            current_thread->ControlBlock.Edi = state->Edi;
            current_thread->ControlBlock.Eip = state->Eip;
            current_thread->ControlBlock.Eflags = state->Eflags | 1 << 9 | 1 << 1;
            current_thread->ControlBlock.Cs = state->Cs;
            current_thread->ControlBlock.Ds = state->Ds;

            /* Determine whether a far stack pointer has been pushed by the CPU */
            if (state->Cs == 0x8) {
                /* Add 20 because of cs:eip, eflags, interrupt number
                   and error code being pushed before pusha executes */
                current_thread->ControlBlock.Esp = state->Esp + 20;
                current_thread->ControlBlock.Ss = 0x10;
            }
            else {
                current_thread->ControlBlock.Esp = state->EspU;
                current_thread->ControlBlock.Ss = state->SsU;
            }

            current_thread->ControlBlock.Es = state->Es;
            current_thread->ControlBlock.Fs = state->Fs;
            current_thread->ControlBlock.Gs = state->Gs;
            HalFloatingPointSave(current_thread->ControlBlock.FxSaveRegion);
        }

        if (CURRENT_QUEUE.NumberOfActiveThreads > 0) {
            if (!terminated) {
                AllocateQuantaForThread(CURRENT_THREAD);
                CURRENT_QUEUE.ThreadList.RotateLeft();
            }

            while (!THREAD_WORKING(CURRENT_THREAD->Flags))
                CURRENT_QUEUE.ThreadList.RotateLeft();
        }
    }

    SchStartup = false;

    if (!CURRENT_QUEUE.SoftwareInducedTick && state->InterruptNumber != 16)
        Hal8259ASendEoi(false);

    PzThreadObject *thread = CURRENT_THREAD;
    CURRENT_QUEUE.SoftwareInducedTick = false;
    CURRENT_QUEUE.FsSpace = thread->ControlBlock;

    if (thread->IsUserMode) {
        HalTss.Esp0 = u32(thread->KernelStack) + KERNEL_CALL_STACK_SIZE;
        HalTss.Ss0 = 0x10;
        HalTss.IopbOffset = sizeof(HalTss);
        HalSwitchPageTable(thread->ParentProcess->Cr3);
    }

    /* Determine whether a far stack switch is required */
    if (thread->ControlBlock.Cs == 0x8)
        HalSwitchContextKernel();
    else
        HalSwitchContextUser();
}