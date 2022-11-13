#pragma once

#include <syscall/handle.hh>
#include <window/wnd.hh>
#include <window/console.hh>

struct UmCreateWindowParams {
    PzHandle *Handle;
    PzHandle Parent;
    const PzString *Title;
    int X, Y, Z;
    u32 Width, Height;
    int Style;
};

struct UmGetWindowParameterParams {
    PzHandle Window;
    int Index;
    uptr *Value;
};

struct UmSetWindowParameterParams {
    PzHandle Window;
    int Index;
    uptr Value;
};

struct UmGetWindowTitleParams {
    PzHandle Window;
    char *OutTitle;
    u32 *MaxSize;
};

struct UmSetWindowTitleParams {
    PzHandle Window;
    const PzString *OutTitle;
};

struct UmGetWindowBufferParams {
    PzHandle Window;
    int Index;
    GfxHandle *Handle;
};

struct UmSetWindowBufferParams {
    PzHandle Window;
    int Index;
    GfxHandle Handle;
};

struct UmSwapBuffersParams {
    PzHandle Window;
};

struct UmEnumerateChildWindowsParams {
    PzHandle Window;
    u32 *MaxCount;
    PzHandle *Handles;
    bool Recursive;
};

DECL_SYSCALL(UmCreateWindow);
DECL_SYSCALL(UmGetWindowTitle);
DECL_SYSCALL(UmSetWindowTitle);
DECL_SYSCALL(UmGetWindowBuffer);
DECL_SYSCALL(UmSetWindowBuffer);
DECL_SYSCALL(UmSwapBuffers);
DECL_SYSCALL(UmGetWindowParameter);
DECL_SYSCALL(UmSetWindowParameter);
DECL_SYSCALL(UmRegisterWindowServer);
DECL_SYSCALL(UmUnregisterWindowServer);
DECL_SYSCALL(UmEnumerateChildWindows);
DECL_SYSCALL(UmAllocateConsole);
DECL_SYSCALL(UmRegisterConsoleHost);
DECL_SYSCALL(UmUnregisterConsoleHost);