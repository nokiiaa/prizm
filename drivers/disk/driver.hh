#include <core.hh>
#include <defs.hh>
#include <io/manager.hh>
#include <pci/pci.hh>
#include <debug.hh>
#include <lib/util.hh>

#pragma pack(push, 1)
struct BmPhysRegDesc
{
    u32 PhysicalAddress;
    u16 ByteCount;
    u16 Reserved;
};

struct MbrDisk
{
    int BytesPerSector;
    int Number;
    int SelectedPartition;
    struct PartitionEntry
    {
        u8 Status;
        u8 StartHead, StartSector, StartCylinder;
        u8 Type;
        u8 EndHead, EndSector, EndCylinder;
        u32 StartLba;
        u32 SectorCount;
    } Partitions[4];
};

struct IdeDeviceInfo
{
    bool SupportsLba48, Present;
    char Model[41];
};
#pragma pack(pop)

extern int IoPrimary, IoPrimaryCtrl;
extern int IoSecondary, IoSecondaryCtrl;
extern int BusMasterBase;
extern u8 *Dma64KBuffer;
extern PciDevice BmDevice;
extern BmPhysRegDesc *Prdt;
extern PzHandle IoComplete, IoLock;
extern uptr PrdtPhysical, Dma64KBufferPhysical;

PzStatus IdeDetectDevices();
PzStatus DriverDispatch(
    PzDeviceObject *device,
    PzIoRequestPacket *irp);
PzStatus DriverInitialize(PzDriverObject *driver);
void IrqNotifyDmaReady(CpuInterruptState *state);
PzStatus DriverInitialize(PzDriverObject *driver);