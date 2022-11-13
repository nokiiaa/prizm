#include "driver.hh"
#include <sched/scheduler.hh>
#include <mm/physical.hh>
#include <lib/string.hh>

#define	ATA_DATA         0
#define	ATA_SR_ERR       1
#define	ATA_FEATURES     1
#define	ATA_SEC_COUNT    2
#define	ATA_LBA_LO       3
#define	ATA_LBA_MID      4
#define	ATA_LBA_HI       5
#define	ATA_DRIVE_SELECT 6
#define	ATA_STATUS       7
#define	ATA_COMMAND      7

#define ATA_SR_BSY 0x80
#define ATA_SR_DRQ 0x08

#define ATA_PACKET                0xA0
#define ATA_IDENTIFY_PACKET       0xA1
#define ATA_IDENTIFY              0xEC
#define	ATA_PIO_READ_SECTORS_EXT  0x24
#define ATA_DMA_READ_SECTORS      0xC8
#define	ATA_DMA_READ_SECTORS_EXT  0x25
#define	ATA_PIO_WRITE_SECTORS_EXT 0x34
#define ATA_DMA_WRITE_SECTORS     0xCA
#define	ATA_DMA_WRITE_SECTORS_EXT 0x35

#define ATA_IDS_DEVICETYPE   0
#define ATA_IDS_CYLINDERS    2
#define ATA_IDS_HEADS        6
#define ATA_IDS_SECTORS      12
#define ATA_IDS_SERIAL       20
#define ATA_IDS_MODEL        54
#define ATA_IDS_CAPABILITIES 98
#define ATA_IDS_FIELDVALID   106
#define ATA_IDS_MAX_LBA      120
#define ATA_IDS_COMMANDSETS  164
#define ATA_IDS_MAX_LBA_EXT  200

#define	BM_COMMAND   0x0
#define	BM_STATUS    0x2
#define	BM_PRDT_BASE 0x4

static u8 IdeRead8(int offset, bool secondary = false)
{
    return HalPortIn8(offset + (secondary ? IoSecondary : IoPrimary));
}

static void IdeWrite8(int offset, u8 value, bool secondary = false)
{
    HalPortOut8(offset + (secondary ? IoSecondary : IoPrimary), value);
}

static u32 IdeRead32(int offset, bool secondary = false)
{
    return HalPortIn32(offset + (secondary ? IoSecondary : IoPrimary));
}

static u8 IdeBmRead8(int offset, bool secondary = false)
{
    return HalPortIn8(BusMasterBase + offset + secondary * 8);
}

static void IdeCtrlWrite8(u8 value, bool secondary = false)
{
    HalPortOut8(secondary ? IoSecondaryCtrl : IoPrimaryCtrl, value);
}

static void IdeBmWrite8(int offset, u8 value, bool secondary = false)
{
    HalPortOut8(BusMasterBase + offset + secondary * 8, value);
}

static void IdeBmWrite32(int offset, u32 value, bool secondary = false)
{
    HalPortOut32(BusMasterBase + offset + secondary * 8, value);
}

PciDevice BmDevice;
BmPhysRegDesc *Prdt;
u8 *Dma64KBuffer;
uptr PrdtPhysical, Dma64KBufferPhysical;

void AtaWaitBusy(bool secondary)
{
    /* Wait for BSY to become zero. */
    while (IdeRead8(ATA_STATUS, secondary) & ATA_SR_BSY);
}

void IrqNotifyDmaReady(CpuInterruptState *state)
{
    PsSetEvent(IoComplete);
}

IdeDeviceInfo IdeDevices[4];

