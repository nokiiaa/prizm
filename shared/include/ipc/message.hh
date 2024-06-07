#pragma once

#include <defs.hh>

struct PzMessage {
    u32 Type;
    uptr Params[4];
};

PzStatus MsgPostThreadMessage(PzHandle thread, u32 type, uptr param1, uptr param2, uptr param3, uptr param4);
PzStatus MsgPeekThreadMessage(PzMessage *out_message);
PzStatus MsgReceiveThreadMessage(PzMessage *out_message);