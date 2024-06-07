#include <ipc/pipes.hh>
#include <lib/util.hh>
#include <sched/scheduler.hh>

PzStatus PzCreatePipe(
    PzProcessObject *readProcess, PzProcessObject *writeProcess,
    PzHandle *read, PzHandle *write, int buffer_cap)
{
    PzHandle oldValue = *read;
    PzAnonymousPipeObject *pipe = nullptr;

    if (buffer_cap > APIPE_BUFSIZE_MAX)
        return STATUS_ABOVE_LIMIT;

    if (!ObCreateUnnamedObject(ObGetObjectDirectory(PZ_OBJECT_APIPE),
        (ObPointer *)&pipe, PZ_OBJECT_APIPE, 0, false))
        return STATUS_FAILED;

    pipe->BufferCapacity = buffer_cap ? buffer_cap : APIPE_BUFSIZE_DEFAULT;
    pipe->Buffer = (u8*)MmVirtualAllocateMemory(nullptr, pipe->BufferCapacity, PAGE_READWRITE, nullptr);
    pipe->ReadPtr = 0;
    pipe->WritePtr = 0;

    PzStatus status;

    if (!pipe->Buffer) {
        status = STATUS_ALLOCATION_FAILED;
        goto fail;
    }

    if (status = PsCreateSemaphore(&pipe->SemaphoreEmpty, PZ_KPROC, nullptr))
        goto fail;

    if (status = PsCreateSemaphore(&pipe->SemaphoreFull, PZ_KPROC, nullptr))
        goto fail;

    if (status = PsCreateMutex(&pipe->MutexRead, PZ_KPROC, nullptr))
        goto fail;

    if (status = PsCreateMutex(&pipe->MutexWrite, PZ_KPROC, nullptr))
        goto fail;

    if (status = PsCreateMutex(&pipe->Mutex, PZ_KPROC, nullptr))
        goto fail;

    status = STATUS_FAILED;

    if (!ObCreateHandle(readProcess, HANDLE_READ, read, pipe))
        goto fail;

    if (!ObCreateHandle(writeProcess, HANDLE_WRITE, write, pipe)) {
        ObCloseProcessHandle(readProcess, *read);
        *read = oldValue;
        goto fail;
    }

    PsReleaseSemaphore(pipe->SemaphoreEmpty, pipe->BufferCapacity);

    return STATUS_SUCCESS;

fail:
    ObDereferenceObject(pipe);
    return status;
}

PzStatus PziReadPipe(PzAnonymousPipeObject *pipe, void *data, u32 bytes)
{
    for (int i = 0; i < bytes; i++) {
        PsWaitForObject(pipe->SemaphoreFull);
        PsWaitForObject(pipe->Mutex);

        ((u8 *)data)[i] = pipe->Buffer[pipe->ReadPtr++];
        pipe->ReadPtr %= pipe->BufferCapacity;

        PsReleaseMutex(pipe->Mutex);
        PsReleaseSemaphore(pipe->SemaphoreEmpty, 1);
    }

    return STATUS_SUCCESS;
}

PzStatus PziWritePipe(PzAnonymousPipeObject *pipe, const void *data, u32 bytes)
{
    for (int i = 0; i < bytes; i++) {
        PsWaitForObject(pipe->SemaphoreEmpty);
        PsWaitForObject(pipe->Mutex);

        pipe->Buffer[pipe->WritePtr++] = ((const u8 *)data)[i];
        pipe->WritePtr %= pipe->BufferCapacity;

        PsReleaseMutex(pipe->Mutex);
        PsReleaseSemaphore(pipe->SemaphoreFull, 1);
    }

    return STATUS_SUCCESS;
}