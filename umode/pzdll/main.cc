#include <pzapi.h>

enum {
    PZ_SYSCALL_THREAD_EXIT,
    PZ_SYSCALL_ALLOC_VMEM,
    PZ_SYSCALL_PROTECT_VMEM,
    PZ_SYSCALL_FREE_VMEM,
    PZ_SYSCALL_CREATE_FILE,
    PZ_SYSCALL_READ_FILE,
    PZ_SYSCALL_WRITE_FILE,
    PZ_SYSCALL_QUERY_INFO_FILE,
    PZ_SYSCALL_SET_INFO_FILE,
    PZ_SYSCALL_DEVICE_IOCTL,
    PZ_SYSCALL_CLOSE_HANDLE,
    PZ_SYSCALL_EX_CALL_NEXT_HANDLER,
    PZ_SYSCALL_EX_CONTINUE_EXEC,
    PZ_SYSCALL_EX_PUSH_EX_HANDLER,
    PZ_SYSCALL_EX_POP_EX_HANDLER,
    PZ_SYSCALL_CREATE_THREAD,
    PZ_SYSCALL_TERMINATE_THREAD,
    PZ_SYSCALL_SUSPEND_THREAD,
    PZ_SYSCALL_RESUME_THREAD,
    PZ_SYSCALL_WAIT_FOR_OBJECT,
    PZ_SYSCALL_RELEASE_MUTEX,
    PZ_SYSCALL_RELEASE_SEMAPHORE,
    PZ_SYSCALL_SET_EVENT,
    PZ_SYSCALL_RESET_EVENT,
    PZ_SYSCALL_SCH_YIELD,
    PZ_SYSCALL_CREATE_MUTEX,
    PZ_SYSCALL_CREATE_TIMER,
    PZ_SYSCALL_CREATE_SEMAPHORE,
    PZ_SYSCALL_CREATE_EVENT,
    PZ_SYSCALL_OPEN_MUTEX,
    PZ_SYSCALL_OPEN_TIMER,
    PZ_SYSCALL_OPEN_SEMAPHORE,
    PZ_SYSCALL_OPEN_EVENT,
    PZ_SYSCALL_CREATE_PROCESS,
    PZ_SYSCALL_TERMINATE_PROCESS,
    PZ_SYSCALL_RESET_TIMER,
    PZ_SYSCALL_GFX_GENERATE_BUFFERS,
    PZ_SYSCALL_GFX_TEXTURE_DATA,
    PZ_SYSCALL_GFX_TEXTURE_FLAGS,
    PZ_SYSCALL_GFX_VERTEX_BUFFER_DATA,
    PZ_SYSCALL_GFX_DRAW_PRIMITIVES,
    PZ_SYSCALL_GFX_DRAW_RECTANGLE,
    PZ_SYSCALL_GFX_CLEAR_COLOR,
    PZ_SYSCALL_GFX_CLEAR,
    PZ_SYSCALL_GFX_BIT_BLIT,
    PZ_SYSCALL_GFX_UPLOAD_TO_DISPLAY,
    PZ_SYSCALL_GFX_RENDER_BUFFER_DATA,
    PZ_SYSCALL_POST_THREAD_MESSAGE,
    PZ_SYSCALL_PEEK_THREAD_MESSAGE,
    PZ_SYSCALL_RECEIVE_THREAD_MESSAGE,
    PZ_SYSCALL_CREATE_WINDOW,
    PZ_SYSCALL_GET_WINDOW_TITLE,
    PZ_SYSCALL_SET_WINDOW_TITLE,
    PZ_SYSCALL_GET_WINDOW_BUFFER,
    PZ_SYSCALL_SET_WINDOW_BUFFER,
    PZ_SYSCALL_SWAP_BUFFERS,
    PZ_SYSCALL_GET_WINDOW_PARAMETER,
    PZ_SYSCALL_SET_WINDOW_PARAMETER,
    PZ_SYSCALL_REGISTER_WINDOW_SERVER,
    PZ_SYSCALL_UNREGISTER_WINDOW_SERVER,
    PZ_SYSCALL_ENUMERATE_CHILD_WINDOWS,
    PZ_SYSCALL_ALLOCATE_CONSOLE,
    PZ_SYSCALL_REGISTER_CONSOLE_HOST,
    PZ_SYSCALL_UNREGISTER_CONSOLE_HOST
};

