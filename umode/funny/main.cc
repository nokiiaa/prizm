#include <pzapi.h>
#include "math.hh"

struct Vertex {
    float Coords[3];
    //float U, V;
    u32 RGBA;
};

u32 HueRotate(u32 color, float t)
{
    float h, s, l;
    Color(color).ToHsl(h, s, l);
    return Color::FromHsl(h + t, s, l).Raw | 0xFF000000;
}

void CalculateCube(GfxHandle vbo_handle, Vec3 center, Vec3 size, Vec3 angle, float t)
{
    Vec3 a = Vec3 { center.X - size.X, center.Y - size.Y, center.Z - size.Z }.Rotated(angle, center);
    Vec3 b = Vec3 { center.X - size.X, center.Y + size.Y, center.Z - size.Z }.Rotated(angle, center);
    Vec3 c = Vec3 { center.X + size.X, center.Y - size.Y, center.Z - size.Z }.Rotated(angle, center);
    Vec3 d = Vec3 { center.X + size.X, center.Y + size.Y, center.Z - size.Z }.Rotated(angle, center);
    Vec3 e = Vec3 { center.X - size.X, center.Y - size.Y, center.Z + size.Z }.Rotated(angle, center);
    Vec3 f = Vec3 { center.X - size.X, center.Y + size.Y, center.Z + size.Z }.Rotated(angle, center);
    Vec3 g = Vec3 { center.X + size.X, center.Y - size.Y, center.Z + size.Z }.Rotated(angle, center);
    Vec3 h = Vec3 { center.X + size.X, center.Y + size.Y, center.Z + size.Z }.Rotated(angle, center);

    Vertex va { a.X, a.Y, a.Z, HueRotate(0xFFFF0000, t) };
    Vertex vb { b.X, b.Y, b.Z, HueRotate(0xFF00FF00, t) };
    Vertex vc { c.X, c.Y, c.Z, HueRotate(0xFF0000FF, t) };
    Vertex vd { d.X, d.Y, d.Z, HueRotate(0xFFFF00FF, t) };
    Vertex ve { e.X, e.Y, e.Z, HueRotate(0xFFFFFF7F, t) };
    Vertex vf { f.X, f.Y, f.Z, HueRotate(0xFF00FFFF, t) };
    Vertex vg { g.X, g.Y, g.Z, HueRotate(0xFFFFFF00, t) };
    Vertex vh { h.X, h.Y, h.Z, HueRotate(0xFF7FFF00, t) };

    Vertex cube[] = {
        va, vc, vb,
        vc, vd, vb,
        ve, va, vf,
        va, vb, vf,
        vc, vg, vd,
        vg, vh, vd,
        vb, vd, vf,
        vd, vh, vf,
        ve, vg, va,
        vg, vc, va,
        ve, vg, vf,
        vg, vh, vf,
    };

    //PzPutStringFormatted("\r\n[funny.exe] GfxVertexBufferData: %i\r\n",
    GfxVertexBufferData(vbo_handle, cube, sizeof(cube));
}

template<typename T> inline void Swap(T &a, T &b)
{
    T t = a;
    a = b;
    b = t;
}

