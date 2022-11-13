#include <io/manager.hh>
#include <lib/malloc.hh>
#include <obj/device.hh>
#include <obj/manager.hh>
#include <obj/module.hh>
#include <obj/process.hh>
#include <sched/scheduler.hh>
#include <ipc/pipes.hh>
#include <debug.hh>

PzIoRequestPacket *IoAllocateIrp(int locations)
{
    auto irp = (PzIoRequestPacket *)PzHeapAllocate(sizeof(PzIoRequestPacket), 0);
    irp->CurrentLocation = (PzIoStackLocation *)PzHeapAllocate(sizeof(PzIoStackLocation), 0);
    return irp;
}

void IoFreeIrp(PzIoRequestPacket *irp)
{
    //ObDereferenceObject(irp->AssociatedThread);
    PzHeapFree(irp->CurrentLocation);
    PzHeapFree(irp);
}

PzStatus IoCallDriver(PzDeviceObject *device, PzIoRequestPacket *irp)
{
    int major_func = irp->CurrentLocation->MajorFunction;
    auto func_pointer = device->MajorFunctions[major_func];

    if (major_func > 0 && major_func <= MJ_FUNC_MAX && func_pointer)
        return func_pointer(device, irp);

    return STATUS_FAILED;
}

PzStatus IoCreateDevice(PzDriverObject *driver,
    u32 type, u32 flags, PzString *name, PzDeviceObject **device)
{
    if (!ObCreateNamedObject(ObGetObjectDirectory(PZ_OBJECT_DEVICE),
        (ObPointer *)device, PZ_OBJECT_DEVICE, name, 0, false))
        return STATUS_FAILED;

    (*device)->Name = name;
    (*device)->Type = type;
    (*device)->Flags = flags;
    (*device)->DriverObject = driver;

    return STATUS_SUCCESS;
}

PzStatus PzCreateFile(
    bool kernel, PzHandle *handle, const PzString *filename,
    PzIoStatusBlock *iosb, u32 access, u32 disposition)
{
    u32 flags;
    PzStatus drv_status = STATUS_FAILED;
    PzString *name = PzDuplicateString(filename);
    PzIoControlBlock *control_block;

    if (!ObCreateUnnamedObject(ObGetObjectDirectory(PZ_OBJECT_IOCB),
        (ObPointer *)&control_block, PZ_OBJECT_IOCB, 0, false)) {
        PzFreeString(name);
        return STATUS_FAILED;
    }

    PzIoRequestPacket *irp = IoAllocateIrp(1);

    if (!ObReferenceObjectByName(PZ_OBJECT_DEVICE,
        nullptr, name, (ObPointer *)&control_block->Device, false,
        &control_block->Filename))
        goto failed;

    control_block->Irp = irp;
    control_block->CurrentOffset = 0;

    irp->AssociatedThread = PsGetCurrentThread();
    irp->UserStatus = iosb;
    irp->Iocb = control_block;
    irp->CurrentLocation->MajorFunction = IRP_MJ_CREATE;
    irp->CurrentLocation->MinorFunction = 0;
    irp->CurrentLocation->Parameters.Create.Access = access;
    irp->CurrentLocation->Parameters.Create.Disposition = disposition;

    drv_status = IoCallDriver(control_block->Device, irp);
    iosb->Status = drv_status;

    if (drv_status != STATUS_SUCCESS)
        goto failed;

    flags =
        !!(access & ACCESS_READ ) * HANDLE_READ |
        !!(access & ACCESS_WRITE) * HANDLE_WRITE;

    if (!ObCreateHandle(kernel ? PZ_KPROC : nullptr, flags, handle, control_block))
        goto failed;

    //ObDereferenceObject(control_block);
    return STATUS_SUCCESS;
failed:
    IoFreeIrp(irp);
    PzFreeString(name);
    ObDereferenceObject(control_block);
    return drv_status;
}

#include <window/console.hh>

