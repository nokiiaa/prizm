# Graphics driver
Supposed to implement a basic graphics subsystem.
Right now, only Device\Gfx\Framebuffer may be opened, which directs reads and writes to the VBE framebuffer. The bounds and pixel format of the display is retrieved through I/O control code GFX_FRAMEBUFFER_DATA (1). The format of the output buffer in that case is as follows:
```c
struct ScreenData {
    u32 Width, Height;
    u32 Pitch, PixelSize;
};
```