PzStatus IdeDetectDevices()
{
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            static u8 ide_buffer[512];
            u8 err = 0, type = 0, status;

            IdeWrite8(ATA_DRIVE_SELECT, 0xA0 | (j << 4), i);
            IdeWrite8(ATA_COMMAND, ATA_IDENTIFY, i);

            /* Device does not exist */
            status = IdeRead8(ATA_STATUS, i);
            if (status == 0)
                continue;

            for (;;) {
                status = IdeRead8(ATA_STATUS, i);
                if (status & ATA_SR_ERR) { err = 1; break; }
                if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
                    break;
            }

            if (err != 0) {
                u8 cl = IdeRead8(ATA_LBA_LO, i);
                u8 ch = IdeRead8(ATA_LBA_MID, i);

                if (cl == 0x14 && ch == 0xEB ||
                    cl == 0x69 && ch == 0x96)
                    type = 1;
                else
                    continue;

                IdeWrite8(ATA_COMMAND, ATA_IDENTIFY_PACKET, i);
            }

            IdeCtrlWrite8(0x80, i);

            for (int k = 0; k < 128; k++)
                ((u32 *)ide_buffer)[k] = IdeRead32(ATA_DATA, i);

            u16 signature = *(u16 *)(ide_buffer + ATA_IDS_DEVICETYPE);
            u16 capabilities = *(u16 *)(ide_buffer + ATA_IDS_CAPABILITIES);
            u32 cmd_sets = *(u32 *)(ide_buffer + ATA_IDS_COMMANDSETS);
            bool supports_lba48 = cmd_sets & 1 << 26;
            u32 size = supports_lba48 ?
                *(u32 *)(ide_buffer + ATA_IDS_MAX_LBA_EXT) :
                *(u32 *)(ide_buffer + ATA_IDS_MAX_LBA);

            char model[40 + 1] = { 0 };

            for (int k = 0; k < 40; k += 2) {
                model[k + 0] = ide_buffer[ATA_IDS_MODEL + k + 1];
                model[k + 1] = ide_buffer[ATA_IDS_MODEL + k];
            }

            MemCopy(&IdeDevices[i * 2 + j].Model, model, sizeof(model));
            IdeDevices[i * 2 + j].SupportsLba48 = supports_lba48;

            DbgPrintStr("(IDE %i:%i) model=%s\r\n", i, j, model);
            DbgPrintStr("(IDE %i:%i) supports lba 48=%i\r\n", i, j, supports_lba48);
        }
    }

    return STATUS_SUCCESS;
}

#include <processor.hh>

bool AtaDmaTransfer(
    MbrDisk *disk, void *buffer, bool write,
    u64 start, u32 bytes, u32 &transferred)
{
    transferred = 0;

    //DbgPrintStr("AtaDmaTransfer 1 (%p%p %i bytes)\r\n", (u32)(start >> 32), (u32)(start), bytes);

    if (!disk->BytesPerSector)
        return false;

    if (!bytes)
        return true;

    bool errors = false;
    bool lba48 = IdeDevices[disk->Number].SupportsLba48;

    /* The starting offset is outside of the accessible range:
       2^28 and 2^48 sectors for (non-)supporting drives respectively */
    if (!lba48 && (start + bytes / disk->BytesPerSector) >= 1ull << 28)
        return false;
    else if ((start + bytes / disk->BytesPerSector) >= 1ull << 48)
        return false;

    //DbgPrintStr("AtaDmaTransfer 2\r\n");


    u8 *byte_buffer = (u8 *)buffer;
    int max_sectors = lba48 ? 65535 : 255;
    u32 max_size = max_sectors * disk->BytesPerSector;

    bool secondary = !!(disk->Number & 2);
    int disk_select = disk->Number & 1;

    /* If byte count exceeds the maximum size per read,
       read it in chunks */
    if (bytes > max_size) {
        u32 ptrans;

        while (bytes > max_size) {
            errors |= AtaDmaTransfer(disk, byte_buffer, write, start, max_size, ptrans);
            byte_buffer += max_size;
            start += max_size;
            transferred += ptrans;
            bytes -= max_size;
        }

        errors |= AtaDmaTransfer(disk, byte_buffer, write, start, bytes, ptrans);
        transferred += ptrans;
        return errors;
    }

    PsWaitForObject(IoLock);

    bool allocated = false;
    u8 *active_buffer;

    if (bytes < 65536) {
        active_buffer = Dma64KBuffer;
    }
    else {
        allocated = true;
        active_buffer = (u8 *)MmAllocateDmaMemory(bytes, 16, nullptr, PAGE_READWRITE);

        if (!active_buffer) {
            PsReleaseMutex(IoLock);
            return false;
        }
    }

    u64 start_sector = start / disk->BytesPerSector;
    int skipped_bytes = start % disk->BytesPerSector;
    u16 sector_count = ALIGN(bytes + skipped_bytes, disk->BytesPerSector) / disk->BytesPerSector;
    u32 dma_bytes = sector_count * disk->BytesPerSector;
    uptr tmp_dma = MmVirtualToPhysical(active_buffer, 0);

    for (int i = 0; dma_bytes && i < 4096; i++) {
        int csize = Min(dma_bytes, 65536u);
        uptr bb = tmp_dma;

        /* Split transfers across 64K boundaries into two separate ones */
        if ((bb & 0xFFFF) + csize > 0x10000)
            csize = 0x10000 - (bb & 0xFFFF);

        Prdt[i].PhysicalAddress = u32(tmp_dma);

        /* 65536 bytes will wrap around to 0 */
        Prdt[i].ByteCount = csize & 0xFFFF;
        tmp_dma += csize;

        /* Set bit 15 if there is no more data left after this */
        Prdt[i].Reserved = !(dma_bytes -= csize) << 15;
    }

    /* Clear start/stop bit */
    IdeBmWrite8(BM_COMMAND, IdeBmRead8(BM_COMMAND, secondary) & ~1, secondary);

    /* Set PRDT address */
    IdeBmWrite32(BM_PRDT_BASE, PrdtPhysical, secondary);

    /* Clear interrupt and error bits of status register */
    IdeBmWrite8(BM_STATUS, IdeBmRead8(BM_STATUS, secondary) | 2 | 4, secondary);

    /* Put controller into read/write mode (set/clear bit 3) */
    IdeBmWrite8(BM_COMMAND,
        IdeBmRead8(BM_COMMAND, secondary) & ~(1 << 3) | !write << 3, secondary);

    /* Select drive */
    IdeWrite8(ATA_DRIVE_SELECT, 0x40 | disk_select << 4 | !lba48 * (start_sector >> 24), secondary);

    if (lba48) {
        /* High 8 bits of sector count */
        IdeWrite8(ATA_SEC_COUNT, sector_count >> 8, secondary);

        /* LBA bytes 3, 4, 5 */
        IdeWrite8(ATA_LBA_LO, start_sector >> 24 & 0xFF, secondary);
        IdeWrite8(ATA_LBA_MID, start_sector >> 32 & 0xFF, secondary);
        IdeWrite8(ATA_LBA_HI, start_sector >> 40 & 0xFF, secondary);
    }

    /* Low 8 bits of sector count */
    IdeWrite8(ATA_SEC_COUNT, sector_count & 0xFF, secondary);

    /* LBA bytes 0, 1, 2 */
    IdeWrite8(ATA_LBA_LO, start_sector >> 0 & 0xFF, secondary);
    IdeWrite8(ATA_LBA_MID, start_sector >> 8 & 0xFF, secondary);
    IdeWrite8(ATA_LBA_HI, start_sector >> 16 & 0xFF, secondary);
    AtaWaitBusy(secondary);

    /* Read/write sectors command */
    IdeWrite8(ATA_COMMAND,
        write ? ATA_DMA_WRITE_SECTORS : ATA_DMA_READ_SECTORS,
        secondary);

    /* Start DMA transfer (set bit 0) */
    PsResetEvent(IoComplete);
    IdeBmWrite8(BM_COMMAND, IdeBmRead8(BM_COMMAND, secondary) | 1, secondary);
    PsWaitForObject(IoComplete);
    transferred = bytes;

    if (!write)
        MemCopy(buffer, active_buffer + skipped_bytes, bytes);

    if (allocated)
        MmFreeDmaMemory(active_buffer, 16, bytes);

    PsReleaseMutex(IoLock);

    return !errors;
}