PzStatus PzReadFileRaw(
    PzIoControlBlock *control_block,
    void *buffer, PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset)
{
    PzIoStackLocation *stack_loc = control_block->Irp->CurrentLocation;
    control_block->Irp->SystemBuffer = buffer;
    control_block->Irp->UserStatus = iosb;
    stack_loc->MajorFunction = IRP_MJ_READ;
    stack_loc->MinorFunction = 0;
    stack_loc->Parameters.Read.Length = bytes;
    stack_loc->Parameters.Read.Offset = offset ? *offset : control_block->CurrentOffset;

    PzStatus drv_status = IoCallDriver(control_block->Device, control_block->Irp);
    iosb->Status = drv_status;

    if (drv_status == STATUS_SUCCESS) {
        /* Automatically advance the internal file offset by the number of bytes read */
        if (offset) control_block->CurrentOffset = *offset + bytes;
        else control_block->CurrentOffset += bytes;
    }

    return drv_status;
}

PzStatus PzReadFile(
    PzHandle handle, void *buffer,
    PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset)
{
    u32 flags;
    ObPointer object;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_ANY, &flags, handle, (ObPointer *)&object))
        return STATUS_INVALID_HANDLE;

    if (!(flags & HANDLE_READ))
        return STATUS_ACCESS_DENIED;

    if (ObGetObjectType(object) == PZ_OBJECT_APIPE)
        return PziReadPipe((PzAnonymousPipeObject*)object, buffer, bytes);
    else if (ObGetObjectType(object) == PZ_OBJECT_IOCB) {
        auto *control_block = (PzIoControlBlock*)object;
        PzStatus drv_status = PzReadFileRaw(control_block, buffer, iosb, bytes, offset);

        ObDereferenceObject(control_block);
        return drv_status;
    }

    return STATUS_INVALID_HANDLE;
}

PzStatus PzWriteFileRaw(
    PzIoControlBlock *control_block,
    const void *buffer,
    PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset)
{
    PzIoStackLocation *stack_loc = control_block->Irp->CurrentLocation;

    control_block->Irp->SystemBuffer = (void *)buffer;
    control_block->Irp->UserStatus = iosb;
    stack_loc->MajorFunction = IRP_MJ_WRITE;
    stack_loc->MinorFunction = 0;
    stack_loc->Parameters.Write.Length = bytes;
    stack_loc->Parameters.Read.Offset = offset ? *offset : control_block->CurrentOffset;

    PzStatus drv_status = IoCallDriver(control_block->Device, control_block->Irp);
    iosb->Status = drv_status;

    if (drv_status == STATUS_SUCCESS) {
        /* Automatically advance the internal file offset by the number of bytes written */
        if (offset) control_block->CurrentOffset = *offset + iosb->Information;
        else control_block->CurrentOffset += iosb->Information;
    }

    return drv_status;
}

PzStatus PzWriteFile(
    PzHandle handle, const void *buffer,
    PzIoStatusBlock *iosb,
    u32 bytes, u64 *offset)
{
    u32 flags;
    ObPointer object;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_ANY, &flags, handle, (ObPointer *)&object)) {
        if (handle == STDOUT_HANDLE || handle == STDERR_HANDLE) {
            DbgPrintStrUnformatted((const char*)buffer, bytes);
            return STATUS_SUCCESS;
        }
        return STATUS_INVALID_HANDLE;
    }

    if (!(flags & HANDLE_WRITE))
        return STATUS_ACCESS_DENIED;

    if (ObGetObjectType(object) == PZ_OBJECT_APIPE)
        return PziWritePipe((PzAnonymousPipeObject*)object, buffer, bytes);
    else if (ObGetObjectType(object) == PZ_OBJECT_IOCB) {
        auto *control_block = (PzIoControlBlock *)object;
        PzStatus drv_status = PzWriteFileRaw(control_block, buffer, iosb, bytes, offset);

        ObDereferenceObject(control_block);
        return drv_status;
    }

    return STATUS_INVALID_HANDLE;
}

PzStatus PzCloseHandle(PzHandle handle)
{
    ObCloseHandle(handle);
    return STATUS_SUCCESS;
}

#include <debug.hh>
#include <ldr/peldr.hh>
#include <core.hh>