PzStatus PzExecuteSystemCall(int number, const void *params)
{
#ifdef __GNUC__
    asm volatile("movl %0, %%eax;"
        "movl %1, %%edx;"
        "int $0x80"
        :
        : "g"(number), "g"(params)
        : "eax", "edx"
        );

    register u32 ret asm("eax");
#else
    #error TODO: msvc inline assembly for this function
#endif
    return ret;
}

PZDLL_EXPORT void PzThreadExit()
{
    PzExecuteSystemCall(PZ_SYSCALL_THREAD_EXIT, nullptr);
}

PZDLL_EXPORT PzStatus PzAllocateVirtualMemory(
    PzHandle process_handle,
    void **base_address,
    u32 size,
    u32 protection)
{
    return PzExecuteSystemCall(PZ_SYSCALL_ALLOC_VMEM, &process_handle);
}

PZDLL_EXPORT PzStatus PzProtectVirtualMemory(
    PzHandle process_handle,
    void **base_address,
    u32 size,
    u32 protection)
{
    return PzExecuteSystemCall(PZ_SYSCALL_PROTECT_VMEM, &process_handle);
}

PZDLL_EXPORT PzStatus PzFreeVirtualMemory(
    PzHandle process_handle,
    void *base_address,
    u32 *size)
{
    return PzExecuteSystemCall(PZ_SYSCALL_FREE_VMEM, &process_handle);
}

PZDLL_EXPORT PzStatus PzCreateFile(
    PzHandle *handle, const PzString *filename,
    PzIoStatusBlock *iosb, u32 access, u32 disposition
)
{
    return PzExecuteSystemCall(PZ_SYSCALL_CREATE_FILE, &handle);
}

PZDLL_EXPORT PzStatus PzReadFile(
    PzHandle handle, void *buffer,
    PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset
)
{
    return PzExecuteSystemCall(PZ_SYSCALL_READ_FILE, &handle);
}

PZDLL_EXPORT PzStatus PzWriteFile(
    PzHandle handle, const void *buffer,
    PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset
)
{
    return PzExecuteSystemCall(PZ_SYSCALL_WRITE_FILE, &handle);
}

PZDLL_EXPORT PzStatus PzCloseHandle(PzHandle handle)
{
    return PzExecuteSystemCall(PZ_SYSCALL_CLOSE_HANDLE, &handle);
}

PZDLL_EXPORT PzStatus PzQueryInformationFile(
    PzHandle handle, PzIoStatusBlock *iosb, u32 type,
    void *out_buffer, u32 buffer_size)
{
    return PzExecuteSystemCall(PZ_SYSCALL_QUERY_INFO_FILE, &handle);
}

PZDLL_EXPORT PzStatus PzSetInformationFile(
    PzHandle handle, PzIoStatusBlock *iosb, u32 type,
    const void *in_buffer, u32 buffer_size)
{
    return PzExecuteSystemCall(PZ_SYSCALL_SET_INFO_FILE, &handle);
}

PZDLL_EXPORT PzStatus PzDeviceIoControl(
    PzHandle device_handle,
    PzIoStatusBlock *iosb,
    u32 ctrl_code,
    void *in_buffer,
    u32 in_buffer_size,
    void *out_buffer,
    u32 out_buffer_size
)
{
    return PzExecuteSystemCall(PZ_SYSCALL_DEVICE_IOCTL, &device_handle);
}

PZDLL_EXPORT PzStatus ExCallNextHandler()
{
    return PzExecuteSystemCall(PZ_SYSCALL_EX_CALL_NEXT_HANDLER, nullptr);
}

PZDLL_EXPORT PzStatus ExContinueExecution()
{
    return PzExecuteSystemCall(PZ_SYSCALL_EX_CONTINUE_EXEC, nullptr);
}

PZDLL_EXPORT PzStatus ExPushExceptionHandler(PzExceptionHandler handler, uptr stack_context)
{
    return PzExecuteSystemCall(PZ_SYSCALL_EX_PUSH_EX_HANDLER, &handler);
}

PZDLL_EXPORT PzStatus ExPopExceptionHandler()
{
    return PzExecuteSystemCall(PZ_SYSCALL_EX_POP_EX_HANDLER, nullptr);
}

