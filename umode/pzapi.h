#pragma once

#include <stdint.h>
#include <stdarg.h>

#ifndef __cplusplus
typedef int bool;
#define true  1
#define false 0
#ifdef __MINGW32__
#define alignas(x) __attribute__((aligned(x)))
#else
#define alignas(x) __declspec(align(x))
#endif
#endif

#ifdef __MINGW32__

#ifdef __cplusplus
#define PZDLL_EXPORT extern "C" __attribute__((dllexport))
#else
#define PZDLL_EXPORT __attribute__((dllexport))
#endif

#else

#ifdef __cplusplus
#define PZDLL_EXPORT extern "C" __declspec(dllexport)
#else
#define PZDLL_EXPORT __declspec(dllexport)
#endif

#endif

#define ALIGN(bytes, align) ((bytes) + (align) - 1 & -(align))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uintptr_t uptr;
typedef intptr_t iptr;
typedef uintptr_t usize;
typedef u32 PzStatus;

typedef iptr PzHandle;

#define CPROC_HANDLE   ((PzHandle)-2)
#define CTHREAD_HANDLE ((PzHandle)-3)
#define STDIN_HANDLE   ((PzHandle)-4)
#define STDOUT_HANDLE  ((PzHandle)-5)
#define STDERR_HANDLE  ((PzHandle)-6)

#define STATUS_SUCCESS              ((u32)0 )
#define STATUS_FAILED               ((u32)1 )
#define STATUS_INVALID_HANDLE       ((u32)2 )
#define STATUS_WRONG_DEVICE_TYPE    ((u32)3 )
#define STATUS_TRANSFER_FAILED      ((u32)4 )
#define STATUS_INVALID_PATH         ((u32)5 )
#define STATUS_UNSUPPORTED_FUNCTION ((u32)6 )
#define STATUS_PATH_NOT_FOUND       ((u32)7 )
#define STATUS_ACCESS_DENIED        ((u32)8 )
#define STATUS_DEVICE_UNAVAILABLE   ((u32)9 )
#define STATUS_INVALID_ARGUMENT     ((u32)10)
#define STATUS_ABOVE_LIMIT          ((u32)11)
#define STATUS_NO_DATA              ((u32)12)
#define STATUS_ALLOCATION_FAILED    ((u32)13)
#define INVALID_HANDLE_VALUE ((PzHandle)-1u)

#define ACCESS_READ  1
#define ACCESS_WRITE 2

#define OPEN_EXISTING  1
#define CREATE_ALWAYS  2
#define TRUNC_EXISTING 3

#define PAGE_READ 1
#define PAGE_WRITE 2
#define PAGE_EXECUTE 4
#define PAGE_READWRITE (PAGE_READ | PAGE_WRITE)
#define PAGE_EXECUTE_READWRITE (PAGE_READWRITE | PAGE_EXECUTE)

#define FILE_INFORMATION_BASIC 1

#define GFX_RENDER_BUFFER  1
#define GFX_VERTEX_BUFFER  2
#define GFX_TEXTURE_BUFFER 3

#define GFX_PIXEL_ARGB32 1

#define GFX_FORMAT_COL    (0U << 29)
#define GFX_FORMAT_UV     (1U << 29)
#define GFX_FORMAT_INT    (0U << 30)
#define GFX_FORMAT_FLOAT  (1U << 30)

#define DEF_FORMAT_4(x, n) \
    GFX_##x             = n, \
    GFX_##x##_COL       = n | GFX_FORMAT_COL, \
    GFX_##x##_UV        = n | GFX_FORMAT_UV, \
    GFX_##x##_COL_FLOAT = n | GFX_FORMAT_COL | GFX_FORMAT_FLOAT, \
    GFX_##x##_UV_FLOAT  = n | GFX_FORMAT_UV  | GFX_FORMAT_FLOAT,

enum {
    DEF_FORMAT_4(TRI_STRIP, 0)
    DEF_FORMAT_4(TRI_LIST, 1)
    DEF_FORMAT_4(QUAD_LIST, 2)
    DEF_FORMAT_4(QUAD_STRIP, 3)
    DEF_FORMAT_4(RECT_LIST, 4)
};

#define GFX_CLEAR_COLOR 1
#define GFX_CLEAR_DEPTH 2

