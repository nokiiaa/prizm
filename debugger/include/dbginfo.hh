#pragma once

#include <main.hh>

struct ModuleSourceLocation {
    int Line, Column;
    std::string Filename;
};

struct ModuleSymbolBounds {
    uptr Start, End;
};

struct ModuleDebugInfo {
    std::string Name;
    uptr Start, End;
    std::map<uptr /* address */, ModuleSourceLocation> Lines;
    std::map<std::string, ModuleSymbolBounds> Symbols;
};

namespace DebugInfo {
    extern bool Initialized;
    extern std::vector<ModuleDebugInfo> Modules;
    bool Parse(
        ModuleDebugInfo &info,
        const PzModuleObject &module,
        const std::string &name,
        const std::string &lines_path,
        const std::string &info_path);
    bool GetModule(uptr addr, ModuleDebugInfo **info);
    bool GetSourceLocation(
        const ModuleDebugInfo &module, uptr addr,
        int &line, int &col, std::string &filename);
    bool GetFunction(
        const ModuleDebugInfo &module,
        uptr addr, uptr &func_start, uptr &func_end,
        std::string &func_name);
    bool AddModule(uptr addr);
    bool Init();
};