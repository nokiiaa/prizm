#pragma once

#include <stdarg.h>
#include <defs.hh>
#include <lib/string.hh>

void PrintfWithCallbackV(void (*print_func)(char), const char *format, va_list params);