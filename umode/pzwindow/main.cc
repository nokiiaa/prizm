#include <pzapi.h>
#include <list.h>
#include <font_render.hh>
#include "math.hh"

#define GFX_FRAMEBUFFER_DATA 1

struct ScreenData
{
    u32 Width, Height;
    u32 Pitch, PixelSize;
} data;

PzHandle font_texture;

void RenderWindow(PzHandle window, GfxHandle render_buffer)
{
    int sw, sh;
    GfxHandle front_handle;
    uptr width, height, x, y, z;
    u32 style;

    PzGetWindowParameter(window, WINDOW_WIDTH, &width);
    PzGetWindowParameter(window, WINDOW_HEIGHT, &height);
    PzGetWindowParameter(window, WINDOW_POS_X, &x);
    PzGetWindowParameter(window, WINDOW_POS_Y, &y);
    PzGetWindowParameter(window, WINDOW_POS_Z, (uptr *)&z);
    PzGetWindowParameter(window, WINDOW_STYLE, &style);

    u32 size = 0;
    char title[256];
    PzHandle children[64];
    u32 child_count = 0;
    GfxHandle inner_buffer;

    int str_x, str_y;

    switch (style) {
        /* Bordered window */
    case 0:
        PzGetWindowTitle(window, nullptr, &size);
        //title = (char *)PzAllocateHeapMemory(nullptr, size, 0);
        PzGetWindowTitle(window, title, &size);
        PzGetWindowBuffer(window, 0, &front_handle);
        PzEnumerateChildWindows(window, &child_count, nullptr, false);

        if (child_count) {
            //children = (PzHandle *)PzAllocateHeapMemory(nullptr,
            //    child_count * sizeof(PzHandle), 0);
            PzEnumerateChildWindows(window, &child_count, children, false);
        }

        GfxGenerateBuffers(GFX_RENDER_BUFFER, 1, &inner_buffer);
        GfxRenderBufferData(inner_buffer, width, height, GFX_PIXEL_ARGB32);

        GfxBitBlit(inner_buffer, 0, 0, width, height, front_handle, 0, 0);

        for (int i = 0; i < child_count; i++) {
            RenderWindow(children[i], inner_buffer);
            PzCloseHandle(children[i]);
        }

        /* Drop shadow */
        GfxDrawRectangle(render_buffer, 0, x + 4, y,
            width + 4, height + 9, ARGB(127, 0, 0, 0), false, nullptr);
        /* Border outline */
        GfxDrawRectangle(render_buffer, 0, x - 4, y - 23,
           width + 4 + 4, height + 23 + 4, ARGB(255, 0, 55, 127), false, nullptr);
        /* Border */
        GfxDrawRectangle(render_buffer, 0, x - 2, y - 21,
            width + 2 + 2, height + 21 + 2, ARGB(255, 0, 66, 255), false, nullptr);

        str_x = x;
        str_y = y - 20;

        FontRenderString(render_buffer, font_texture, str_x, str_y,
            title, 64, str_x, x, x + width, y - 20, y);
        //if (child_count)
        //    PzFreeHeapMemory(nullptr, children);
        //PzFreeHeapMemory(nullptr, title);
        GfxBitBlit(render_buffer, x, y, width, height, inner_buffer, 0, 0);
        PzCloseHandle(front_handle);
        PzCloseHandle(inner_buffer);
        break;

        /* Label */
    case 1:
        PzGetWindowTitle(window, nullptr, &size);
        //title = (char *)PzAllocateHeapMemory(nullptr, size, 0);

        PzGetWindowTitle(window, title, &size);
        FontGetStringSize(title, size, sw, sh);

        str_x = x + (width - sw) / 2;
        str_y = y + (height - sh) / 2;

        FontRenderString(render_buffer,
            font_texture,
            str_x, str_y,
            title, 64, str_x,
            x, x + width, y, y + height);
        
        //PzFreeHeapMemory(nullptr, title);
        break;
    }
}

extern "C" PzStatus PzProcessStartup(void *base)
{
    PzIoStatusBlock io_status;
    PzHandle file_handle;

    Math::InitSineTable();

    FontOpenTexture(&font_texture);

    printf("\r\nPzRegisterWindowServer returned %i\r\n",
        PzRegisterWindowServer());

    static PzString gfx_device = PZ_CONST_STRING("Device\\Gfx\\Framebuffer");
    u32 *framebuffer = nullptr;

    if (PzStatus cf = PzCreateFile(&file_handle, &gfx_device, &io_status, ACCESS_WRITE, OPEN_EXISTING))
        return cf;

    if (PzStatus ioctl = PzDeviceIoControl(file_handle, &io_status,
        GFX_FRAMEBUFFER_DATA, nullptr, 0, &data, sizeof(ScreenData)))
        return ioctl;

    PzCloseHandle(file_handle);

    GfxHandle render_buffer;
    GfxGenerateBuffers(GFX_RENDER_BUFFER, 1, &render_buffer);
    GfxRenderBufferData(render_buffer, data.Width, data.Height, GFX_PIXEL_ARGB32);

    LinkedList windows;
    LLInitialize(&windows); 

    for (;;) {
        PzMessage message;
        GfxClear(render_buffer, GFX_CLEAR_DEPTH);
        GfxClearColor(render_buffer, ARGB(255, 255, 0, 0));

        if (PzReceiveThreadMessage(&message) == STATUS_SUCCESS) {
            if (message.Type == 1) {
                uptr width, height, x, y, z;

                PzGetWindowParameter(message.Params[0], WINDOW_WIDTH, &width);
                PzGetWindowParameter(message.Params[0], WINDOW_HEIGHT, &height);
                PzGetWindowParameter(message.Params[0], WINDOW_POS_X, &x);
                PzGetWindowParameter(message.Params[0], WINDOW_POS_Y, &y);
                PzGetWindowParameter(message.Params[0], WINDOW_POS_Z, &z);

                printf(
                    "Window server found new window: width=%i height=%i x=%i y=%i z=%i\r\n",
                    width, height, x, y, z);

                LLAdd(&windows, &message.Params[0], sizeof(PzHandle));
            }
        }

        ENUM_LIST(node, windows) {
            PzHandle window = NODE_VALUE(node, PzHandle);
            RenderWindow(window, render_buffer);
        }

        GfxUploadToDisplay(render_buffer);
    }

    return STATUS_SUCCESS;
}