PZDLL_EXPORT PzStatus PzCreateThread(
    PzHandle *handle, PzHandle parent_process,
    int (*start)(void *param), void *param,
    int stack_size, int priority)
{
    return PzExecuteSystemCall(PZ_SYSCALL_CREATE_THREAD, &handle);
}

PZDLL_EXPORT PzStatus PzTerminateThread(PzHandle thread, int exit_code)
{
    return PzExecuteSystemCall(PZ_SYSCALL_TERMINATE_THREAD, &thread);
}

PZDLL_EXPORT PzStatus PzSuspendThread(PzHandle thread, int *suspended_count)
{
    return PzExecuteSystemCall(PZ_SYSCALL_SUSPEND_THREAD, &thread);
}

PZDLL_EXPORT PzStatus PzResumeThread(PzHandle thread, int *suspended_count)
{
    return PzExecuteSystemCall(PZ_SYSCALL_RESUME_THREAD, &thread);
}

PZDLL_EXPORT PzStatus PzWaitForObject(PzHandle object)
{
    return PzExecuteSystemCall(PZ_SYSCALL_WAIT_FOR_OBJECT, &object);
}

PZDLL_EXPORT PzStatus PzReleaseMutex(PzHandle mutex)
{
    return PzExecuteSystemCall(PZ_SYSCALL_RELEASE_MUTEX, &mutex);
}

PZDLL_EXPORT PzStatus PzReleaseSemaphore(PzHandle semaphore, int count)
{
    return PzExecuteSystemCall(PZ_SYSCALL_RELEASE_SEMAPHORE, &semaphore);
}

PZDLL_EXPORT PzStatus PzSetEvent(PzHandle event)
{
    return PzExecuteSystemCall(PZ_SYSCALL_SET_EVENT, &event);
}

PZDLL_EXPORT PzStatus PzResetEvent(PzHandle event)
{
    return PzExecuteSystemCall(PZ_SYSCALL_RESET_EVENT, &event);
}

PZDLL_EXPORT PzStatus PzSchYield()
{
    return PzExecuteSystemCall(PZ_SYSCALL_SCH_YIELD, nullptr);
}

PZDLL_EXPORT PzStatus PzCreateMutex(PzHandle *handle, const PzString *name)
{
    return PzExecuteSystemCall(PZ_SYSCALL_CREATE_MUTEX, &handle);
}

PZDLL_EXPORT PzStatus PzCreateTimer(PzHandle *handle, const PzString *name)
{
    return PzExecuteSystemCall(PZ_SYSCALL_CREATE_TIMER, &handle);
}

PZDLL_EXPORT PzStatus PzCreateSemaphore(PzHandle *handle, const PzString *name)
{
    return PzExecuteSystemCall(PZ_SYSCALL_CREATE_SEMAPHORE, &handle);
}

PZDLL_EXPORT PzStatus PzCreateEvent(PzHandle *handle, const PzString *name)
{
    return PzExecuteSystemCall(PZ_SYSCALL_CREATE_EVENT, &handle);
}

PZDLL_EXPORT PzStatus PzOpenMutex(PzHandle *handle, const PzString *name)
{
    return PzExecuteSystemCall(PZ_SYSCALL_OPEN_MUTEX, &handle);
}

PZDLL_EXPORT PzStatus PzOpenTimer(PzHandle *handle, const PzString *name)
{
    return PzExecuteSystemCall(PZ_SYSCALL_OPEN_TIMER, &handle);
}

PZDLL_EXPORT PzStatus PzOpenSemaphore(PzHandle *handle, const PzString *name)
{
    return PzExecuteSystemCall(PZ_SYSCALL_OPEN_SEMAPHORE, &handle);
}

PZDLL_EXPORT PzStatus PzOpenEvent(PzHandle *handle, const PzString *name)
{
    return PzExecuteSystemCall(PZ_SYSCALL_OPEN_EVENT, &handle);
}

PZDLL_EXPORT PzStatus PzCreateProcess(PzHandle *process_handle, const PzProcessCreationParams *params)
{
    return PzExecuteSystemCall(PZ_SYSCALL_CREATE_PROCESS, &process_handle);
}

PZDLL_EXPORT PzStatus PzTerminateProcess(PzHandle process_handle, int exit_code)
{
    return PzExecuteSystemCall(PZ_SYSCALL_TERMINATE_PROCESS, &process_handle);
}

PZDLL_EXPORT PzStatus PzResetTimer(PzHandle handle, int milliseconds)
{
    return PzExecuteSystemCall(PZ_SYSCALL_RESET_TIMER, &handle);
}

