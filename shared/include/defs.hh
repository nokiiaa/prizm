#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __MINGW32__
#define PZ_KERNEL_EXPORT extern "C" __attribute__((dllexport))
#define PZ_KERNEL_EXPORT_CPP __attribute__((dllexport))
#else
#define PZ_KERNEL_EXPORT
#define PZ_KERNEL_EXPORT_CPP
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
#ifdef DEBUGGER_INCLUDE
typedef DEBUGGER_TARGET_PTR uptr;
typedef DEBUGGER_TARGET_IPTR iptr;
typedef DEBUGGER_TARGET_SIZE usize;
#else
typedef uintptr_t uptr;
typedef intptr_t iptr;
typedef size_t usize;
#endif
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