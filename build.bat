@echo off
make clean
make build
make dbginfo
cd bin
copy pzkernel.exe I:\pzkernel.exe
copy bootloader_p2.bin I:\part2.bin
copy disk.sys I:\disk.sys
copy fat.sys I:\fat.sys
copy gfx.sys I:\gfx.sys
copy pzdll.dll I:\pzdll.dll
copy pzinit.exe I:\pzinit.exe
copy funny.exe I:\funny.exe
copy pzwindow.exe I:\pzwindow.exe
copy pzconhost.exe I:\pzconhost.exe
copy pzshell.exe I:\pzshell.exe
cd..
pause