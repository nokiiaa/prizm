#include "driver.hh"

VbeVideoModeData *VbeData;
void *VbeFramebuffer;
u32 VbeFramebufferSize;
PzSpinlock TestLock;

PzStatus DriverDispatch(PzDeviceObject *device, PzIoRequestPacket *irp)
{
    switch (irp->CurrentLocation->MajorFunction) {
        case IRP_MJ_CREATE: {
			if (!Utf8CompareRawStrings(
				irp->Iocb->Filename->Buffer, 
				irp->Iocb->Filename->Size,
				"Framebuffer", 11)) {
				irp->Iocb->Context = CREATE_TYPE_FRAMEBUFFER;
				return STATUS_SUCCESS;
			}
			return STATUS_UNSUPPORTED_FUNCTION;
        }

		case IRP_MJ_READ: {
			if (irp->Iocb->Context != CREATE_TYPE_FRAMEBUFFER)
				return STATUS_UNSUPPORTED_FUNCTION;

			u32 offset = irp->CurrentLocation->Parameters.Read.Offset;
			int read   = irp->CurrentLocation->Parameters.Read.Length;

			if (offset + read > VbeFramebufferSize)
				if ((read = VbeFramebufferSize - offset) < 0)
					read = 0;

			MemCopy(irp->SystemBuffer,
				(u8 *)VbeFramebuffer + offset,
				read);

			irp->UserStatus->Information = read;
			return STATUS_SUCCESS;
		}

		case IRP_MJ_WRITE: {
			if (irp->Iocb->Context != CREATE_TYPE_FRAMEBUFFER)
				return STATUS_UNSUPPORTED_FUNCTION;

			u32 offset = irp->CurrentLocation->Parameters.Write.Offset;
			int write  = irp->CurrentLocation->Parameters.Write.Length;

			if (offset + write >= VbeFramebufferSize)
				if ((write = VbeFramebufferSize - offset) < 0)
					write = 0;

			MemCopy(
				(u8 *)VbeFramebuffer + offset,
				irp->SystemBuffer, write);

			irp->UserStatus->Information = write;
			return STATUS_SUCCESS;
		}

		case IRP_MJ_DEVICE_IOCTL:
			#define GFX_FRAMEBUFFER_DATA 1

			if (irp->CurrentLocation->Parameters.IoControl.Code == GFX_FRAMEBUFFER_DATA) {
				struct ScreenData {
					u32 Width, Height;
					u32 Pitch, PixelSize;
				} data = {
					VbeData->Width,
					VbeData->Height,
					VbeData->Pitch,
					(u32)VbeData->BitsPerPixel / 8
				};

				MemCopy(irp->SystemBuffer, &data, sizeof(ScreenData));
			}
			return STATUS_SUCCESS;
	}
    return STATUS_UNSUPPORTED_FUNCTION;
}

PzStatus DriverInitialize(PzDriverObject *driver)
{
	VbeData = (VbeVideoModeData*)MmVirtualMapPhysical(
		nullptr, 0x1000, sizeof(VbeVideoModeData), PAGE_READWRITE);

	if (!VbeData)
		return STATUS_ALLOCATION_FAILED;

	VbeFramebufferSize = VbeData->Pitch * VbeData->Height * VbeData->BitsPerPixel / 8;

	VbeFramebuffer = MmVirtualMapPhysical(nullptr,
		VbeData->Framebuffer, VbeFramebufferSize, PAGE_READWRITE);

	if (!VbeFramebuffer) {
		MmVirtualFreeMemory(VbeData, sizeof(VbeVideoModeData));
		return STATUS_ALLOCATION_FAILED;
	}

	return STATUS_SUCCESS;
}

PzStatus DriverUnload(PzDriverObject *driver)
{
	MmVirtualFreeMemory(VbeData, sizeof(VbeVideoModeData));
	MmVirtualFreeMemory(VbeFramebuffer, VbeFramebufferSize);

	return STATUS_SUCCESS;
}

extern "C" PzStatus DriverEntry(PzDriverObject *driver)
{
    static PzString DeviceName = PZ_CONST_STRING("Gfx");
    static PzString DriverName = PZ_CONST_STRING("Gfx");

    driver->Name = &DriverName;
    driver->InitFunction = DriverInitialize;
    driver->UnloadFunction = DriverUnload;

    PzDeviceObject *device;

    if (IoCreateDevice(driver, DEVICE_GRAPHICS, 0, &DeviceName, &device) != STATUS_SUCCESS)
        return STATUS_FAILED;

    for (int i = 0; i < MJ_FUNC_MAX; i++)
        device->MajorFunctions[i] = DriverDispatch;

    return STATUS_SUCCESS;
}