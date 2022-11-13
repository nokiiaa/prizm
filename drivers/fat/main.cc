#include <io/manager.hh>
#include "fat.hh"

PzStatus DriverFsDispatch(PzDeviceObject *device, PzIoRequestPacket *irp);

PzStatus DriverMountDispatch(PzDeviceObject *device, PzIoRequestPacket *irp)
{
    if (irp->CurrentLocation->MajorFunction != IRP_MJ_MOUNT_FS)
        return STATUS_UNSUPPORTED_FUNCTION;

    auto *params = &irp->CurrentLocation->Parameters.MountFs;
    PzString *mount_point = PzDuplicateString(params->MountPoint);

    if (!mount_point)
        return STATUS_ALLOCATION_FAILED;

    PzDeviceObject *fs_device;

    if (IoCreateDevice(device->DriverObject, DEVICE_FS_MOUNTED,
        0, mount_point, &fs_device) != STATUS_SUCCESS)
    {
        PzFreeString(mount_point);
        return STATUS_FAILED;
    }

    FatVolumeContext *volume = new FatVolumeContext;

    if (!volume) {
        PzFreeString(mount_point);
        return STATUS_ALLOCATION_FAILED;
    }

    volume->BlockDevice = params->BlockDevice;

    u64 offset = 0;
    PzIoStatusBlock iosb;

    if (PzStatus status = PzReadFileRaw(volume->BlockDevice,
        &volume->Bpb, &iosb, sizeof volume->Bpb, &offset)) {
        delete volume;
        PzFreeString(mount_point);
        return status;
    }

    volume->FatStartOffset = volume->Bpb.BytesPerSector * u64(volume->Bpb.ReservedSectors);
    volume->DataStartOffset = volume->FatStartOffset + volume->Bpb.BytesPerSector
        * u64(volume->Bpb.SectorsPerFat32 * volume->Bpb.FatCount);
    volume->MountPoint = mount_point;
    volume->ClusterSize = volume->Bpb.BytesPerSector * volume->Bpb.SectorsPerCluster;
    fs_device->Context = (uptr)volume;

    for (int i = 0; i < MJ_FUNC_MAX; i++)
        fs_device->MajorFunctions[i] = DriverFsDispatch;

    return STATUS_SUCCESS;
}

PzStatus DriverFsDispatch(PzDeviceObject *device, PzIoRequestPacket *irp)
{
    switch (irp->CurrentLocation->MajorFunction)
    {
    case IRP_MJ_CREATE: {
        FatFileContext *file = new FatFileContext();

        if (!file)
            return STATUS_ALLOCATION_FAILED;

        if (PzStatus status = FatInitFile(
            (FatVolumeContext *)device->Context, irp->Iocb->Filename, file)) {
            delete file;
            return status;
        }

        irp->Iocb->Context = (uptr)file;
        return STATUS_SUCCESS;
    }

    case IRP_MJ_READ: {
        u32 read;
        PzStatus status = FatReadFile(
            (FatVolumeContext *)device->Context,
            (FatFileContext *)irp->Iocb->Context,
            irp->Iocb, irp->SystemBuffer,
            irp->CurrentLocation->Parameters.Read.Offset,
            irp->CurrentLocation->Parameters.Read.Length,
            read);
        irp->UserStatus->Information = read;
        return status;
    }

    case IRP_MJ_WRITE:
        return STATUS_UNSUPPORTED_FUNCTION;

    case IRP_MJ_CLOSE:
        if (!irp->Iocb->Context)
            return STATUS_FAILED;

        delete (FatFileContext *)irp->Iocb->Context;
        return STATUS_SUCCESS;

    case IRP_MJ_QUERY_INFO_FILE:
        switch (irp->CurrentLocation->MinorFunction)
        {
        case FILE_INFORMATION_BASIC:
            ((PzFileInformationBasic *)irp->SystemBuffer)->Size =
                ((FatFileContext *)irp->Iocb->Context)->Size;
            return STATUS_SUCCESS;
        }
        break;
    }
    return STATUS_UNSUPPORTED_FUNCTION;
}

PzStatus DriverInitialize(PzDriverObject *driver)
{
    return STATUS_SUCCESS;
}

#include <debug.hh>

extern "C" PzStatus DriverEntry(PzDriverObject *driver)
{
    static PzString DeviceName = PZ_CONST_STRING("FsFat");
    static PzString DriverName = PZ_CONST_STRING("FsFat");

    driver->Name = &DriverName;
    driver->InitFunction = DriverInitialize;
    driver->UnloadFunction = nullptr;

    PzDeviceObject *device;
    DbgPrintStr("[FAT] IoCreateDevice returned: %i\r\n",
        IoCreateDevice(driver, DEVICE_FS_DRIVER, 0, &DeviceName, &device));

    for (int i = 0; i < MJ_FUNC_MAX; i++)
        device->MajorFunctions[i] = DriverMountDispatch;

    return STATUS_SUCCESS;
}