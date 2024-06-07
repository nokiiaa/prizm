#include <syscall/message.hh>
#include <mm/virtual.hh>

DECL_SYSCALL(UmPostThreadMessage)
{
    auto *prm = (UmPostThreadMessageParams *)params;
    return MsgPostThreadMessage(prm->Thread, prm->Type, prm->Param1, prm->Param2, prm->Param3, prm->Param4);
}

DECL_SYSCALL(UmPeekThreadMessage)
{
    auto *prm = (UmPeekThreadMessageParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm->OutMessage, sizeof(PzMessage), true))
        return STATUS_INVALID_ARGUMENT;

    return MsgPeekThreadMessage(prm->OutMessage);
}

DECL_SYSCALL(UmReceiveThreadMessage)
{
    auto *prm = (UmPeekThreadMessageParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm->OutMessage, sizeof(PzMessage), true))
        return STATUS_INVALID_ARGUMENT;

    return MsgReceiveThreadMessage(prm->OutMessage);
}