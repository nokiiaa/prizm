#include <syscall/window.hh>

DECL_SYSCALL(UmCreateWindow)
{
    auto *prm = (UmCreateWindowParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm->Handle, sizeof(PzHandle), true) ||
        !PzValidateString(true, prm->Title, false))
        return STATUS_INVALID_ARGUMENT;

    return WndCreateWindow(
        prm->Handle, prm->Parent, prm->Title, prm->X, prm->Y, prm->Z,
        prm->Width, prm->Height, prm->Style);
}

DECL_SYSCALL(UmGetWindowTitle)
{
    auto *prm = (UmGetWindowTitleParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm->MaxSize, sizeof(uptr), !prm->OutTitle) ||
        prm->OutTitle && !MmVirtualProbeMemory(true, (uptr)prm->OutTitle, *prm->MaxSize, true))
        return STATUS_INVALID_ARGUMENT;

    return WndGetWindowTitle(prm->Window, prm->OutTitle, prm->MaxSize);
}

DECL_SYSCALL(UmSetWindowTitle)
{
    auto *prm = (UmSetWindowTitleParams *)params;

    if (!PzValidateString(true, prm->OutTitle, false))
        return STATUS_INVALID_ARGUMENT;

    return WndSetWindowTitle(prm->Window, prm->OutTitle);
}

DECL_SYSCALL(UmGetWindowBuffer)
{
    auto *prm = (UmGetWindowBufferParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm->Handle, sizeof(uptr), false))
        return STATUS_INVALID_ARGUMENT;

    return WndGetWindowBuffer(prm->Window, prm->Index, prm->Handle);
}

DECL_SYSCALL(UmSetWindowBuffer)
{
    auto *prm = (UmSetWindowBufferParams *)params;
    return WndSetWindowBuffer(prm->Window, prm->Index, prm->Handle);
}

DECL_SYSCALL(UmSwapBuffers)
{
    auto *prm = (UmSwapBuffersParams *)params;
    return WndSwapBuffers(prm->Window);
}

DECL_SYSCALL(UmGetWindowParameter)
{
    auto *prm = (UmGetWindowParameterParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm->Value, sizeof(uptr), true))
        return STATUS_INVALID_ARGUMENT;

    return WndGetWindowParameter(prm->Window, prm->Index, prm->Value);
}

DECL_SYSCALL(UmSetWindowParameter)
{
    auto *prm = (UmSetWindowParameterParams *)params;
    return WndSetWindowParameter(prm->Window, prm->Index, prm->Value);
}

DECL_SYSCALL(UmRegisterWindowServer)
{
    return WndRegisterWindowServer();
}

DECL_SYSCALL(UmUnregisterWindowServer)
{
    return WndUnregisterWindowServer();
}

DECL_SYSCALL(UmEnumerateChildWindows)
{
    auto *prm = (UmEnumerateChildWindowsParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm->MaxCount, sizeof(uptr), !prm->Handles) ||
        prm->Handles && !MmVirtualProbeMemory(true, (uptr)prm->Handles, *prm->MaxCount, true))
        return STATUS_INVALID_ARGUMENT;

    return WndEnumerateChildWindows(prm->Window, prm->MaxCount, prm->Handles, prm->Recursive);
}

DECL_SYSCALL(UmAllocateConsole)
{
    return ConAllocateConsole();
}

DECL_SYSCALL(UmRegisterConsoleHost)
{
    return ConRegisterConsoleHost();
}

DECL_SYSCALL(UmUnregisterConsoleHost)
{
    return ConUnregisterConsoleHost();
}