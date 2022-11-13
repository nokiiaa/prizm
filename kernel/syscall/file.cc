#include <syscall/file.hh>
#include <debug.hh>

PzStatus UmCreateFile(CpuInterruptState *state, void *params)
{
    auto *prm = (UmCreateFileParams*)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmCreateFileParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Handle, sizeof(PzHandle), true) ||
        !PzValidateString(true, prm->Filename, false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Iosb, sizeof(PzIoStatusBlock), false)) {
        return STATUS_INVALID_ARGUMENT;
    }

    return PzCreateFile(false, prm->Handle, prm->Filename, prm->Iosb, prm->Access, prm->Disposition);
}

PzStatus UmReadFile(CpuInterruptState *state, void *params)
{
    auto *prm = (UmReadFileParams*)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmReadFileParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Buffer, prm->Bytes, true) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Iosb, sizeof(PzIoStatusBlock), false) ||
        prm->Offset && !MmVirtualProbeMemory(true, (uptr)prm->Offset, sizeof(u64), false))
        return STATUS_INVALID_ARGUMENT;

    return PzReadFile(prm->Handle, prm->Buffer, prm->Iosb, prm->Bytes, prm->Offset);
}

PzStatus UmWriteFile(CpuInterruptState *state, void *params)
{
    auto *prm = (UmWriteFileParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmWriteFileParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Buffer, prm->Bytes, false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Iosb, sizeof(PzIoStatusBlock), false) ||
        prm->Offset && !MmVirtualProbeMemory(true, (uptr)prm->Offset, sizeof(u64), false))
        return STATUS_INVALID_ARGUMENT;

    return PzWriteFile(prm->Handle, prm->Buffer, prm->Iosb, prm->Bytes, prm->Offset);
}

PzStatus UmQueryInformationFile(CpuInterruptState *state, void *params)
{
    auto *prm = (UmQueryInformationFileParams*)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmQueryInformationFileParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Iosb, sizeof(PzIoControlBlock), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->OutBuffer, prm->BufferSize, true))
        return STATUS_INVALID_ARGUMENT;

    return PzQueryInformationFile(prm->Handle, prm->Iosb, prm->Type, prm->OutBuffer, prm->BufferSize);
}

PzStatus UmSetInformationFile(CpuInterruptState *state, void *params)
{
    auto *prm = (UmSetInformationFileParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmQueryInformationFileParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Iosb, sizeof(PzIoControlBlock), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Buffer, prm->BufferSize, true))
        return STATUS_INVALID_ARGUMENT;

    return PzSetInformationFile(prm->Handle, prm->Iosb, prm->Type, prm->Buffer, prm->BufferSize);
}

PzStatus UmDeviceIoControl(CpuInterruptState *state, void *params)
{
    auto *prm = (UmDeviceIoControlParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmQueryInformationFileParams), false) ||
        !MmVirtualProbeMemory(true, (uptr)prm->Iosb, sizeof(PzIoControlBlock), false) ||
        prm->InBuffer  && !MmVirtualProbeMemory(true, (uptr)prm->InBuffer, prm->InBufSize, false) ||
        prm->OutBuffer && !MmVirtualProbeMemory(true, (uptr)prm->OutBuffer, prm->OutBufSize, true))
        return STATUS_INVALID_ARGUMENT;

    return PzDeviceIoControl(
        prm->DeviceHandle,
        prm->Iosb,
        prm->ControlCode,
        prm->InBuffer,
        prm->InBufSize,
        prm->OutBuffer,
        prm->OutBufSize);
}

PzStatus UmCloseHandle(CpuInterruptState *state, void *params)
{
    auto *prm = (UmCloseHandleParams *)params;
    if (!MmVirtualProbeMemory(true, (uptr)prm, sizeof(UmCloseHandleParams), false))
        return STATUS_INVALID_ARGUMENT;

    return PzCloseHandle(prm->Handle);
}