PzStatus IoLoadMemoryDriver(
    void *file_start,
    u32 file_length,
    PzDriverObject **driver)
{
    PzStatus status;
    PzHandle mod_handle;
    PzModuleObject *mod_object;

    if (!ObCreateUnnamedObject(ObGetObjectDirectory(PZ_OBJECT_DRIVER),
        (ObPointer *)driver, PZ_OBJECT_DRIVER, 0, false))
        return STATUS_FAILED;

    if (!LdrLoadImage(PZ_KPROC, nullptr, file_start, file_length, &mod_handle)) {
        status = STATUS_FAILED;
        goto fail2;
    }

    if (!ObReferenceObjectByHandle(PZ_OBJECT_MODULE, nullptr, mod_handle, (ObPointer *)&mod_object)) {
        status = STATUS_FAILED;
        goto fail2;
    }

    (*driver)->AssociatedModule = mod_object;
    PzSymlinkObject *symlink;

    if (mod_object->EntryPointAddress(*driver) != STATUS_SUCCESS)
        goto fail;

    if (auto init = (*driver)->InitFunction) {
        status = init(*driver);
        if (status != STATUS_SUCCESS)
            goto fail;
    }

    ObCreateSymlink(ObGetObjectDirectory(PZ_OBJECT_DRIVER),
        &symlink, (*driver)->Name, 0, *driver);
    ObDereferenceObject(mod_object);
    return STATUS_SUCCESS;

fail:
    ObDereferenceObject(mod_object);

fail2:
    ObDereferenceObject(*driver);
    return status;
}

PzStatus IoLoadDriver(const PzString *path, PzDriverObject **driver)
{
    PzHandle handle;
    PzIoStatusBlock status;

    if (PzStatus cf_status = PzCreateFile(
        true, &handle, path,
        &status, ACCESS_READ, OPEN_EXISTING))
        return cf_status;

    /* Retrieve the file's length so we can allocate a buffer of sufficient size for it. */
    PzFileInformationBasic basic;
    if (PzStatus q_status = PzQueryInformationFile(handle, &status,
        FILE_INFORMATION_BASIC, &basic,
        sizeof(PzFileInformationBasic))) {
        PzCloseHandle(handle);
        return q_status;
    }

    /* Allocate a buffer for the file and read it */
    void *buffer = MmVirtualAllocateMemory(nullptr, basic.Size, PAGE_READWRITE, nullptr);

    if (!buffer) {
        PzCloseHandle(handle);
        return STATUS_ALLOCATION_FAILED;
    }

    if (PzStatus rd_status = PzReadFile(handle, buffer, &status, basic.Size, nullptr)) {
        PzCloseHandle(handle);
        MmVirtualFreeMemory(buffer, basic.Size);
        return rd_status;
    }

    PzStatus load_status = IoLoadMemoryDriver(buffer, basic.Size, driver);

    /* Free buffer */
    MmVirtualFreeMemory(buffer, basic.Size);
    PzCloseHandle(handle);
    return load_status;
}

PzStatus PzQueryInformationFileRaw(
    PzIoControlBlock *control_block, PzIoStatusBlock *iosb,
    u32 type, void *out_buffer, u32 buffer_size)
{
    PzIoStackLocation *stack_loc = control_block->Irp->CurrentLocation;
    control_block->Irp->UserStatus = iosb;
    stack_loc->MajorFunction = IRP_MJ_QUERY_INFO_FILE;
    stack_loc->MinorFunction = type;
    stack_loc->Parameters.QueryInfoFile.Length = buffer_size;
    control_block->Irp->SystemBuffer = out_buffer;
    return IoCallDriver(control_block->Device, control_block->Irp);
}


PzStatus PzQueryInformationFile(
    PzHandle handle, PzIoStatusBlock *iosb,
    u32 type, void *out_buffer, u32 buffer_size)
{
    PzIoControlBlock *control_block;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_IOCB, nullptr, handle, (ObPointer *)&control_block))
        return STATUS_INVALID_HANDLE;

    PzStatus drv_status =
        PzQueryInformationFileRaw(control_block, iosb, type, out_buffer, buffer_size);

    ObDereferenceObject(control_block);
    return drv_status;
}

