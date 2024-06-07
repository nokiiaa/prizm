#pragma once

#include <defs.hh>

#define GFX_RENDER_BUFFER  1
#define GFX_VERTEX_BUFFER  2
#define GFX_TEXTURE_BUFFER 3

#define GFX_PIXEL_ARGB32 1

#define GFX_FORMAT_COL    (0U << 29)
#define GFX_FORMAT_UV     (1U << 29)
#define GFX_FORMAT_INT    (0U << 30)
#define GFX_FORMAT_FLOAT  (1U << 30)

#define GFX_CLEAR_COLOR 1
#define GFX_CLEAR_DEPTH 2

#define DEF_FORMAT_4(x, n) \
    GFX_##x             = n, \
    GFX_##x##_COL       = n | GFX_FORMAT_COL, \
    GFX_##x##_UV        = n | GFX_FORMAT_UV, \
    GFX_##x##_COL_FLOAT = n | GFX_FORMAT_COL | GFX_FORMAT_FLOAT, \
    GFX_##x##_UV_FLOAT  = n | GFX_FORMAT_UV  | GFX_FORMAT_FLOAT,

enum {
    DEF_FORMAT_4(TRI_STRIP, 0)
    DEF_FORMAT_4(TRI_LIST, 1)
};

typedef int GfxHandle;

struct GfxRenderBuffer {
    u32 PixelSize, ZSize;
    int Width, Height;
    int PixelFormat;
    bool HasData;
    void *PixelBufferDescriptor;
    void *ZBufferDescriptor;
};

struct GfxVertexBuffer {
    u32 Size;
    bool HasData;
    void *MemoryDescriptor;
};

struct GfxTextureBuffer {
    u32 Size;
    int Width, Height;
    int PixelFormat;
    u32 Flags;
    bool HasData;
    void *MemoryDescriptor;
};

struct GfxVertexFormat {
    union { float FloatCoords[3]; int IntCoords[3]; };
    union {
        struct { float FloatU, FloatV; };
        struct { int IntU, IntV; };
        struct { u8 R, G, B, A; };
    };
};

struct PzGraphicsObject;

PzStatus GfxInitializeDriver();
PZ_KERNEL_EXPORT void GfxDestroyObject(PzGraphicsObject *gfx);
PZ_KERNEL_EXPORT PzStatus GfxGenerateBuffers(int type, int count, GfxHandle *handles);
PZ_KERNEL_EXPORT PzStatus GfxTextureData(GfxHandle handle,
    int width, int height, int pixel_format, int flags, void *data);
PZ_KERNEL_EXPORT PzStatus GfxTextureFlags(GfxHandle handle, u32 flags);
PZ_KERNEL_EXPORT PzStatus GfxVertexBufferData(GfxHandle handle, void *data, u32 size);
PZ_KERNEL_EXPORT PzStatus GfxDrawTriangles(
    GfxHandle render_handle, GfxHandle vbo_handle, GfxHandle texture_handle,
    int data_format, int start_index, int vertex_count);
PZ_KERNEL_EXPORT PzStatus GfxDrawRectangle(
    GfxHandle render_handle, GfxHandle texture_handle,
    int x, int y, int width, int height, u32 color, bool int_uvs, const void *uvs);
PZ_KERNEL_EXPORT PzStatus GfxClearColor(GfxHandle render_handle, u32 color);
PZ_KERNEL_EXPORT PzStatus GfxClear(GfxHandle render_handle, u32 flags);
PZ_KERNEL_EXPORT PzStatus GfxBitBlit(
    GfxHandle dest_buffer, int dx, int dy, int dw, int dh,
    GfxHandle src_buffer, int sx, int sy);
PZ_KERNEL_EXPORT PzStatus GfxUploadToDisplay(GfxHandle render_buffer);
PZ_KERNEL_EXPORT PzStatus GfxRenderBufferData(GfxHandle handle,
    int width, int height,
    int pixel_format);