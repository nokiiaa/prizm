#include <x86/port.hh>

#ifdef __GNUC__
u8 HalPortIn8(u16 port_number)
{
    u8 ret;
    asm volatile ("inb %1, %0"
        : "=a"(ret)
        : "Nd"(port_number));
    return ret;
}

u16 HalPortIn16(u16 port_number)
{
    u16 ret;
    asm volatile ("inw %1, %0"
        : "=a"(ret)
        : "Nd"(port_number));
    return ret;
}

u32 HalPortIn32(u16 port_number)
{
    u32 ret;
    asm volatile ("inl %1, %0"
        : "=a"(ret)
        : "Nd"(port_number));
    return ret;
}

void HalPortOut8(u16 port_number, u8 val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port_number));
}

void HalPortOut16(u16 port_number, u16 val)
{
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port_number));
}

void HalPortOut32(u16 port_number, u32 val)
{
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port_number));
}

void HalMsrRead(u32 msr, u32 *lo, u32 *hi)
{
    asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

void HalMsrWrite(u32 msr, u32 lo, u32 hi)
{
    asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}
#else
    #error TODO: msvc inline assembly for this file
#endif