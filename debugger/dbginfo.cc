#include <dbginfo.hh>
#include <fstream>
#include <sstream>

std::vector<ModuleDebugInfo> DebugInfo::Modules;
bool DebugInfo::Initialized; 

bool DebugInfo::Parse(
    ModuleDebugInfo &info,
    const PzModuleObject &module,
    const std::string &name,
    const std::string &lines_path,
    const std::string &info_path)
{
    info.Start = module.BaseAddress;
    info.End = info.Start + module.ImageSize;
    info.Name = name;
    info.Lines.clear();
    info.Symbols.clear();

    /* Scan dwarfdump output for address and line information */
    std::ifstream lines(lines_path);
    std::string fline, filename;
    std::size_t pos;
    std::string _;

    while (std::getline(lines, fline)) {
        uptr addr;
        int line, col;
        std::stringstream stm(fline);

        if (fline.substr(0, 2) == "0x") {
            stm >> std::hex >> addr >> _ >> std::dec >> line >> _ >> col;
            if ((pos = fline.find("uri: ")) != std::string::npos)
                filename = fline.substr(pos + 5);
            info.Lines[addr] = ModuleSourceLocation { line, col, filename };
        }
    }

    /* Scan dwarfdump output for function names and get their names and start and end addresses */
    std::ifstream symbols(info_path);
    std::string sym_name, field_name;
    uptr sym_start, sym_size;

    while (std::getline(symbols, fline)) {
        bool skip = false;

        if (fline.find("DW_TAG_subprogram") != std::string::npos) {
            int fields = 0;

            while (fields < 3 && std::getline(symbols, fline)) {
                if (fline[0] == '<') {
                    skip = true;
                    break;
                }

                std::stringstream stm(fline);
                if (fline.find("DW_AT_name") != std::string::npos) {
                    stm >> field_name >> sym_name;
                    //std::printf("DW_AT_NAME %s\n", sym_name.c_str());
                    fields++;
                }
                else if (fline.find("DW_AT_low_pc") != std::string::npos) {
                    stm >> field_name >> std::hex >> sym_start;
                    //std::printf("DW_AT_low_pc %08x\n", sym_start);
                    fields++;
                }
                else if (fline.find("DW_AT_high_pc") != std::string::npos) {
                    stm >> field_name >> _ >> sym_size;
                    //std::printf("DW_AT_high_pc %08x\n", sym_size);
                    fields++;
                }
            }

            if (!skip) {
                //std::printf("symbol %s!%s (0x%08x-0x%08x)\n",
                //    name.c_str(), sym_name.c_str(), sym_start, sym_start + sym_size);
                info.Symbols[sym_name] = ModuleSymbolBounds
                { sym_start, sym_start + sym_size };
            }
        }
    }
    return true;
}

std::map<std::string, std::pair<std::string, std::string>> DebugInfoLocations = {
    {"pzkernel.exe", {"../../bin/pzkernel_lines.dbg", "../../bin/pzkernel_info.dbg"}}
};

bool DebugInfo::AddModule(uptr addr)
{
    PzModuleObject module;
    std::string module_name;
    Link::ReadKernelObject<PzModuleObject>(addr, &module);
    Link::ReadKernelString(module.ModuleName, module_name);
    auto it = DebugInfoLocations.find(module_name);

    if (it == DebugInfoLocations.end())
        return false;

    ModuleDebugInfo info;
    Parse(info, module, module_name, it->second.first, it->second.second);
    Modules.push_back(info);
    return true;
}

bool DebugInfo::GetModule(uptr addr, ModuleDebugInfo **info)
{
    for (ModuleDebugInfo &module : Modules) {
        if (module.Start <= addr && addr < module.End) {
            *info = &module;
            return true;
        }
    }

    return false;
}

bool DebugInfo::GetSourceLocation(
    const ModuleDebugInfo& module, uptr addr,
    int& line, int& col, std::string& filename)
{
    auto it = module.Lines.find(addr);

    if (it == module.Lines.end())
        return false;

    line     = it->second.Line;
    col      = it->second.Column;
    filename = it->second.Filename;
    return true;
}

bool DebugInfo::GetFunction(
    const ModuleDebugInfo &module,
    uptr addr, uptr &func_start, uptr &func_end,
    std::string &func_name)
{
    for (auto &p : module.Symbols) {
        if (p.second.Start <= addr && addr < p.second.End) {
            func_start = p.second.Start;
            func_end = p.second.End;
            func_name = p.first;
            return true;
        }
    }
    return false;
}

bool DebugInfo::Init()
{
    if (!Link::RefreshObjectList())
        return false;

    for (uptr addr : Link::Modules)
        AddModule(addr);

    Initialized = true;
    return true;
}