PzStatus DriverDispatch(
    PzDeviceObject *device,
    PzIoRequestPacket *irp)
{
    PzIoStackLocation *stack = irp->CurrentLocation;
    int mj_func = stack->MajorFunction;
    u8 *output = (u8 *)irp->SystemBuffer;
    MbrDisk *disk;
    u8 mbr_buffer[512];
    const char *filename = irp->Iocb->Filename->Buffer;

    switch (mj_func) {
    case IRP_MJ_CREATE: {
        disk = (MbrDisk *)PzHeapAllocate(sizeof(MbrDisk), 0);
        stack->Context = (uptr)disk;
        disk->BytesPerSector = 512;
        int length;
        filename = ParseUint(filename, &disk->Number, &length);

        if (length == 0 || disk->Number > 3) {
            PzHeapFree(disk);
            return STATUS_INVALID_PATH;
        }

        if (filename[0] != '\\')
            disk->SelectedPartition = -1;
        else {
            filename = ParseUint(filename + 1, &disk->SelectedPartition, &length);

            if (length == 0 || disk->SelectedPartition > 4) {
                PzHeapFree(disk);
                return STATUS_INVALID_PATH;
            }
        }

        u32 read;
        if (!AtaDmaTransfer(disk, &disk->Partitions, false,
            446, sizeof(MbrDisk::PartitionEntry) * 4, read)) {
            PzHeapFree(disk);
            return STATUS_TRANSFER_FAILED;
        }

        return STATUS_SUCCESS;
    }

    case IRP_MJ_READ: {
        u32 read; u64 offset;
        disk = (MbrDisk *)stack->Context;

        offset = disk->SelectedPartition != -1 ?
            disk->Partitions[disk->SelectedPartition].StartLba * disk->BytesPerSector :
            0;

        if (AtaDmaTransfer(
            disk, output,
            false, offset + stack->Parameters.Read.Offset,
            stack->Parameters.Read.Length, read)) {
            irp->UserStatus->Information = stack->Parameters.Read.Length;
            return STATUS_SUCCESS;
        }

        return STATUS_TRANSFER_FAILED;
    }

    case IRP_MJ_WRITE: {
        break;
    }

    case IRP_MJ_CLOSE: {
        PzHeapFree((void *)stack->Context);
        break;
    }
    }
    return STATUS_FAILED;
}