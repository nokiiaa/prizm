#pragma once

#include <defs.hh>
#include <obj/driver.hh>
#include <lib/string.hh>
#include <obj/thread.hh>
#include <obj/process.hh>

#define ACCESS_READ  1
#define ACCESS_WRITE 2

#define OPEN_EXISTING  1
#define CREATE_ALWAYS  2
#define TRUNC_EXISTING 3

#define IRP_MJ_CREATE          1
#define IRP_MJ_READ            2
#define IRP_MJ_WRITE           3
#define IRP_MJ_SET_FILE        4
#define IRP_MJ_QUERY_INFO_DIR  5
#define IRP_MJ_QUERY_INFO_FILE 6
#define IRP_MJ_SET_INFO_DIR    7
#define IRP_MJ_SET_INFO_FILE   8
#define IRP_MJ_DEVICE_IOCTL    9
#define IRP_MJ_CLOSE           10
#define IRP_MJ_MOUNT_FS        11
#define IRP_MJ_LAST            IRP_MJ_MOUNT_FS
#define MJ_FUNC_MAX            31

struct PzIoRequestPacket;

struct PzIoControlBlock
{
#ifdef DEBUGGER_INCLUDE
    DEBUGGER_TARGET_PTR Device;
    DEBUGGER_TARGET_PTR Filename;
    DEBUGGER_TARGET_PTR Irp;
#else
    PzDeviceObject *Device;
    PzString *Filename;
    PzIoRequestPacket *Irp;
#endif
    u64 CurrentOffset;
    uptr Context;
};

struct PzIoStatusBlock
{
    PzStatus Status;
    uptr Information;
};

struct PzIoStackLocation
{
    u8 MajorFunction;
    u8 MinorFunction;
    uptr Context;

    union
    {
        struct
        {
            u32 Access;
            u32 Disposition;
        } Create;

        struct
        {
            u32 Length;
            u64 Offset;
        } Read;

        struct
        {
            u32 Length;
            u64 Offset;
        } Write;

        struct
        {

        } QueryInfoDirectory;

        struct
        {
            u32 Length;
        } QueryInfoFile;

        struct
        {
            u32 Length;
        } SetInfoDirectory;

        struct
        {
            u32 Length;
        } SetInfoFile;

        struct
        {
            u32 Code;
            u32 InputBufLength;
            u32 OutputBufLength;
            #ifdef DEBUGGER_INCLUDE
                DEBUGGER_TARGET_PTR InputBuffer;
            #else
                const void *InputBuffer;
            #endif
        } IoControl;

        struct
        {
            #ifdef DEBUGGER_INCLUDE
                DEBUGGER_TARGET_PTR BlockDevice;
                DEBUGGER_TARGET_PTR MountPoint;
            #else
                PzIoControlBlock *BlockDevice;
                PzString *MountPoint;
            #endif
        } MountFs;
    } Parameters;
};

#define FILE_INFORMATION_BASIC 1

struct PzFileInformationBasic
{
    u64 Size;
};

struct PzIoRequestPacket
{
#ifdef DEBUGGER_INCLUDE
    DEBUGGER_TARGET_PTR
        UserBuffer,
        SystemBuffer,
        UserStatus,
        AssociatedThread,
        CurrentLocation,
        Iocb;
#else
    void *UserBuffer;
    void *SystemBuffer;
    PzIoStatusBlock *UserStatus;
    PzThreadObject *AssociatedThread;
    PzIoStackLocation *CurrentLocation;
    PzIoControlBlock *Iocb;
#endif
};

/* Allocates an I/O request packet object and the specified amount of I/O stack locations for it. */
PZ_KERNEL_EXPORT PzIoRequestPacket *IoAllocateIrp(int locations);
/* Deallocates an I/O request packet object. */
PZ_KERNEL_EXPORT void IoFreeIrp(PzIoRequestPacket *irp);
/* Calls a driver by the major function number specified by the IRP in the provided device structure. */
PZ_KERNEL_EXPORT PzStatus IoCallDriver(
    PzDeviceObject *device, PzIoRequestPacket *irp);
/* Creates a device object with an associated driver and name. */
PZ_KERNEL_EXPORT PzStatus IoCreateDevice(
    PzDriverObject *driver, u32 type, u32 flags,
    PzString *name, PzDeviceObject **device);
