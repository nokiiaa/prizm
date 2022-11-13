#include <main.hh>
#include <link.hh>
#include <view.hh>
#include <objprint.hh>
#include <hexview.hh>
#include <dbginfo.hh>
#include <disasview.hh>
#include <algorithm>
#include <cstring>
#include <string>
#include <stdio.h>
#include <string>
#include <sstream>
#include <memory>
#include <SDL.h>
#include <glad/glad.h>
#include <stdint.h>
#include <inttypes.h>
#include <expr.hh>
#include <Zydis/Zydis.h>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

static ImVec4 ClearColor                 = ImVec4(204 / 255.f, 153 / 255.f, 255 / 255.f, 1.00f);
static ImVec4 FieldColor                 = ImVec4(0, 1, 1, 1);
static ImVec4 HexAddressColor            = ImVec4(0, 1, 0, 1);
static ImVec4 HexValueColor              = ImVec4(1, 1, 1, 1);
static ImVec4 AsciiValueUnprintableColor = ImVec4(.5, .5, .5, 1);
static ImVec4 AsciiValuePrintableColor   = ImVec4(1, 1, 0, 1);
static ImVec4 LogTextColor               = ImVec4(1, 1, 1, 1);
static ImFont *MainFont;
std::shared_ptr<ObjView> LastProcessTable;
std::shared_ptr<ObjView> RootDirectoryTable;

#define RENDER_VALUE(color, name, ...) \
    ImGui::TextColored(color, "%s", name); ImGui::SameLine(); ImGui::Text(__VA_ARGS__)

struct Breakpoint {
    u8 OriginalByte;
};

std::map<uptr, Breakpoint> Breakpoints;
std::map<uptr, std::shared_ptr<ObjView>> BackcheckMap;
std::map<std::string, i64> EvalContext;
float Time, Id;
/*static ZydisDecoder DisDecoder;
static ZydisFormatter DisFormatter;*/

bool ResolveAddress(uptr addr, ModuleDebugInfo **info,
    uptr& fstart, uptr& fend, std::string& name)
{
    bool resolved = false;

    if (DebugInfo::GetModule(addr, info))
        if (DebugInfo::GetFunction(**info, addr, fstart, fend, name))
            return true;

    return false;
}

void RenderStackTrace(const char *name, const std::vector<uptr>& trace)
{
    if (ImGui::TreeNode(name)) {
        for (uptr addr : trace) {

            uptr fstart, fend; std::string name;
            int line, col; std::string fname;
            ModuleDebugInfo *module;

            if (ResolveAddress(addr, &module, fstart, fend, name)) {
                if (DebugInfo::GetSourceLocation(*module, addr, line, col, fname)) {
                    /* Print address, location in module and source */
                    fname = fname.substr(fname.find("osdev\\") + 6);
                    fname.pop_back();

                    ImGui::TextColored(ImVec4(.7, 1, 0, 1),
                        "0x%08x (%s!%s+0x%x, %i:%i, %s)",
                        addr, module->Name.c_str(), name.c_str(), addr - fstart,
                        line, col, fname.c_str());
                }
                else {
                    /* Print address, location in module only */
                    ImGui::TextColored(ImVec4(.7, 1, 0, 1), "0x%08x (%s!%s+0x%x)",
                        addr, module->Name.c_str(), name.c_str(), addr - fstart);
                }
            }
            else
                /* Print address only */
                ImGui::TextColored(ImVec4(.7, 1, 0, 1), "0x%08x", addr);
        }

        ImGui::TreePop();
    }
}

