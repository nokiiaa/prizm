#include "driver.hh"
#include <mm/virtual.hh>
#include <mm/physical.hh>
#include <lib/util.hh>
#include <sched/scheduler.hh>

int IoPrimary, IoPrimaryCtrl;
int IoSecondary, IoSecondaryCtrl;
int BusMasterBase;
PzHandle IoComplete, IoLock;

PzStatus DriverInitialize(PzDriverObject *driver)
{
    if (!PciLocateDeviceByClass(PCI_CLASS_MASS_STORAGE, PCI_SUB_MSC_IDE_CTRL, &BmDevice))
        return STATUS_DEVICE_UNAVAILABLE;

    /* Enable I/O and bus master for IDE controller device */
    u16 command = PciReadConfigDevice16(&BmDevice, PCI_COMMAND_OFFSET);
    PciWriteConfigDevice16(&BmDevice, PCI_COMMAND_OFFSET, command | 5);
    u32 bar0 = PciReadBarDevice(&BmDevice, 0) & ~0xF;
    u32 bar1 = PciReadBarDevice(&BmDevice, 1) & ~0xF;
    u32 bar2 = PciReadBarDevice(&BmDevice, 2) & ~0xF;
    u32 bar3 = PciReadBarDevice(&BmDevice, 3) & ~0xF;
    u32 bar4 = PciReadBarDevice(&BmDevice, 4) & ~0xF;
    IoPrimary = bar0 ? bar0 : 0x1F0;
    IoPrimaryCtrl = bar1 ? bar1 : 0x3F6;
    IoSecondary = bar2 ? bar2 : 0x170;
    IoSecondaryCtrl = bar3 ? bar3 : 0x376;
    BusMasterBase = bar4;

    /* Preallocated DMA buffer for small transfers */
    Dma64KBuffer = (u8 *)MmAllocateDmaMemory(65536, 16, &Dma64KBufferPhysical, PAGE_READWRITE);

    if (!Dma64KBuffer)
        return STATUS_ALLOCATION_FAILED;

    /* 64K-aligned PRDT buffer */
    Prdt = (BmPhysRegDesc *)MmAllocateDmaMemory(
        sizeof(BmPhysRegDesc) * 4096, 16, &PrdtPhysical, PAGE_READWRITE);

    if (!Prdt) {
        MmFreeDmaMemory(Dma64KBuffer, 16, 65536);
        return STATUS_ALLOCATION_FAILED;
    }

    PzStatus status;

    /* Set up synchronization objects */
    if (status = PsCreateMutex(&IoLock, PZ_KPROC, nullptr))
        goto fail_free;
    if (status = PsCreateEvent(&IoComplete, PZ_KPROC, nullptr))
        goto fail_free;

    /* Set up IRQs */
    PzInstallIrqHandler(14, IrqNotifyDmaReady);
    PzInstallIrqHandler(15, IrqNotifyDmaReady);

    if (status = IdeDetectDevices())
        goto fail_free;

    return STATUS_SUCCESS;

fail_free:
    MmFreeDmaMemory(Dma64KBuffer, 16, 65536);
    MmFreeDmaMemory(Prdt, 16, sizeof(BmPhysRegDesc) * 4096);
    return status;
}