/* Opens a handle to a file object given a filename, an access mask and a disposition type. */
PZ_KERNEL_EXPORT PzStatus PzCreateFile(
    bool kernel, PzHandle *handle, const PzString *filename,
    PzIoStatusBlock *iosb, u32 access, u32 disposition
);
/* Given a pointer to an I/O control block,
   reads `count` byte from a file from the offset `offset` into the specified buffer. */
PZ_KERNEL_EXPORT PzStatus PzReadFileRaw(
    PzIoControlBlock *control_block,
    void *buffer, PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset
);
/* Given a handle, reads `count` byte from a file from the offset `offset` into the specified buffer. */
PZ_KERNEL_EXPORT PzStatus PzReadFile(
    PzHandle handle, void *buffer,
    PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset
);
/* Given a pointer to an I/O control block,
   writes `count` bytes from the specified buffer
   into the handle's associated file at the offset `offset`. */
PZ_KERNEL_EXPORT PzStatus PzWriteFileRaw(
    PzIoControlBlock *control_block,
    const void *buffer, PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset
);
/* Given a handle, writes `count` bytes from the specified buffer
   into the handle's associated file at the offset `offset`. */
PZ_KERNEL_EXPORT PzStatus PzWriteFile(
    PzHandle handle, const void *buffer,
    PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset
);
/* Closes a handle to a given file. */
PZ_KERNEL_EXPORT PzStatus PzCloseHandle(PzHandle handle);
/* Given a pointer to an I/O control block, retrieves an information data
   structure of a certain type and length to a given buffer. */
PZ_KERNEL_EXPORT PzStatus PzQueryInformationFileRaw(
    PzIoControlBlock *control_block,
    PzIoStatusBlock *iosb, u32 type,
    void *out_buffer, u32 buffer_size);
/* Given a file handle, retrieves an information data
   structure of a certain type and length to a given buffer. */
PZ_KERNEL_EXPORT PzStatus PzQueryInformationFile(
    PzHandle handle, PzIoStatusBlock *iosb, u32 type,
    void *out_buffer, u32 buffer_size);
/* Given a pointer to an I/O control block, changes various kinds of
   information about the file according to a data
   structure of a certain type and length. */
PZ_KERNEL_EXPORT PzStatus PzSetInformationFileRaw(
    PzIoControlBlock *control_block,
    PzIoStatusBlock *iosb, u32 type,
    const void *out_buffer, u32 buffer_size);
/* Given a file handle, changes various kinds of
   information about the file according to a data
   structure of a certain type and length. */
PZ_KERNEL_EXPORT PzStatus PzSetInformationFile(
    PzHandle handle, PzIoStatusBlock *iosb, u32 type,
    const void *out_buffer, u32 buffer_size);
/* Given a device handle and
   information about the control code and input and output buffers,
   issues an I/O control command directly to the specified device. */
PZ_KERNEL_EXPORT PzStatus PzDeviceIoControl(
    PzHandle device_handle,
    PzIoStatusBlock *iosb,
    u32 ctrl_code,
    void *in_buffer,
    u32 in_buffer_size,
    void *out_buffer,
    u32 out_buffer_size
);
/* Given a handle to a block device, to a filesystem driver device and the path to a mount point,
   requests the filesystem driver to mount the filesystem at that block device (i.e., a partition). */
PZ_KERNEL_EXPORT PzStatus IoMountFileSystem(
    PzIoControlBlock *block_device, PzHandle fs_device, PzString *mount_point);
/* Loads a driver into the kernel address space, given
   the base address and length of the driver's file in memory. */
PZ_KERNEL_EXPORT PzStatus IoLoadMemoryDriver(void *file_start, u32 file_length, PzDriverObject **driver);
/* Loads a driver into the kernel address space, given
   the path of the driver file on disk. */
PZ_KERNEL_EXPORT PzStatus IoLoadDriver(const PzString *path, PzDriverObject **driver);
/* Unloads a driver from the kernel address space, given the base address of it in memory. */
PZ_KERNEL_EXPORT PzStatus IoUnloadDriver(PzDriverObject *driver_object);