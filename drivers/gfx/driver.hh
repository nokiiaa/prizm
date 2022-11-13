#pragma once

#include <defs.hh>
#include <io/manager.hh>
#include <mm/virtual.hh>
#include <lib/util.hh>
#include <sched/scheduler.hh>
#include <gfx/link.hh>

#pragma pack(push, 1)
struct VbeVideoModeData
{
	u16 Attributes;
	u8 WindowA;
	u8 WindowB;
	u16 Granularity;
	u16 WindowSize;
	u16 SegmentA;
	u16 SegmentB;
	u32 WinFuncPtr;
	u16 Pitch;
	u16 Width;
	u16 Height;
	u8 WChar;
	u8 YChar;
	u8 Planes;
	u8 BitsPerPixel;
	u8 Banks;
	u8 MemoryModel;
	u8 BankSize;
	u8 ImagePages;
	u8 Reserved;

	u8 RedMask;
	u8 RedPosition;
	u8 GreenMask;
	u8 GreenPosition;
	u8 BlueMask;
	u8 BluePosition;
	u8 ReservedMask;
	u8 ReservedPosition;
	u8 DirectColorAttributes;

	u32 Framebuffer;
	u32 OffScreenMemOffset;
	u16 OffScreenMemSize;
};
#pragma pack(pop)

#define CREATE_TYPE_FRAMEBUFFER 1

extern VbeVideoModeData *VbeData;
extern void *VbeFramebuffer;
extern u32 VbeFramebufferSize;