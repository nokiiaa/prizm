#pragma once

#include <defs.hh>

PZ_KERNEL_EXPORT void SerialSetRate(int baud);
PZ_KERNEL_EXPORT void SerialInitializePort(int n, int baud);
PZ_KERNEL_EXPORT char SerialReadChar();
PZ_KERNEL_EXPORT void SerialPrintChar(char c);
PZ_KERNEL_EXPORT void SerialPrintStr(const char *str, ...);