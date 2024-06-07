#pragma once

#include <defs.hh>
#include <lib/list.hh>
#include <x86/port.hh>

#define PCI_CLASS_UNCLASSIFIED       0U
#define PCI_CLASS_MASS_STORAGE       1U
#define PCI_CLASS_NETWORK_CTRL       2U
#define PCI_CLASS_DISPLAY_CTRL       3U
#define PCI_CLASS_MULTIMEDIA         4U
#define PCI_CLASS_MEMORY_CTRL        5U
#define PCI_CLASS_BRIDGE_DEV         6U
#define PCI_CLASS_SCC                7U
#define PCI_CLASS_BSP                8U
#define PCI_CLASS_INPUT_CTRL         9U
#define PCI_CLASS_DOCKING_STN        10U
#define PCI_CLASS_PROCESSOR          11U
#define PCI_CLASS_SERIAL_CTRL        12U
#define PCI_CLASS_WIRELESS_CTRL      13U
#define PCI_CLASS_INTELLIGENT_CTRL   14U
#define PCI_CLASS_SATELLITE_COM_CTRL 15U
#define PCI_CLASS_ENCRYPTION_CTRL    16U
#define PCI_CLASS_SIGNAL_PROC_CTRL   17U
#define PCI_CLASS_PROC_ACCEL         18U
#define PCI_CLASS_NEI                19U
#define PCI_CLASS_COPROCESSOR        64U

#define PCI_SUB_MSC_IDE_CTRL 1U

#define PCI_COMMAND_OFFSET 4U
#define PCI_BAR0_OFFSET 16U

struct PciDevice
{
    inline PciDevice(u8 bus, u8 device, u8 slot, u16 vendor_id, u16 dev_id,
        u8 base_class, u8 subclass)
        : Bus(bus), Device(device), Slot(slot), VendorID(vendor_id),
        DeviceID(dev_id), BaseClass(base_class), Subclass(subclass) { }
    inline PciDevice() { }
    u8 Bus, Device, Slot;
    u16 VendorID, DeviceID;
    u8 BaseClass, Subclass;
};

/* Reads a doubleword at an 8-bit offset from a PCI device's configuration space. */
PZ_KERNEL_EXPORT u32 PciReadConfig(u8 bus, u8 slot, u8 func, u8 offset);
/* Reads a word at an 8-bit offset from a PCI device's configuration space. */
PZ_KERNEL_EXPORT u16 PciReadConfig16(u8 bus, u8 slot, u8 func, u8 offset);
/* Writes a doubleword at an 8-bit offset from a PCI device's configuration space. */
PZ_KERNEL_EXPORT void PciWriteConfig(u8 bus, u8 slot, u8 func, u8 offset, u32 value);
/* Writes a word at an 8-bit offset from a PCI device's configuration space. */
PZ_KERNEL_EXPORT void PciWriteConfig16(u8 bus, u8 slot, u8 func, u8 offset, u16 value);
/* Attempts to find a PCI device with the given vendor and device identifier,
   filling out the input PciDevice structure. */
PZ_KERNEL_EXPORT bool PciLocateDevice(u16 vendor, u16 device, PciDevice *dev);
/* Attempts to find a PCI device with the given class and subclass identifier,
   filling out the input PciDevice structure. */
PZ_KERNEL_EXPORT bool PciLocateDeviceByClass(u8 base, u8 subclass, PciDevice *dev);
/* Returns the linked list of all devices identified on the PCI bus. */
PZ_KERNEL_EXPORT LinkedList<PciDevice> *PciGetDeviceList();

inline u32 PciReadConfigDevice(const PciDevice *dev, u8 offset)
{
    return PciReadConfig(dev->Bus, dev->Device, dev->Slot, offset);
}

inline u32 PciReadConfigDevice16(const PciDevice *dev, u8 offset)
{
    return PciReadConfig16(dev->Bus, dev->Device, dev->Slot, offset);
}

inline void PciWriteConfigDevice(const PciDevice *dev, u8 offset, u32 value)
{
    PciWriteConfig(dev->Bus, dev->Device, dev->Slot, offset, value);
}

inline void PciWriteConfigDevice16(const PciDevice *dev, u8 offset, u16 value)
{
    PciWriteConfig16(dev->Bus, dev->Device, dev->Slot, offset, value);
}

inline u32 PciReadBar(u8 bus, u8 slot, u8 func, int bar)
{
    return PciReadConfig(bus, slot, func, PCI_BAR0_OFFSET + bar * 4);
}

inline u32 PciReadBarDevice(PciDevice *dev, int bar)
{
    return PciReadBar(dev->Bus, dev->Device, dev->Slot, bar);
}

void PciScanAll();