extern "C" PzStatus PzProcessStartup(void *base)
{
    PzIoStatusBlock io_status;
    PzHandle gfx_handle, bmp_handle;

    PzPutStringFormatted("Hello from usermode! I am funny.exe\r\n");

    static PzString gfx_device = PZ_CONST_STRING("Device\\Gfx\\Framebuffer");
    static PzString bmp_file   = PZ_CONST_STRING("SystemDir\\image.bmp");

    u32 *image = nullptr;
    if (PzCreateFile(&gfx_handle, &gfx_device, &io_status, ACCESS_WRITE, OPEN_EXISTING) == STATUS_SUCCESS)
        PzPutStringFormatted("\r\n[funny.exe] Successfully opened framebuffer device: %i\r\n", gfx_handle);

    u32 fb_size = 640 * 480 * 4;
    u64 offset = 0x96;

    if (PzAllocateVirtualMemory(CPROC_HANDLE,
        (void **)&image, fb_size, PAGE_READWRITE) == STATUS_SUCCESS)
        PzPutStringFormatted("[funny.exe] Successfully allocated %i bytes for bmp file: 0x%p\r\n",
            fb_size, image);

    if (PzCreateFile(&bmp_handle, &bmp_file, &io_status, ACCESS_READ, OPEN_EXISTING) == STATUS_SUCCESS)
        PzPutStringFormatted("\r\n[funny.exe] Successfully opened bmp file: %i\r\n", bmp_handle);

    //if (PzReadFile(bmp_handle, image, &io_status, fb_size, &offset) == STATUS_SUCCESS)
    //    PzPutStringFormatted("\r\n[funny.exe] Successfully read bmp file: %i\r\n");

    /*static PzString mutex_name = PZ_CONST_STRING("FrameMutex");
    for (;;) {
        for (int i = 0; i < 50; i++)
        for (int i = 0; i < fb_size; i++)
            ((u8 *)image)[i] *= 0x80f935ab;
        u64 offset = 0;
        PzWriteFile(gfx_handle, image, &io_status, fb_size, &offset);
    }*/

    GfxHandle vbo_handle, texture_handle;
    GfxHandle render_handles[2];

    PzPutStringFormatted("\r\n[funny.exe] GfxGenerateBuffers: %i\r\n",
        GfxGenerateBuffers(GFX_RENDER_BUFFER, 2, render_handles));

    PzPutStringFormatted("\r\n[funny.exe] GfxGenerateBuffers: %i\r\n",
        GfxGenerateBuffers(GFX_VERTEX_BUFFER, 1, &vbo_handle));

    PzPutStringFormatted("\r\n[funny.exe] GfxGenerateBuffers: %i\r\n",
        GfxGenerateBuffers(GFX_TEXTURE_BUFFER, 1, &texture_handle));

    struct Vertex {
        float Coords[3];
        float U, V;
    } data[] = {
        {0.f, 0.f, 1.f,     0.f, 0.f},
        {1.f, 0.2f, 1.f,    1.f, 0.f},
        {0.f, 1.0f, 1.f,    0.f, 1.f},
        {-0.5f, -0.5f, 1.f, 1.f, 1.f},
    };

    GfxTextureData(texture_handle, 320, 200, GFX_PIXEL_ARGB32, 0, image);

    PzPutStringFormatted("\r\n[funny.exe] GfxRenderBufferData: %i\r\n",
        GfxRenderBufferData(render_handles[0], 320, 200, GFX_PIXEL_ARGB32));

    PzPutStringFormatted("\r\n[funny.exe] GfxRenderBufferData: %i\r\n",
        GfxRenderBufferData(render_handles[1], 320, 200, GFX_PIXEL_ARGB32));

    PzHandle timer;
    PzCreateTimer(&timer, nullptr);
    Math::InitSineTable();

    PzHandle wnd_handle, lbl_handle;
    const PzString title = PZ_CONST_STRING("Prizm!");
    const PzString label = PZ_CONST_STRING("It is an operating system, isn't it?");

    PzPutStringFormatted("[funny.exe] PzCreateWindow: %i\r\n",
        PzCreateWindow(&wnd_handle, 0, &title, 100, 100, 0, 320, 200, 0));

    PzPutStringFormatted("[funny.exe] PzSetWindowBuffer: %i\r\n",
        PzSetWindowBuffer(wnd_handle, 0, render_handles[0]));

    PzPutStringFormatted("[funny.exe] PzSetWindowBuffer: %i\r\n",
        PzSetWindowBuffer(wnd_handle, 1, render_handles[1]));

    PzPutStringFormatted("[funny.exe] PzCreateWindow: %i\r\n",
        PzCreateWindow(&lbl_handle, wnd_handle, &label, 100, 100, 0, 120, 16, 1));

    for (float t = 0;; t += .025f / 4) {
        GfxHandle current_handle = render_handles[1];
        //PzPutStringFormatted("\r\n[funny.exe] PzGetWindowBuffer: %i\r\n",
        PzSetWindowParameter(wnd_handle, WINDOW_POS_X, (uptr)(100 + Math::Cos(t) * 40));
        PzSetWindowParameter(wnd_handle, WINDOW_POS_Y, (uptr)(100 + Math::Sin(t) * 40));
        //PzPutStringFormatted("\r\n[funny.exe] GfxClear: %i\r\n",
        GfxClear(current_handle, GFX_CLEAR_DEPTH);
        GfxClearColor(current_handle, 0xFF000000);
        CalculateCube(vbo_handle, Vec3{ 0, 0, 2 }, Vec3{ .5, .5, .5 }, Vec3{ t, t + 2, t + 4 }, t * 0.125f);
        //PzPutStringFormatted("\r\n[funny.exe] GfxDrawPrimitives: %i\r\n",
        GfxDrawPrimitives(current_handle, vbo_handle, 0, GFX_TRI_LIST_COL_FLOAT, 0, 36);
        //PzPutStringFormatted("\r\n[funny.exe] PzSwapBuffers: %i\r\n",
        PzSwapBuffers(wnd_handle);
        Swap(render_handles[0], render_handles[1]);
        //PzResetTimer(timer, 10);
        //PzWaitForObject(timer);
    }

    return STATUS_SUCCESS;
}