#define ARGB(a, r, g, b) (u32)((u8)(a) << 24 | (u8)(r) << 16 | (u8)(g) << 8 | (u8)(b))

#define WINDOW_SERVER_CREATE_WINDOW  1
#define WINDOW_SERVER_DESTROY_WINDOW 2

#define CONSOLE_HOST_ALLOC_CONSOLE 3
#define CONSOLE_HOST_WRITE_CONSOLE 4
#define CONSOLE_HOST_READ_CONSOLE  5

typedef struct {
    union { float FloatCoords[3]; int IntCoords[3]; };
    union {
        struct { float FloatU, FloatV; };
        struct { int IntU, IntV; };
        struct { u8 R, G, B, A; };
    };
} GfxVertexFormat;

#define WINDOW_POS_X        0
#define WINDOW_POS_Y        1
#define WINDOW_POS_Z        2
#define WINDOW_WIDTH        3
#define WINDOW_HEIGHT       4
#define WINDOW_STYLE        5
#define WINDOW_MAX_PARAM    63

/* A structure describing the codepoint count, size and location of a UTF-8 encoded string. */
typedef struct
{
    u32 Size;
    char *Buffer;
} PzString;

#define PZ_CONST_STRING(x) ((PzString){sizeof(x) - 1, (char*)(x)})

typedef struct
{
    PzStatus Status;
    uptr Information;
} PzIoStatusBlock;

typedef struct
{
    u64 Size;
} PzFileInformationBasic;

typedef struct {
    u32 Type;
    uptr Params[4];
} PzMessage;

#define EXCEPTION_RECORD_EXT_MAX 8

#define EXCEPTION_DIVIDE_BY_ZERO      1
#define EXCEPTION_ACCESS_VIOLATION    2
#define EXCEPTION_PRIV_INSTRUCTION    3
#define EXCEPTION_ILLEGAL_INSTRUCTION 4
#define EXCEPTION_FPU_ERROR           5
#define EXCEPTION_OTHER               6

typedef struct
{
    u32 Eflags;
    u32 Eax, Ecx, Edx, Ebx;
    u32 Esp, Ebp, Esi, Edi;
    u32 Eip;
    u32 Cs, Ds, Es, Fs, Gs, Ss;
    u32 Cr3;
    uptr FxSaveRegion;
} PzThreadContext;

typedef struct {
    u32 Code;
    u32 Flags;
    uptr InstructionAddress;
    int ExtensionCount;
    uptr Extension[EXCEPTION_RECORD_EXT_MAX];
    PzThreadContext SavedContext;
    uptr StackContext;
} PzExceptionInfo;

typedef uptr PzStackEnvBuffer[6];

typedef struct {
    PzExceptionInfo Info;
    PzStackEnvBuffer Buffer;
} PzExceptionInfoExt;

typedef struct
{
    const PzString *ProcessName;
    const PzString *ExecutablePath;
    bool InheritHandles;
    PzHandle StandardInput, StandardOutput, StandardError;
} PzProcessCreationParams;

typedef int GfxHandle;
typedef void (*PzExceptionHandler)(PzExceptionInfo info);

