#pragma once

#include <main.hh>

struct DisasmView {
    uptr Start;
    u32 Size;
    std::vector<u8> Data;
};