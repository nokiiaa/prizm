#include <x86/cmos.hh>
#include <x86/port.hh>

u8 CmosReadRegister(int reg)
{
    HalPortOut8(CMOS_ADDRESS, reg);
    return HalPortIn8(CMOS_DATA);
}

bool CmosRtcUpdateInProgress()
{
    return CmosReadRegister(0xA) & 1 << 7;
}

void CmosEnableRtcIrq(int rate)
{
    HalPortOut8(CMOS_ADDRESS, 0x8B);
    u8 prev = HalPortIn8(CMOS_DATA);
    HalPortOut8(CMOS_ADDRESS, 0x8B);
    HalPortOut8(CMOS_DATA, prev | 0x40);

    rate &= 0xF;
    HalPortOut8(CMOS_ADDRESS, 0x8A);
    prev = HalPortIn8(CMOS_DATA);
    HalPortOut8(CMOS_ADDRESS, 0x8A);
    HalPortOut8(CMOS_DATA, prev & 0xF0 | rate);
}

void CmosWaitForChange(int reg)
{
    while (CmosRtcUpdateInProgress());

    int secs = CmosReadRegister(reg);
    do while (CmosRtcUpdateInProgress()); while (secs == CmosReadRegister(reg));
}