#ifdef __cplusplus
extern "C" {
#endif

/* Prints the specified formatted null-terminated string into the standard output. */
void PzPutStringFormatted(const char *format, ...);

/* Terminates the running thread (does not return). */
void PzThreadExit();

/* Given a process handle and information about the desired region,
    allocates a region of virtual memory in the specified process. */
PzStatus PzAllocateVirtualMemory(
    PzHandle process_handle,
    void **base_address,
    u32 size,
    u32 protection);

/* Given a process handle and information about the desired region,
   changes the protection of a region of virtual memory in the specified process. */
PzStatus PzProtectVirtualMemory(
    PzHandle process_handle,
    void **base_address,
    u32 size,
    u32 protection);

/* Given a process handle and information about the region,
    frees a region of virtual memory in the specified process. */
PzStatus PzFreeVirtualMemory(
    PzHandle process_handle,
    void *base_address,
    u32 *size);

/* Creates or opens a file given a filename, an access mask
    and a disposition type, and opens a handle to it. */
PzStatus PzCreateFile(
    PzHandle *handle, const PzString *filename,
    PzIoStatusBlock *iosb, u32 access, u32 disposition);

/* Given a handle, reads `count` byte from a file from the offset `offset` into the specified buffer. */
PzStatus PzReadFile(
    PzHandle handle, void *buffer,
    PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset);

/* Given a handle, writes `count` bytes from the specified buffer
    into the handle's associated file at the offset `offset`. */
PzStatus PzWriteFile(
    PzHandle handle, const void *buffer,
    PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset);

/* Given a handle, invalidates it and decreases the object's reference count by 1. */
PzStatus PzCloseHandle(PzHandle handle);

/* Given a handle to a file and a buffer, fills out the buffer
    with the desired type of information about the file. */
PzStatus PzQueryInformationFile(
    PzHandle handle, PzIoStatusBlock *iosb, u32 type,
    void *out_buffer, u32 buffer_size);

/* Given a file handle, changes various kinds of
    information about the file according to a data
    structure of a certain type and length. */
PzStatus PzSetInformationFile(
    PzHandle handle, PzIoStatusBlock *iosb, u32 type,
    const void *in_buffer, u32 buffer_size);

/* Given a device handle and
    information about the control code and input and output buffers,
    issues an I/O control command directly to the specified device. */
PzStatus PzDeviceIoControl(
    PzHandle device_handle,
    PzIoStatusBlock *iosb,
    u32 ctrl_code,
    void *in_buffer,
    u32 in_buffer_size,
    void *out_buffer,
    u32 out_buffer_size
);

/* Calls the next available exception handler. If this is the last exception handler, the process is terminated. */
PzStatus ExCallNextHandler();

/* Continues execution from the point the exception occurred. */
PzStatus ExContinueExecution();

/* Pushes an exception handler onto this thread's exception handler stack. */
PzStatus ExPushExceptionHandler(PzExceptionHandler handler, uptr stack_context);

/* Pop an exception handler onto this thread's exception handler stack. */
PzStatus ExPopExceptionHandler();
PzStatus PzCreateThread(
    PzHandle *handle, PzHandle parent_process,
    int (*start)(void *param), void *param,
    int stack_size, int priority);
PzStatus PzTerminateThread(PzHandle thread, int exit_code);
PzStatus PzSuspendThread(PzHandle thread, int *suspended_count);
PzStatus PzResumeThread(PzHandle thread, int *suspended_count);
PzStatus PzWaitForObject(PzHandle object);
PzStatus PzReleaseMutex(PzHandle mutex);
PzStatus PzReleaseSemaphore(PzHandle semaphore, int count);
PzStatus PzSetEvent(PzHandle event);
PzStatus PzResetEvent(PzHandle event);
PzStatus PzSchYield();
PzStatus PzCreateMutex(PzHandle *handle, const PzString *name);
PzStatus PzCreateTimer(PzHandle *handle, const PzString *name);
PzStatus PzCreateSemaphore(PzHandle *handle, const PzString *name);
PzStatus PzCreateEvent(PzHandle *handle, const PzString *name);
PzStatus PzOpenMutex(PzHandle *handle, const PzString *name);
PzStatus PzOpenTimer(PzHandle *handle, const PzString *name);
PzStatus PzOpenSemaphore(PzHandle *handle, const PzString *name);
PzStatus PzOpenEvent(PzHandle *handle, const PzString *name);
PzStatus PzCreateProcess(PzHandle *process_handle, const PzProcessCreationParams *params);
PzStatus PzTerminateProcess(PzHandle process_handle, int exit_code);
PzStatus PzResetTimer(PzHandle handle, int milliseconds);

PzStatus GfxGenerateBuffers(int type, int count, GfxHandle *handles);
PzStatus GfxTextureData(GfxHandle handle,
    int width, int height, int pixel_format, int flags, void *data);
PzStatus GfxTextureFlags(GfxHandle handle, u32 flags);
PzStatus GfxVertexBufferData(GfxHandle handle, void *data, u32 size);
PzStatus GfxDrawPrimitives(
    GfxHandle render_handle, GfxHandle vbo_handle, GfxHandle texture_handle,
    int data_format, int start_index, int vertex_count);
PzStatus GfxDrawRectangle(
    GfxHandle render_handle, GfxHandle texture_handle,
    int x, int y, int width, int height, u32 color, bool int_uvs, const void *uvs);
PzStatus GfxClearColor(GfxHandle render_handle, u32 color);
PzStatus GfxClear(GfxHandle render_handle, u32 flags);
PzStatus GfxBitBlit(
    GfxHandle dest_buffer, int dx, int dy, int dw, int dh,
    GfxHandle src_buffer, int sx, int sy);
PzStatus GfxUploadToDisplay(GfxHandle render_buffer);
PzStatus GfxRenderBufferData(GfxHandle handle,
    int width, int height,
    int pixel_format);

PzStatus PzPostThreadMessage(PzHandle thread, u32 type, uptr param1, uptr param2, uptr param3, uptr param4);
PzStatus PzPeekThreadMessage(PzMessage *out_message);
PzStatus PzReceiveThreadMessage(PzMessage *out_message);

PzStatus PzCreateWindow(
    PzHandle *handle, PzHandle parent,
    const PzString *title,
    int x, int y, int z,
    u32 width, u32 height,
    int style);

PzStatus PzGetWindowTitle(PzHandle window, char *out_title, u32 *max_size);
PzStatus PzSetWindowTitle(PzHandle window, const PzString *out_title);
PzStatus PzGetWindowBuffer(PzHandle window, int index, GfxHandle *handle);
PzStatus PzSetWindowBuffer(PzHandle window, int index, GfxHandle handle);
PzStatus PzSwapBuffers(PzHandle window);
PzStatus PzGetWindowParameter(PzHandle window, int index, uptr *value);
PzStatus PzSetWindowParameter(PzHandle window, int index, uptr value);
PzStatus PzRegisterWindowServer();
PzStatus PzUnregisterWindowServer();
PzStatus PzEnumerateChildWindows(PzHandle window, u32 *max_count, PzHandle *handles, bool recursive);
PzStatus PzAllocateConsole();
PzStatus PzRegisterConsoleHost();
PzStatus PzUnregisterConsoleHost();

#define printf PzPutStringFormatted

/* Essentially a setjmp/longjmp implementation. */
#ifdef __GNUC__
__attribute__((noinline, noclone, returns_twice, optimize(0)))
#endif
int PzSaveStackEnvironment(PzStackEnvBuffer *buffer);
#ifdef __GNUC__
__attribute__((noinline, noclone, optimize(0)))
#endif
void PzRestoreStackEnvironment(PzStackEnvBuffer *buffer, int ret);
void PzExceptionRouter(PzExceptionInfo info);
void MemMove(void *dest, const void *src, int count);
void MemCopy(void *dest, const void *src, int count);
void MemSet(void *dest, int value, int count);

#ifdef __cplusplus
}
#endif

