#include <pci/pci.hh>
#include <debug.hh>
#include <core.hh>

LinkedList<PciDevice> PciConnectedDevices;
PzSpinlock PciLock;
constexpr u16 PciAddr = 0xCF8;
constexpr u16 PciData = 0xCFC;

u32 PciReadConfig(u8 bus, u8 slot, u8 func, u8 offset)
{
    PzAcquireSpinlock(&PciLock);

    u32 address =
        u32(bus) << 16 |
        u32(slot) << 11 |
        u32(func) << 8 |
        offset & 0xFC |
        1u << 31;
    HalPortOut32(PciAddr, address);
    u32 data = HalPortIn32(PciData);

    PzReleaseSpinlock(&PciLock);
    return data;
}

u16 PciReadConfig16(u8 bus, u8 slot, u8 func, u8 offset)
{
    return u16(PciReadConfig(bus, slot, func, offset) >> (offset & 2) * 8);
}

void PciWriteConfig(u8 bus, u8 slot, u8 func, u8 offset, u32 data)
{
    PzAcquireSpinlock(&PciLock);

    u32 address =
        u32(bus) << 16 |
        u32(slot) << 11 |
        u32(func) << 8 |
        offset & 0xFC |
        1u << 31;

    HalPortOut32(PciAddr, address);
    HalPortOut32(PciData, data);

    PzReleaseSpinlock(&PciLock);
}

void PciWriteConfig16(u8 bus, u8 slot, u8 func, u8 offset, u16 data)
{
    u32 value = PciReadConfig(bus, slot, func, offset);

    if (offset & 2)
        value = value & 0x0000FFFF | data << 16;
    else
        value = value & 0xFFFF0000 | data;

    PciWriteConfig(bus, slot, func, offset, value);
}

u8 ReadHeaderType(u8 bus, u8 slot, u8 func)
{
    return u8(PciReadConfig16(bus, slot, func, 0xC + 2));
}

void ReadClass(u8 bus, u8 slot, u8 func,
    u8 &base, u8 &subclass)
{
    u16 codes = PciReadConfig16(bus, slot, func, 0x8 + 2);
    base = codes >> 8;
    subclass = codes & 0xFF;
}

u16 ReadVendor(u8 bus, u8 slot, u8 func)
{
    return PciReadConfig16(bus, slot, func, 0x0);
}

void ReadBridgeBusNumbers(
    u8 bus, u8 slot, u8 func,
    u8 &primary, u8 &secondary)
{
    u16 n = PciReadConfig16(bus, slot, func, 0x18);
    primary = n & 0xFF;
    secondary = n >> 8;
}

void ScanBus(u8 bus);

void ScanFunction(u8 bus, u8 slot, u8 func)
{
    u8 base, subclass;
    ReadClass(bus, slot, func, base, subclass);

    /* Is a bridge to some other bus */
    if (base == 6 && subclass == 4) {
        u8 prim, sec;
        ReadBridgeBusNumbers(bus, slot, func, prim, sec);
        ScanBus(sec);
    }

    u32 dev = PciReadConfig(bus, slot, func, 0);
    u16 vendor_id = dev & 0xFFFF;
    u16 device_id = dev >> 16;

    DbgPrintStr(
        "(bus%i, dev%i, slot%i) PCI device detected: vendor 0x%04x device 0x%04x class %02x subclass %02x\r\n",
        bus, slot, func, vendor_id, device_id, base, subclass);

    PciConnectedDevices.Add(
        PciDevice(bus, slot, func, vendor_id, device_id, base, subclass));
}

void ScanDevice(u8 bus, u8 slot)
{
    if (ReadVendor(bus, slot, 0) == 0xFFFF)
        return;

    ScanFunction(bus, slot, 0);

    /* Check if device has multiple functions */
    u8 hdr_type = ReadHeaderType(bus, slot, 0);

    if (hdr_type >> 7)
        for (int func = 1; func < 8; func++)
            if (ReadVendor(bus, slot, func) != 0xFFFF)
                ScanFunction(bus, slot, func);
}

void ScanBus(u8 bus)
{
    for (int i = 0; i < 32; i++)
        ScanDevice(bus, i);
}

bool PciLocateDevice(u16 vendor, u16 device, PciDevice *dev)
{
    LLNode<PciDevice> *node = PciConnectedDevices.First;

    do {
        if (node->Value.VendorID == vendor && node->Value.DeviceID == device) {
            *dev = node->Value;
            return true;
        }
    } while (node = node->Next);

    return false;
}

bool PciLocateDeviceByClass(u8 base, u8 subclass, PciDevice *dev)
{
    ENUM_LIST(node, PciConnectedDevices) {
        if (node->Value.BaseClass == base && node->Value.Subclass == subclass) {
            *dev = node->Value;
            return true;
        }
    }

    return false;
}

void PciScanAll()
{
    /* Check if this is a single PCI host controller */
    if (!(ReadHeaderType(0, 0, 0) & 0x80))
        ScanBus(0);
    else {
        for (int i = 0; i < 8; i++) {
            /* Check if bus exists first */
            if (ReadVendor(0, 0, i) != 0xFFFF)
                break;

            ScanBus(i);
        }
    }
}

LinkedList<PciDevice> *PciGetDeviceList()
{
    return &PciConnectedDevices;
}