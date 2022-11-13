#include <window/console.hh>
#include <window/wnd.hh>
#include <lib/util.hh>
#include <sched/scheduler.hh>
#include <obj/directory.hh>
#include <obj/semaphore.hh>
#include <obj/process.hh>
#include <debug.hh>

PzSpinlock ConHostRegSpinlock;
PzThreadObject *ConHostThread = nullptr;
PzHandle ConHostEvent = INVALID_HANDLE_VALUE;

PzStatus AllocConHostEvent()
{
    if (ConHostEvent != INVALID_HANDLE_VALUE)
        return STATUS_SUCCESS;

    if (PzStatus status = PsCreateEvent(&ConHostEvent, PZ_KPROC, nullptr))
        return status;

    if (PzStatus status = PsResetEvent(ConHostEvent)) {
        ObCloseHandle(ConHostEvent);
        ConHostEvent = INVALID_HANDLE_VALUE;
        return status;
    }

    return STATUS_SUCCESS;
}

#include <ipc/pipes.hh>

PzStatus ConAllocateConsole()
{
    if (PzStatus status = AllocConHostEvent())
        return status;

    if (PzStatus status = PsWaitForObject(ConHostEvent))
        return status;

    PzProcessObject *current = PsGetCurrentProcess();
    PzProcessObject *conhost = ConHostThread->ParentProcess;

    PzHandle stdout_read, stdout_write, stdin_read, stdin_write;

    if (PzStatus status = PzCreatePipe(conhost, current, &stdout_read, &stdout_write, 4096))
        return status;

    if (PzStatus status = PzCreatePipe(current, conhost, &stdin_read, &stdin_write, 4096)) {
        ObCloseProcessHandle(conhost, stdout_read);
        ObCloseHandle(stdout_write);
        return status;
    }

    ObAcquireObject(current);
    ObAcquireObject(ConHostThread);

    current->StandardInput  = stdin_read;
    current->StandardOutput = stdout_write;
    current->StandardError  = stdout_write;

    ConHostThread->MessageQueue.Add(PzMessage {
        CONSOLE_HOST_ALLOC_CONSOLE,
        current->Id, (uptr)stdin_write, (uptr)stdout_read, 0u
    });

    ObReleaseObject(ConHostThread);
    ObReleaseObject(current);

    return STATUS_SUCCESS;
}

PzStatus ConRegisterConsoleHost()
{
    if (PzStatus status = AllocConHostEvent())
        return status;

    PzAcquireSpinlock(&ConHostRegSpinlock);

    if (ConHostThread) {
        PzReleaseSpinlock(&ConHostRegSpinlock);
        return STATUS_ACCESS_DENIED;
    }

    if (PzStatus status = PsSetEvent(ConHostEvent)) {
        PzReleaseSpinlock(&ConHostRegSpinlock);
        return status;
    }

    ObReferenceObject(ConHostThread = PsGetCurrentThread());

    PzReleaseSpinlock(&ConHostRegSpinlock);
    return STATUS_SUCCESS;
}

PzStatus ConUnregisterConsoleHost()
{
    PzAcquireSpinlock(&ConHostRegSpinlock);

    if (PsGetCurrentThread() == ConHostThread) {
        ObDereferenceObject(ConHostThread);
        ConHostThread = nullptr;
        PsResetEvent(ConHostEvent);
        PzReleaseSpinlock(&ConHostRegSpinlock);
        return STATUS_SUCCESS;
    }

    PzReleaseSpinlock(&ConHostRegSpinlock);
    return STATUS_ACCESS_DENIED;
}