void RenderBreakpointButton(uptr addr)
{
    auto bp = Breakpoints.find(uptr(addr));

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 40.f);
    ImGui::PushStyleColor(ImGuiCol_Button,
        Link::Breakpoint && addr == Link::BreakInfo.CpuState.Eip ?
        ImVec4(1, 1, 0, 1) : /* Yellow (breakpoint active) */
        bp != Breakpoints.end() ?
        ImVec4(1, 0, 0, 1) : /* Red (breakpoint set) */
        ImVec4(.7, .7, .7, 1) /* Grey (breakpoint clear) */);
    std::string button_name = "  ##bp" + std::to_string(addr);

    if (ImGui::Button(button_name.c_str())) {
        if (bp == Breakpoints.end()) {
            /* Replace byte at address with 0xCC and save original byte */
            u8 byte;
            Link::ReadVirtualMem(addr, &byte, 1);
            Breakpoints[addr] = { byte };
            Link::WriteVirtualMem(addr, &(byte = 0xCC), 1);
        }
        else {
            /* Remove breakpoint 0xCC byte */
            Link::WriteVirtualMem(addr, &bp->second.OriginalByte, 1);
            Breakpoints.erase(bp);
        }
    }

    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    ImGui::SameLine();
}

void RenderDisasmView(const DisasmView &view)
{
    /*ZyanUSize offset = 0;
    u32 addr = view.Start;
    ZydisDecodedInstruction instruction;
    ZyanStatus status;

    while (ZYAN_SUCCESS(status = ZydisDecoderDecodeBuffer(&DisDecoder,
        view.Data.data() + offset, view.Size - offset,
        &instruction))) {
        // Print current instruction pointer.
        uptr fstart, fend; std::string name;
        ModuleDebugInfo *info;
        RenderBreakpointButton((uptr)addr);

        ImGui::TextColored(HexAddressColor, "%08x ", addr);
        ImGui::SameLine(0, 0);

        bool resolved = false;
        if (ResolveAddress(addr, &info, fstart, fend, name)) {
            if (fstart == addr) {
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s!%s:",
                    info->Name.c_str(), name.c_str());
                RenderBreakpointButton((uptr)addr);
                ImGui::TextColored(HexAddressColor, "%08x ", addr);
                ImGui::SameLine(0, 0);
            }
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "+%04x: ", addr - fstart);
        }
        else
            ImGui::Text("       ", addr);

        char buffer[256];
        ZydisFormatterFormatInstruction(&DisFormatter, &instruction, buffer, sizeof(buffer),
            addr);
        bool ins_name = true;
        char *token_end = buffer;

#define ALPHANUMERIC(x) \
                (std::tolower((x)) >= 'a' && std::tolower((x)) <= 'z' || \
                (x) >= '0' && (x) <= '9' || (x) == '_')
#define ALPHA(x) \
                (std::tolower((x)) >= 'a' && std::tolower((x)) <= 'z' || (x) == '_')

        while (*token_end) {
            char *token_start = token_end;

            ImVec4 color;
            if (ins_name) {
                while (*token_end && *token_end != ' ')
                    token_end++;
                color = ImVec4(1, .7, 1, 1);
                ins_name = false;
            }
            else {
                while (*token_end && *token_end == ' ') token_end++;

                if (*token_end) {
                    if (*token_end >= '0' && *token_end <= '9') {
                        color = ImVec4(170 / 255.f, 242 / 255.f, 128 / 255.f, 1);
                        while (*token_end && ALPHANUMERIC(*token_end)) token_end++;
                    }
                    else if (*token_end == '[' || *token_end == ']' ||
                        *token_end == '+' || *token_end == '-' ||
                        *token_end == '*' || *token_end == ',' ||
                        *token_end == ':') {
                        color = ImVec4(222 / 255.f, 241 / 255.f, 247 / 255.f, 1);
                        token_end++;
                    }
                    else if (ALPHA(*token_end)) {
                        color = ImVec4(93 / 255.f, 247 / 255.f, 229 / 255.f, 1);
                        while (*token_end && ALPHANUMERIC(*token_end)) token_end++;
                    }
                }
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::SameLine(0, 0);
            ImGui::TextUnformatted(token_start, token_end);
            ImGui::PopStyleColor();
        }

        offset += instruction.length;
        addr += instruction.length;
    }*/
}

