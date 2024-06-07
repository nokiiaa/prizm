#include <syscall/handle.hh>
#include <ipc/message.hh>

struct UmPostThreadMessageParams {
    PzHandle Thread;
    u32 Type;
    uptr Param1, Param2, Param3, Param4;
};

struct UmPeekThreadMessageParams {
    PzMessage *OutMessage;
};

DECL_SYSCALL(UmPostThreadMessage);
DECL_SYSCALL(UmPeekThreadMessage);
DECL_SYSCALL(UmReceiveThreadMessage);