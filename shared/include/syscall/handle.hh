#pragma once

#include <core.hh>

#define DECL_SYSCALL(x) PzStatus x(CpuInterruptState *state, void *params)

extern "C" void SyscallHandler(CpuInterruptState *state);
void ScInitializeTable();