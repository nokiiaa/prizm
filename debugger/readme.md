# PrizmDbg
A debugger for the Prizm kernel.
## Supports
- Viewing kernel objects, such as processes, threads, modules, etc.
- Hex/disassembly view of arbitrary virtual memory
- Expression evaluation with support for reading/writing guest memory with read8..64(addr) and write8..64(addr, val) commands
- Logging DbgPrintStr events
- Handling kernel panics with stack traces
- Loading kernel symbols
- Setting breakpoints
## Dependencies
- ImGui for graphics (included in the project source files)
- ~~Zydis for disassembly (x64 Windows built copy included in the lib directory)~~ (currently doesn't build, disassembly functionality is commented out)
- SDL as backend for ImGui (x64 Windows built copy included in the lib directory)
- Currently uses Win32-only I/O and threading functions (this will be fixed in the future when making it more cross-platform)