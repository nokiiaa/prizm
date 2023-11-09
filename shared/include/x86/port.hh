#pragma once

#include <stdint.h>
#include <defs.hh>

PZ_KERNEL_EXPORT void HalPortOut8(u16 port, u8 value);
PZ_KERNEL_EXPORT void HalPortOut16(u16 port, u16 value);
PZ_KERNEL_EXPORT void HalPortOut32(u16 port, u32 value);
PZ_KERNEL_EXPORT u8   HalPortIn8(u16 port);
PZ_KERNEL_EXPORT u16  HalPortIn16(u16 port);
PZ_KERNEL_EXPORT u32  HalPortIn32(u16 port);
PZ_KERNEL_EXPORT void HalMsrRead(u32 msr, u32 *lo, u32 *hi);
PZ_KERNEL_EXPORT void HalMsrWrite(u32 msr, u32 lo, u32 hi);