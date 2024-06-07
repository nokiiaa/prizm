#pragma once

#include <defs.hh>

#define E820_REGION_USABLE   1
#define E820_REGION_RESERVED 2
#define E820_REGION_ACPI_RM  3
#define E820_REGION_ACPI_NVS 4
#define E820_REGION_BAD      5

struct E820MemRegion
{
    u32 BaseLo, BaseHi;
    u32 LengthLo, LengthHi;
    u32 RegionType;
};