void RenderHexView(const HexView &view)
{
    int vpl = view.ValuesPerLine;
    /* Make header */
    ImGui::Text("%10c", ' ');

    for (int i = 0; i < vpl; i++) {
        ImGui::SameLine();
        ImGui::TextColored(HexAddressColor, "%02x", i);
    }

    for (int i = 0; i < view.Size; i++) {
        if (!(i % vpl))
            ImGui::TextColored(HexAddressColor, "%08x: ", view.Start + i);

        ImGui::SameLine();
        ImGui::TextColored(HexValueColor, "%02x", view.Data[i]);

        if (!((i + 1) % vpl)) {
            for (int j = vpl; j; j--) {
                ImGui::SameLine(0, -(j == vpl));
                u8 val = view.Data[i + 1 - j];
                if (val < 0x20 || val >= 0x80)
                    ImGui::TextColored(AsciiValueUnprintableColor, ".");
                else
                    ImGui::TextColored(AsciiValuePrintableColor, "%c", val);
            }
        }
    }
}

void RenderDisasmViewer()
{
    if (ImGui::Begin("Disassembler")) {
        static char start_address[128] = { 0 };
        static char size[128] = { 0 };
        static bool valid = false;
        static DisasmView view;

        ImGui::InputText("Start address", start_address, sizeof start_address);
        ImGui::InputText("Size", size, sizeof size);

        if (ImGui::Button("Refresh")) {
            try {
                view.Start = *start_address ? ExprParser(EvalContext, start_address).Evaluate() : 0;
                view.Size = *size ? ExprParser(EvalContext, size).Evaluate() : 0;
                view.Data.reserve(view.Size);
                valid = Link::ReadVirtualMem(view.Start, view.Data.data(), view.Size);
            }
            catch (std::runtime_error *err) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), err->what());
            }
        }

        if (!valid)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to read memory region");
        else
            RenderDisasmView(view);

        ImGui::End();
    }
}

void RenderMemoryViewer()
{
    if (ImGui::Begin("Memory Viewer")) {
        static char start_address[128] = { 0 };
        static char size[128] = { 0 };
        static bool valid = false;
        static HexView view;

        ImGui::InputText("Start address", start_address, sizeof start_address);
        ImGui::InputText("Size", size, sizeof size);

        if (ImGui::Button("Refresh")) {
            try {
                view.Start = *start_address ? ExprParser(EvalContext, start_address).Evaluate() : 0;
                view.Size  = *size ? ExprParser(EvalContext, size).Evaluate() : 0;
                view.Data.reserve(view.Size);
                valid = Link::ReadVirtualMem(view.Start, view.Data.data(), view.Size);
            }
            catch (std::runtime_error *err) {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), err->what());
            }
        }

        view.ValuesPerLine = 16;

        if (!valid)
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to read memory region");
        else 
            RenderHexView(view);

        ImGui::End();
    }
}

void RenderStateView(const ObjView *view, std::string postfix = "")
{
    int names = view->Names.size();
    bool one = view->Values.size() <= names;
    bool render = one;

    /* Determine the maximum-length row name */
    int max_length = !names ? 0 :
        std::strlen(
            *std::max_element(
                view->Names.begin(), view->Names.end(),
                [](const char *l, const char *r) { return std::strlen(l) < std::strlen(r); }
            )
        );

    for (int i = 0; i < view->Values.size(); i++) {
        const char *unpadded = view->Names[i % names];

        /* Pad the row name */
        std::string name =
            std::string(unpadded) +
            std::string(max_length - std::strlen(unpadded), ' ');

        if (!one && i % names == 0) {
            /* Append invisible postfix to the name so
               that imgui can distinguish between these widgets */
            std::string entry = std::string("[") +
                std::to_string(i / names) + 
                std::string("]##") + postfix +
                std::to_string(i);

            if (render && i)
                ImGui::TreePop();

            render = ImGui::TreeNode(entry.c_str());
        }

        if (render) {
            float r, g, b;
            float h = (Id += 0.03f) + Time; h -= (int)h;
            ImGui::ColorConvertHSVtoRGB(h, 0.8, 1.0, r, g, b);

            ImVec4 color = ImVec4(r, g, b, 1.0);
            const StateValue &value = view->Values[i];

            switch (value.Type) {
                case ValInt:  RENDER_VALUE(color, name.c_str(), "%i", value.Int); break;
                case ValHex:  RENDER_VALUE(color, name.c_str(), "0x%08x", value.Hex); break;
                case ValCStr: RENDER_VALUE(color, name.c_str(), "%s", value.CStr); break;
                case ValStr:  RENDER_VALUE(color, name.c_str(), "%s", value.Str.c_str()); break;
                case ValBool: RENDER_VALUE(color, name.c_str(), "%s", value.Bool ? "true" : "false"); break;
                case ValTable: {
                    ImGui::TextColored(color, "%s", name.c_str());
                    ImGui::SameLine();

                    auto inner = value.Table;
                    if (!inner) {
                        ImGui::Text("???");
                        break;
                    }

                    std::string unique_name = inner->Name;
                    unique_name += "##";
                    unique_name += postfix + std::to_string(i);

                    if (ImGui::TreeNode(unique_name.c_str())) {
                        RenderStateView(inner.get(), postfix + std::to_string(i));
                        ImGui::TreePop();
                    }
                    break;
                }
            }
        }
    }

    if (!one && render)
        ImGui::TreePop();
}

