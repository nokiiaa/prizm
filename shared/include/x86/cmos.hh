#pragma once

#include <defs.hh>

#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

u8 CmosReadRegister(int reg);
bool CmosRtcUpdateInProgress();
void CmosEnableRtcIrq(int rate);
void CmosWaitForChange(int reg);