PzStatus PzSetInformationFileRaw(
    PzIoControlBlock *control_block, PzIoStatusBlock *iosb,
    u32 type, const void *out_buffer, u32 buffer_size)
{
    PzIoStackLocation *stack_loc = control_block->Irp->CurrentLocation;
    control_block->Irp->UserStatus = iosb;
    stack_loc->MajorFunction = IRP_MJ_SET_INFO_FILE;
    stack_loc->MinorFunction = type;
    stack_loc->Parameters.SetInfoFile.Length = buffer_size;
    control_block->Irp->SystemBuffer = (void*)out_buffer;
    return IoCallDriver(control_block->Device, control_block->Irp);
}

PzStatus PzSetInformationFile(
    PzHandle handle, PzIoStatusBlock *iosb,
    u32 type, const void *out_buffer, u32 buffer_size)
{
    PzIoControlBlock *control_block;
    if (!ObReferenceObjectByHandle(PZ_OBJECT_IOCB, nullptr, handle, (ObPointer *)&control_block))
        return STATUS_INVALID_HANDLE;

    PzStatus drv_status = PzSetInformationFileRaw(
        control_block, iosb, type, out_buffer, buffer_size);

    ObDereferenceObject(control_block);
    return drv_status;
}

PzStatus PzDeviceIoControl(
    PzHandle device_handle,
    PzIoStatusBlock *iosb,
    u32 ctrl_code,
    void *in_buffer,
    u32 in_buffer_size,
    void *out_buffer,
    u32 out_buffer_size
)
{
    PzIoControlBlock *control_block;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_IOCB, nullptr, device_handle, (ObPointer *)&control_block))
        return STATUS_INVALID_HANDLE;

    PzIoStackLocation *stack_loc = control_block->Irp->CurrentLocation;
    control_block->Irp->UserStatus = iosb;
    stack_loc->MajorFunction = IRP_MJ_DEVICE_IOCTL;
    stack_loc->MinorFunction = ctrl_code;
    stack_loc->Parameters.IoControl.Code = ctrl_code;
    stack_loc->Parameters.IoControl.InputBuffer = in_buffer;
    stack_loc->Parameters.IoControl.InputBufLength = in_buffer_size;
    stack_loc->Parameters.IoControl.OutputBufLength = out_buffer_size;
    control_block->Irp->SystemBuffer = (void *)out_buffer;

    PzStatus drv_status = IoCallDriver(control_block->Device, control_block->Irp);
    ObDereferenceObject(control_block);
    return drv_status;
}

PzStatus IoMountFileSystem(PzIoControlBlock *block_device, PzHandle fs_device, PzString *mount_point)
{
    PzDeviceObject *fs;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_DEVICE, nullptr,
        fs_device, (ObPointer *)&fs)) {
        return STATUS_INVALID_HANDLE;
    }

    if (fs->Type != DEVICE_FS_DRIVER) {
        ObDereferenceObject(fs);
        return STATUS_WRONG_DEVICE_TYPE;
    }

    PzIoRequestPacket *mount_irp = IoAllocateIrp(1);
    mount_irp->CurrentLocation->MajorFunction = IRP_MJ_MOUNT_FS;
    mount_irp->CurrentLocation->MinorFunction = 0;
    mount_irp->CurrentLocation->Parameters.MountFs.MountPoint = mount_point;
    mount_irp->CurrentLocation->Parameters.MountFs.BlockDevice = block_device;

    if (PzStatus status = IoCallDriver(fs, mount_irp)) {
        IoFreeIrp(mount_irp);
        ObDereferenceObject(fs);
        return status;
    }

    IoFreeIrp(mount_irp);
    ObDereferenceObject(fs);
    return STATUS_SUCCESS;
}

PzStatus IoUnmountFileSystem()
{
    return STATUS_SUCCESS;
}

PzStatus IoUnloadDriver(PzDriverObject *driver_object)
{
    PzStatus status = driver_object->UnloadFunction(driver_object);

    if (status != STATUS_SUCCESS)
        return status;

    LdrUnloadImage(driver_object->AssociatedModule);
    ObDereferenceObject(driver_object);
    return STATUS_SUCCESS;
}