void RenderCpuInterruptState(const char *name, const CpuInterruptState &state)
{
    /*
    u32 Gs, Fs, Es, Ds;
    u32 Edi, Esi, Ebp, Esp, Ebx, Edx, Ecx, Eax;
    u32 InterruptNumber, ErrorCode;
    u32 Eip, Cs, Eflags, EspU, SsU;
    */
    if (ImGui::TreeNode(name)) {
        std::shared_ptr<ObjView> table(new ObjView(name));
        table->ValName("Eax");             table->ValueHex(state.Eax);
        table->ValName("Ecx");             table->ValueHex(state.Ecx);
        table->ValName("Edx");             table->ValueHex(state.Edx);
        table->ValName("Ebx");             table->ValueHex(state.Ebx);
        table->ValName("Esp");             table->ValueHex(state.Esp);
        table->ValName("Ebp");             table->ValueHex(state.Ebp);
        table->ValName("Esi");             table->ValueHex(state.Esi);
        table->ValName("Edi");             table->ValueHex(state.Edi);
        table->ValName("Eip");             table->ValueHex(state.Eip);
        table->ValName("Cs");              table->ValueHex(state.Cs);
        table->ValName("Ds");              table->ValueHex(state.Ds);
        table->ValName("Es");              table->ValueHex(state.Es);
        table->ValName("Fs");              table->ValueHex(state.Fs);
        table->ValName("Gs");              table->ValueHex(state.Gs);
        table->ValName("InterruptNumber"); table->ValueHex(state.InterruptNumber);
        table->ValName("ErrorCode");       table->ValueHex(state.ErrorCode);
        table->ValName("Eflags");          table->ValueHex(state.Eflags);
        table->ValName("EspU");            table->ValueHex(state.EspU);
        table->ValName("SsU");             table->ValueHex(state.SsU);

        RenderStateView(table.get());
        ImGui::TreePop();
    }
}

#undef RENDER_COLUMN_NAME
#undef RENDER_COLUMN

