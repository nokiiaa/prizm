#pragma once

#include <io/manager.hh>
#include <lib/string.hh>
#include <syscall/handle.hh>
#include <defs.hh>
#include <core.hh>

struct UmCreateFileParams
{
    PzHandle *Handle;
    const PzString *Filename;
    PzIoStatusBlock *Iosb;
    u32 Access;
    u32 Disposition;
};

struct UmReadFileParams
{
    PzHandle Handle;
    void *Buffer;
    PzIoStatusBlock *Iosb;
    u32 Bytes;
    u64 *Offset;
};

struct UmWriteFileParams
{
    PzHandle Handle;
    const void *Buffer;
    PzIoStatusBlock *Iosb;
    u32 Bytes;
    u64 *Offset;
};

struct UmQueryInformationFileParams
{
    PzHandle Handle;
    PzIoStatusBlock *Iosb;
    u32 Type;
    void *OutBuffer;
    u32 BufferSize;
};

struct UmSetInformationFileParams
{
    PzHandle Handle;
    PzIoStatusBlock *Iosb;
    u32 Type;
    const void *Buffer;
    u32 BufferSize;
};

struct UmDeviceIoControlParams
{
    PzHandle DeviceHandle;
    PzIoStatusBlock *Iosb;
    u32 ControlCode;
    void *InBuffer;
    u32 InBufSize;
    void *OutBuffer;
    u32 OutBufSize;
};

struct UmCloseHandleParams
{
    PzHandle Handle;
};

DECL_SYSCALL(UmCreateFile);
DECL_SYSCALL(UmReadFile);
DECL_SYSCALL(UmWriteFile);
DECL_SYSCALL(UmQueryInformationFile);
DECL_SYSCALL(UmSetInformationFile);
DECL_SYSCALL(UmDeviceIoControl);
DECL_SYSCALL(UmCloseHandle);