# pzdll.dll
This library contains a wrapper for usermode syscalls, a usermode heap implementation and other things. Here is a list of all the syscalls it currently supports (whose parameters may be looked up in pzapi.h):
- PzExitThread
- PzAllocateVirtualMemory
- PzProtectVirtualMemory
- PzFreeVirtualMemory
- PzCreateFile
- PzReadFile
- PzWriteFile
- PzQueryInformationFile
- PzSetInformationFile
- PzDeviceIoControl
- PzCloseHandle
- ExCallNextHandler
- ExContinueExecution
- ExPushExceptionHandler
- ExPopExceptionHandler
- PzCreateThread
- PzTerminateThread
- PzSuspendThread
- PzResumeThread
- PzWaitForObject
- PzReleaseMutex
- PzReleaseSemaphore
- PzSetEvent
- PzResetEvent
- PzSchYield
- PzCreateMutex
- PzCreateTimer
- PzCreateSemaphore
- PzCreateEvent
- PzOpenMutex
- PzOpenTimer
- PzOpenSemaphore
- PzOpenEvent
- PzCreateProcess
- PzTerminateProcess
- PzResetTimer
- GfxGenerateBuffers
- GfxTextureData
- GfxTextureFlags
- GfxVertexBufferData
- GfxDrawPrimitives
- GfxDrawRectangle
- GfxClearColor
- GfxClear
- GfxBitBlit
- GfxUploadToDisplay
- GfxRenderBufferData
- PzPostThreadMessage
- PzPeekThreadMessage
- PzReceiveThreadMessage
- PzCreateWindow
- PzGetWindowTitle
- PzSetWindowTitle
- PzGetWindowBuffer
- PzSetWindowBuffer
- PzSwapBuffers
- PzGetWindowParameter
- PzSetWindowParameter
- PzRegisterWindowServer
- PzUnregisterWindowServer
- PzEnumerateChildWindows