void RenderWindow()
{
    Id = 0;

    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(
        ClearColor.x * .5,
        ClearColor.y * .5,
        ClearColor.z * .5,
        1.f));

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(
        ClearColor.x,
        ClearColor.y,
        ClearColor.z,
        .4f));

    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(
        ClearColor.x * .75,
        ClearColor.y * .9,
        ClearColor.z,
        1.f));

    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(
        ClearColor.x * .25,
        ClearColor.y * .9,
        ClearColor.z,
        1.f));

    ImGui::Begin("Settings");
    ImGui::SetWindowSize(ImVec2(350, 100));
    ImGui::ColorEdit3("BG color", (float*)&ClearColor);
    ImGui::ColorEdit3("Field color", (float *)&FieldColor);
    ImGui::End();

    if (Link::Established) {
        if (!DebugInfo::Initialized)
            DebugInfo::Init();
        if (Link::Panicked) {
            ImGui::Begin("Break");

            ImGui::Text("Kernel panic ");
            ImGui::SameLine();

            ImGui::TextColored(ImVec4(1, 1, 0, 1), "0x%08x", Link::PanicInfo.Reason);
            ImGui::Text(
                "Message string: %s",
                Link::PanicInfo.MessageString.c_str());

            RenderStackTrace("Stack trace", Link::PanicInfo.StackTrace);
            RenderCpuInterruptState("CPU state", Link::PanicInfo.CpuState);

            ImGui::End();
        }
        else if (Link::Breakpoint) {
            ImGui::Begin("Breakpoint");

            if (ImGui::Button("OK")) {
                Link::Breakpoint = false;
                Link::Continue();
            }

            RenderStackTrace("Stack trace", Link::BreakInfo.StackTrace);
            RenderCpuInterruptState("CPU state", Link::BreakInfo.CpuState);

            if (ImGui::TreeNode("Disassembly")) {
                static DisasmView break_disview;
                static bool break_disview_valid = false;
                static bool break_disview_first = true;

                if (break_disview_first) {
                    break_disview.Start = Link::BreakInfo.CpuState.Eip & -4096;
                    break_disview.Size = 4096;
                    break_disview.Data.reserve(4096);
                    break_disview_valid = Link::ReadVirtualMem(
                        break_disview.Start, break_disview.Data.data(), break_disview.Size);
                    break_disview_first = false;
                }

                if (!break_disview_valid)
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to read memory region");
                else
                    RenderDisasmView(break_disview);

                ImGui::TreePop();
            }
            ImGui::End();

        }

        ImGui::Begin("Processes");

        if (ImGui::Button("Refresh##1")) {
            BackcheckMap.clear();
            Link::RefreshObjectList();
            LastProcessTable = ObjPrint::Processes("Show", Link::Processes);
        }
        if (auto *ptr = LastProcessTable.get()) {
            if (ImGui::TreeNode(ptr->Name)) {
                RenderStateView(ptr);
                ImGui::TreePop();
            }
        }
        ImGui::End();

        RenderMemoryViewer();
        RenderDisasmViewer();

        ImGui::Begin("Expression Evaluator");

        static char expr_buf[256] = { 0 };
        static i64 value = 0;
        ImGui::InputText("", expr_buf, 256);
        ImGui::SameLine();

        if (ImGui::Button("="))
            value = ExprParser(EvalContext, expr_buf).Evaluate();

        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "%lld (0x%llx)", value, value);
        ImGui::End();

        ImGui::Begin("Root Directory");

        if (ImGui::Button("Refresh##2")) {
            BackcheckMap.clear();
            RootDirectoryTable = ObjPrint::Directories("Show",
                std::vector<uptr> { Link::GetRootDirectory() });
        }

        if (auto *ptr = RootDirectoryTable.get()) {
            if (ImGui::TreeNode(ptr->Name)) {
                RenderStateView(ptr);
                ImGui::TreePop();
            }
        }

        ImGui::End();

        ImGui::Begin("Logs");
        ImGui::PushStyleColor(ImGuiCol_Text, LogTextColor);
        ImGui::TextUnformatted(Link::LogBuffer.c_str(), Link::LogBuffer.c_str() + Link::LogBuffer.size());
        ImGui::PopStyleColor();
        ImGui::End();
    }
    else
        ImGui::TextColored(ImVec4(1, 1, 0, 1), "Waiting for connection...");

    Time += 1 / 60.f;
    ImGui::PopStyleColor(4);
}

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    /*ZydisDecoderInit(&DisDecoder, ZYDIS_MACHINE_MODE_LEGACY_32, ZYDIS_ADDRESS_WIDTH_32);
    ZydisFormatterInit(&DisFormatter, ZYDIS_FORMATTER_STYLE_INTEL);*/

    Link::StartListening();

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    auto window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_Window* window = SDL_CreateWindow("PrizmDbg",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGL()) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    MainFont = io.Fonts->AddFontFromFileTTF("FiraCode-Medium.ttf", 20.0f);

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);


    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT &&
                event.window.event == SDL_WINDOWEVENT_CLOSE &&
                event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame(window);
        ImGui::NewFrame();
        
        RenderWindow();

        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, ClearColor.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}