PZDLL_EXPORT PzStatus GfxGenerateBuffers(int type, int count, GfxHandle *handles)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_GENERATE_BUFFERS, &type);
}

PZDLL_EXPORT PzStatus GfxTextureData(GfxHandle handle,
    int width, int height, int pixel_format, int flags, void *data)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_TEXTURE_DATA, &handle);
}

PZDLL_EXPORT PzStatus GfxTextureFlags(GfxHandle handle, u32 flags)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_TEXTURE_FLAGS, &handle);
}

PZDLL_EXPORT PzStatus GfxVertexBufferData(GfxHandle handle, void *data, u32 size)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_VERTEX_BUFFER_DATA, &handle);
}

PZDLL_EXPORT PzStatus GfxDrawPrimitives(
    GfxHandle render_handle, GfxHandle vbo_handle, GfxHandle texture_handle,
    int data_format, int start_index, int vertex_count)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_DRAW_PRIMITIVES, &render_handle);
}

PZDLL_EXPORT PzStatus GfxDrawRectangle(
    GfxHandle render_handle, GfxHandle texture_handle,
    int x, int y, int width, int height, u32 color, bool int_uvs, const void *uvs)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_DRAW_RECTANGLE, &render_handle);
}

PZDLL_EXPORT PzStatus GfxClear(GfxHandle render_handle, u32 flags)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_CLEAR, &render_handle);
}

PZDLL_EXPORT PzStatus GfxClearColor(GfxHandle render_handle, u32 flags)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_CLEAR_COLOR, &render_handle);
}

PZDLL_EXPORT PzStatus GfxBitBlit(
    GfxHandle dest_buffer, int dx, int dy, int dw, int dh,
    GfxHandle src_buffer, int sx, int sy)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_BIT_BLIT, &dest_buffer);
}

PZDLL_EXPORT PzStatus GfxUploadToDisplay(GfxHandle render_buffer)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_UPLOAD_TO_DISPLAY, &render_buffer);
}

PZDLL_EXPORT PzStatus GfxRenderBufferData(GfxHandle handle,
    int width, int height,
    int pixel_format)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GFX_RENDER_BUFFER_DATA, &handle);
}

PZDLL_EXPORT PzStatus PzPostThreadMessage(
    PzHandle thread, u32 type, uptr param1, uptr param2, uptr param3, uptr param4)
{
    return PzExecuteSystemCall(PZ_SYSCALL_POST_THREAD_MESSAGE, &thread);
}

PZDLL_EXPORT PzStatus PzPeekThreadMessage(
    PzMessage *out_message)
{
    return PzExecuteSystemCall(PZ_SYSCALL_PEEK_THREAD_MESSAGE, &out_message);
}

PZDLL_EXPORT PzStatus PzReceiveThreadMessage(
    PzMessage *out_message)
{
    return PzExecuteSystemCall(PZ_SYSCALL_RECEIVE_THREAD_MESSAGE, &out_message);
}

PZDLL_EXPORT PzStatus PzCreateWindow(
    PzHandle *handle, PzHandle parent,
    const PzString *title,
    int x, int y, int z,
    u32 width, u32 height,
    int style)
{
    return PzExecuteSystemCall(PZ_SYSCALL_CREATE_WINDOW, &handle);
}

PZDLL_EXPORT PzStatus PzGetWindowTitle(PzHandle window, char *out_title, u32 *max_size)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GET_WINDOW_TITLE, &window);
}

PZDLL_EXPORT PzStatus PzSetWindowTitle(PzHandle window, const PzString *out_title)
{
    return PzExecuteSystemCall(PZ_SYSCALL_SET_WINDOW_TITLE, &window);
}

PZDLL_EXPORT PzStatus PzGetWindowBuffer(PzHandle window, int index, GfxHandle *handle)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GET_WINDOW_BUFFER, &window);
}

PZDLL_EXPORT PzStatus PzSetWindowBuffer(PzHandle window, int index, GfxHandle handle)
{
    return PzExecuteSystemCall(PZ_SYSCALL_SET_WINDOW_BUFFER, &window);
}

PZDLL_EXPORT PzStatus PzSwapBuffers(PzHandle window)
{
    return PzExecuteSystemCall(PZ_SYSCALL_SWAP_BUFFERS, &window);
}

