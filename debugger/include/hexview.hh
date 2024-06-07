#pragma once

struct HexView {
    uptr Start;
    u32 Size;
    int ValuesPerLine;
    std::vector<u8> Data;
};