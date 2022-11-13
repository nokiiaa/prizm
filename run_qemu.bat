@echo off
qemu-system-i386 -d int -soundhw ac97 -drive format=raw,file=\\.\PhysicalDrive1,index=0,media=disk -monitor stdio -no-shutdown -accel hax
pause