#define __try \
      { \
          PzExceptionInfoExt _exception_data; \
          PzExceptionInfo *ExceptionInfo = &_exception_data.Info; \
          ExPushExceptionHandler(PzExceptionRouter, (uptr)&_exception_data); \
          int _save_result = PzSaveStackEnvironment(&_exception_data.Buffer); \
          \
          for (;;) { \
              if (_save_result) \
                  break;

#define __except \
              break; \
          } \
          ExPopExceptionHandler(); \
          if (_save_result)
#define __finally
#define __end }

#define PL_ALLOC_FLAGS_ZERO 1
#define PL_DEFAULT_GRANULARITY 64

typedef struct PzHeapBlockHeader
{
    struct PzHeapBlockHeader *Previous, *Next;
    u32 Granularity;
    u32 LastIndex;
    u32 Used, Size;
    u32 BitmapSize;
    alignas(64) u8 BitmapStart[0];
} PzHeapBlockHeader;

typedef struct
{
    PzHandle Mutex;
    PzHeapBlockHeader *FirstBlock, *LastBlock;
} PzHeap;

PZDLL_EXPORT void PzInitializeHeap(PzHeap *pool, int init_size);
PZDLL_EXPORT PzHeapBlockHeader *PzCreateHeapBlock(PzHeap *pool, int size, int granularity);
PZDLL_EXPORT void PzUnlinkHeapBlock(PzHeap *pool, PzHeapBlockHeader *header);
PZDLL_EXPORT void *PzAllocateHeapMemory(PzHeap *pool, int bytes, u32 flags);
PZDLL_EXPORT void *PzReAllocateHeapMemory(PzHeap *pool, void *ptr, int bytes);
PZDLL_EXPORT int PzFreeHeapMemory(PzHeap *pool, void *ptr);

void PrintfWithCallbackV(void (*print_func)(char), const char *format, va_list params);