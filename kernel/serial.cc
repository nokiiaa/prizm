#include <serial.hh>
#include <x86/port.hh>
#include <stdarg.h>
#include <lib/strformat.hh>
#include <debug.hh>

const int Ports[] = { 0x3F8, 0x2F8, 0x3E8, 0x2E8 }; // COM1-4
volatile static int ComPort;

#define SERIAL_BASE_BAUD 115200

#define REG_DATA         0
#define REG_INT_ENABLE   1
#define REG_DIV_LO       0
#define REG_DIV_HI       1
#define REG_INT_IDENT    2
#define REG_LINE_CTRL    3
#define REG_MODEM_CTRL   4
#define REG_LINE_STATUS  5
#define REG_MODEM_STATUS 6
#define REG_SCRATCH      7

#define LS_DATA_READY     (1 << 0)
#define LS_OVERRUN_ERR    (1 << 1)
#define LS_PARITY_ERR     (1 << 2)
#define LS_FRAMING_ERR    (1 << 3)
#define LS_BREAK_IND      (1 << 4)
#define LS_TRANSBUF_EMPTY (1 << 5)
#define LS_TRANS_EMPTY    (1 << 6)
#define LS_IMPENDING_ERR  (1 << 7)

#define LINE_CTRL_DLAB    (1 << 7)

void SerialSetRate(int baud)
{
    u16 div = SERIAL_BASE_BAUD / baud;
    HalPortOut8(ComPort + REG_DIV_LO, div & 0xFF);
    HalPortOut8(ComPort + REG_DIV_HI, div >> 8);
}

void SerialInitializePort(int n, int baud)
{
    ComPort = Ports[n - 1];

    /* Disable interrupts */
    HalPortOut8(ComPort + REG_INT_ENABLE, 0);
    HalPortOut8(ComPort + REG_LINE_CTRL, LINE_CTRL_DLAB);
    SerialSetRate(baud);

    /* 0b00000011 - data length is 8 bits */
    HalPortOut8(ComPort + REG_LINE_CTRL, 0b00000011);

    /* 0b11000111 - enable FIFO, clear the transmission
       and receiver FIFO buffers, 14 bytes as queue size */
    HalPortOut8(ComPort + REG_INT_IDENT, 0b11000111);

    /* 0b00001011 - enable interrupt output,
       ready to transmit, data terminal ready bits */
    HalPortOut8(ComPort + REG_MODEM_CTRL, 0b00001011);
}

int SerialReceived()
{
    return HalPortIn8(ComPort + REG_LINE_STATUS) & LS_DATA_READY;
}

int SerialIsTransmitEmpty()
{
    return HalPortIn8(ComPort + REG_LINE_STATUS) & LS_TRANSBUF_EMPTY;
}

char SerialReadChar()
{
    while (!SerialReceived());
    return HalPortIn8(ComPort);
}

void SerialPrintChar(char c)
{
    while (!SerialIsTransmitEmpty());
    HalPortOut8(ComPort, c);
}

void SerialPrintStr(const char *str, ...)
{
    va_list params;
    va_start(params, str);
    PrintfWithCallbackV(SerialPrintChar, str, params);
    va_end(params);
}