#include <boot.hh>
#include <core.hh>
#include <debug.hh>
#include <serial.hh>
#include <mm/physical.hh>
#include <mm/virtual.hh>
#include <x86/gdt.hh>
#include <x86/idt.hh>
#include <lib/malloc.hh>
#include <ldr/peldr.hh>
#include <io/manager.hh>
#include <sched/scheduler.hh>
#include <pci/pci.hh>
#include <acpi/tables.hh>
#include <x86/apic.hh>
#include <panic.hh>
#include <gfx/link.hh>

//#define PRIZM_DEBUG_BUILD

#if 0
/* Stack trace test, ignore */
int How(), Did(), You(), Even(), DivideByZero();

int How()
{
    return Did() + 1;
}

int Did()
{
    return You() + 2;
}

int You()
{
    return Even() + 3;
}

int Even()
{
    return DivideByZero() + 4;
}

int DivideByZero()
{
    volatile int a = 5;
    volatile int b = 0;
    DbgPrintStr("Dividing by zero... %i\r\n", a / b);
    return 0;
}
#endif

void PzCreateSystemDirectory()
{
    static PzString device = PZ_CONST_STRING("Device\\Vol0");
    static PzString sys_dir_name = PZ_CONST_STRING("SystemDir");

    ObPointer device_ptr;
    if (!ObReferenceObjectByName(PZ_OBJECT_DEVICE, nullptr, &device, &device_ptr, false, nullptr))
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_MOUNT, "ObReferenceObjectByName failed");

    PzSymlinkObject *symlink;
    if (!ObCreateSymlink(nullptr, &symlink, &sys_dir_name, 0, device_ptr))
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_MOUNT, "ObCreateSymlink failed");
}

int PzInitThread(void *param)
{
#ifdef PRIZM_DEBUG_BUILD
    DbgStartListener();
#endif

    auto *boot_info = (KernelBootInfo *)param;
    LdrInitializeLoader(boot_info);
    PciScanAll();

    /* Loading crucial driver files provided by bootloader */
    PzDriverObject *driver;

    for (int i = 0; i < boot_info->BootstrapDriverCount; i++) {
        PzStatus result = IoLoadMemoryDriver(boot_info->BootstrapDriverBases[i], -1u, &driver);

        DbgPrintStr("[PzInitThread] Loading bootstrap driver %i: returned %i\r\n",
            i, result);

        if (result != STATUS_SUCCESS)
            PzPanic(nullptr, PANIC_REASON_FAILED_INIT_DRIVER,
                "Failed to load crucial driver");
    }

    /* Mounting FAT32 root volume on IDE 0:0 */
    PzHandle handle;
    PzIoStatusBlock io_status;
    static PzString part_name = PZ_CONST_STRING("Device\\Disk\\0\\0");
    static PzString fat_driver = PZ_CONST_STRING("Device\\FsFat");
    static PzString fat_volume = PZ_CONST_STRING("Vol0");

    if (PzCreateFile(true, &handle, &part_name, &io_status,
        ACCESS_READ | ACCESS_WRITE, OPEN_EXISTING) != STATUS_SUCCESS)
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_MOUNT, "Failed to create handle to partition");

    ObPointer fat_dev_obj;
    PzHandle fs_device, file_handle;

    if (!ObReferenceObjectByName(PZ_OBJECT_DEVICE, nullptr,
        &fat_driver, &fat_dev_obj, false, nullptr))
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_MOUNT,
            "Failed to mount root volume: failed to locate fs driver device");

    if (!ObCreateHandle(PZ_KPROC, 0, &fs_device, fat_dev_obj))
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_MOUNT,
            "Failed to mount root volume: failed to create fs driver device handle");

    PzIoControlBlock *disk_iocb;

    if (!ObReferenceObjectByHandle(PZ_OBJECT_IOCB, nullptr, handle, (ObPointer *)&disk_iocb))
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_MOUNT,
            "Failed to mount root volume: failed to dereference volume handle");

    if (IoMountFileSystem(disk_iocb, fs_device, &fat_volume) != STATUS_SUCCESS)
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_MOUNT, "Failed to mount root volume");

    PzCreateSystemDirectory();

    if (PzStatus status = GfxInitializeDriver())
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_DRIVER, "Failed to load graphics driver: %i", status);

    /* Creating the first usermode process */
    DbgPrintStr("\r\n[PzInitThread] Creating init process...\r\n");

    static PzString usermode = PZ_CONST_STRING("SystemDir\\pzinit.exe");
    static PzString process_name = PZ_CONST_STRING("pzinit.exe");

    PzProcessCreationParams params1 = {
        &process_name,
        &usermode,
        false,
        INVALID_HANDLE_VALUE,
        INVALID_HANDLE_VALUE,
        INVALID_HANDLE_VALUE
    };

    PzStatus creation = PsCreateProcess(true, &handle, &params1);

    DbgPrintStr("[PzInitThread] PzCreateProcess returned: %i\r\n", creation);

    if (creation != STATUS_SUCCESS)
        PzPanic(nullptr, PANIC_REASON_FAILED_INIT_PROCESS, "Failed to create init process");

    return 0;
}

void AttachDebugger()
{
#ifdef PRIZM_DEBUG_BUILD
    DbgPrintStr("[PzKernelInit] Awaiting debugger attachment...\r\n");
    DbgPrintStr("[PzKernelInit] Received attachment=0x%08x\r\n", DbgRead32());
    DbgWrite32(CONNECT_SEQUENCE);
#endif
}

extern "C" void HalEnableSSE();

PZ_KERNEL_EXPORT void PzKernelInit(KernelBootInfo *boot)
{
    HalEnableSSE();
    SerialInitializePort(1, 115200);

    AttachDebugger();
    DbgPrintStr(
        "[PzKernelInit] KernelBootInfo "
        "{KernelPhysicalStart=%p, KernelPhysicalEnd=%p, MemMapPointer=%p}\r\n",
        boot->KernelPhysicalStart,
        boot->KernelPhysicalEnd,
        boot->MemMapPointer);

    MmPhysicalInitializeState(boot);
    MmVirtualInitBootPageTable(boot);

    HalGdtInitialize();
    HalIdtInitialize();
    PzHeapInitialize();

    //AcpiInitialize();
    //AcpiInitializeTables();
    ObInitializeObjManager();
    //HalApicInitialize();
    SchInitializeScheduler(PzInitThread, boot);

    for (;;);
}