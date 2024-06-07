#include <syscall/handle.hh>
#include <syscall/sched.hh>
#include <syscall/mem.hh>
#include <syscall/file.hh>
#include <syscall/gfx.hh>
#include <syscall/except.hh>
#include <syscall/message.hh>
#include <syscall/window.hh>
#include <processor.hh>
#include <serial.hh>

#define SYSCALL_COUNT 64

PzStatus (*PrizmSyscallTable[SYSCALL_COUNT])(CpuInterruptState *state, void *params) = {
    UmExitThread,
    UmAllocateVirtualMemory,
    UmProtectVirtualMemory,
    UmFreeVirtualMemory,
    UmCreateFile,
    UmReadFile,
    UmWriteFile,
    UmQueryInformationFile,
    UmSetInformationFile,
    UmDeviceIoControl,
    UmCloseHandle,
    UmExCallNextHandler,
    UmExContinueExecution,
    UmExPushExceptionHandler,
    UmExPopExceptionHandler,
    UmCreateThread,
    UmTerminateThread,
    UmSuspendThread,
    UmResumeThread,
    UmWaitForObject,
    UmReleaseMutex,
    UmReleaseSemaphore,
    UmSetEvent,
    UmResetEvent,
    UmSchYield,
    UmCreateMutex,
    UmCreateTimer,
    UmCreateSemaphore,
    UmCreateEvent,
    UmOpenMutex,
    UmOpenTimer,
    UmOpenSemaphore,
    UmOpenEvent,
    UmCreateProcess,
    UmTerminateProcess,
    UmResetTimer,
    UmGfxGenerateBuffers,
    UmGfxTextureData,
    UmGfxTextureFlags,
    UmGfxVertexBufferData,
    UmGfxDrawPrimitives,
    UmGfxDrawRectangle,
    UmGfxClearColor,
    UmGfxClear,
    UmGfxBitBlit,
    UmGfxUploadToDisplay,
    UmGfxRenderBufferData,
    UmPostThreadMessage,
    UmPeekThreadMessage,
    UmReceiveThreadMessage,
    UmCreateWindow,
    UmGetWindowTitle,
    UmSetWindowTitle,
    UmGetWindowBuffer,
    UmSetWindowBuffer,
    UmSwapBuffers,
    UmGetWindowParameter,
    UmSetWindowParameter,
    UmRegisterWindowServer,
    UmUnregisterWindowServer,
    UmEnumerateChildWindows,
    UmAllocateConsole,
    UmRegisterConsoleHost,
    UmUnregisterConsoleHost
};

#include <sched/scheduler.hh>

extern "C" void SyscallHandler(CpuInterruptState *state)
{
    /* Number of the syscall */
    u32 number = state->Eax;

    if (number >= SYSCALL_COUNT) {
        state->Eax = -1;
        return;
    }

    /* Pointer to the syscall parameter structure */
    void *params = (void*)state->Edx;

    PzEnableInterrupts();
    state->Eax = PrizmSyscallTable[number](state, params);
    PzDisableInterrupts();
}