PZDLL_EXPORT PzStatus PzGetWindowParameter(PzHandle window, int index, uptr *value)
{
    return PzExecuteSystemCall(PZ_SYSCALL_GET_WINDOW_PARAMETER, &window);
}

PZDLL_EXPORT PzStatus PzSetWindowParameter(PzHandle window, int index, uptr value)
{
    return PzExecuteSystemCall(PZ_SYSCALL_SET_WINDOW_PARAMETER, &window);
}

PZDLL_EXPORT PzStatus PzRegisterWindowServer()
{
    return PzExecuteSystemCall(PZ_SYSCALL_REGISTER_WINDOW_SERVER, nullptr);
}

PZDLL_EXPORT PzStatus PzUnregisterWindowServer()
{
    return PzExecuteSystemCall(PZ_SYSCALL_UNREGISTER_WINDOW_SERVER, nullptr);
}

PZDLL_EXPORT PzStatus PzEnumerateChildWindows(
    PzHandle window, u32 *max_count, PzHandle *handles, bool recursive)
{
    return PzExecuteSystemCall(PZ_SYSCALL_ENUMERATE_CHILD_WINDOWS, &window);
}

PZDLL_EXPORT PzStatus PzAllocateConsole()
{
    return PzExecuteSystemCall(PZ_SYSCALL_ALLOCATE_CONSOLE, nullptr);
}

PZDLL_EXPORT PzStatus PzRegisterConsoleHost()
{
    return PzExecuteSystemCall(PZ_SYSCALL_REGISTER_CONSOLE_HOST, nullptr);
}

PZDLL_EXPORT PzStatus PzUnregisterConsoleHost()
{
    return PzExecuteSystemCall(PZ_SYSCALL_UNREGISTER_CONSOLE_HOST, nullptr);
}

PZDLL_EXPORT int PzSaveStackEnvironment(PzStackEnvBuffer *buffer)
{
#ifdef __GNUC__
    __asm__(
        "mov 4(%esp), %eax\n"
        "mov %ebx, (%eax)\n"
        "mov %esi, 4(%eax)\n"
        "mov %edi, 8(%eax)\n"
        "mov %ebp, 12(%eax)\n"
        "lea 4(%esp), %ecx\n"
        "mov %ecx, 16(%eax)\n"
        "mov (%esp), %ecx\n"
        "mov %ecx, 20(%eax)\n"
        "xor %eax, %eax\n"
    );
#else
    #error TODO: msvc inline assembly for this function
#endif
    return 0;
}

PZDLL_EXPORT void PzRestoreStackEnvironment(PzStackEnvBuffer *buffer, int ret)
{
#ifdef __GNUC__
    __asm__("mov  4(%esp), %edx\n"
        "    mov  8(%esp), %eax\n"
        "    test    %eax, %eax\n"
        "    jnz 1f\n"
        "    inc     %eax\n"
        "1:\n"
        "    mov   (%edx), %ebx\n"
        "    mov  4(%edx), %esi\n"
        "    mov  8(%edx), %edi\n"
        "    mov 12(%edx), %ebp\n"
        "    mov 16(%edx), %ecx\n"
        "    mov     %ecx, %esp\n"
        "    mov 20(%edx), %ecx\n"
        "    jmp *%ecx");
#else
    #error TODO: msvc inline assembly for this function
#endif
}

PZDLL_EXPORT void MemMove(void *dest, const void *src, int count)
{
    if (src > dest)
        MemCopy(dest, src, count);
    else for (int i = count - 1; i >= 0; i--)
        ((u8 *)dest)[i] = ((u8 *)src)[i];
}

PZDLL_EXPORT void MemCopy(void *dest, const void *src, int count)
{
    for (int i = 0; i < count; i++)
        ((u8 *)dest)[i] = ((u8 *)src)[i];
}

PZDLL_EXPORT void MemSet(void *dest, int value, int count)
{
    for (int i = 0; i < count; i++)
        ((u8 *)dest)[i] = value;
}

PZDLL_EXPORT void PzExceptionRouter(PzExceptionInfo info)
{
    auto *ext = (PzExceptionInfoExt *)info.StackContext;
    ext->Info = info;
    PzRestoreStackEnvironment(&ext->Buffer, 1);
}

PZDLL_EXPORT PzStatus PzDllEntry(void *base)
{
    return STATUS_SUCCESS;
}