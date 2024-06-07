#include "driver.hh"

extern "C" PzStatus DriverEntry(PzDriverObject *driver)
{
    static PzString DeviceName = PZ_CONST_STRING("Disk");
    static PzString DriverName = PZ_CONST_STRING("Disk");
    driver->Name = &DriverName;
    driver->InitFunction = DriverInitialize;
    driver->UnloadFunction = nullptr;
    PzDeviceObject *device;

    if (IoCreateDevice(driver, DEVICE_BLOCK, 0, &DeviceName, &device) != STATUS_SUCCESS)
        return STATUS_FAILED;

    for (int i = 0; i < MJ_FUNC_MAX; i++)
        device->MajorFunctions[i] = DriverDispatch;

    return STATUS_SUCCESS;
}