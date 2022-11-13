#pragma once

#include <io/manager.hh>
#include <lib/string.hh>
#include <syscall/handle.hh>
#include <gfx/link.hh>
#include <defs.hh>
#include <core.hh>

struct UmGfxGenerateBuffersParams {
    int Type;
    int Count;
    GfxHandle *Handles;
};

struct UmGfxTextureDataParams {
    GfxHandle Handle;
    int Width, Height, PixelFormat, Flags;
    void *Data;
};

struct UmGfxTextureFlagsParams {
    GfxHandle Handle;
    u32 Flags;
};

struct UmGfxVertexBufferDataParams {
    GfxHandle Handle;
    void *Data;
    u32 Size;
};

struct UmGfxDrawPrimitivesParams {
    GfxHandle RenderHandle;
    GfxHandle VboHandle;
    GfxHandle TextureHandle;
    int DataFormat;
    int StartIndex;
    int VertexCount;
};

struct UmGfxClearColorParams {
    GfxHandle RenderHandle;
    u32 Color;
};

struct UmGfxClearParams {
    GfxHandle RenderHandle;
    u32 Flags;
};

struct UmGfxBitBlitParams {
    GfxHandle DestBuffer; int Dx, Dy, Dw, Dh;
    GfxHandle SrcBuffer; int Sx, Sy;
};

struct UmGfxUploadToDisplayParams {
    GfxHandle RenderBuffer;
};

struct UmGfxRenderBufferDataParams {
    GfxHandle Handle;
    int Width, Height, PixelFormat;
};

struct UmGfxDrawRectangleParams {
    GfxHandle RenderHandle;
    GfxHandle TextureHandle;
    int X, Y, Width, Height;
    u32 Color;
    bool IntUVs;
    const void *UVs;
};

DECL_SYSCALL(UmGfxGenerateBuffers);
DECL_SYSCALL(UmGfxTextureData);
DECL_SYSCALL(UmGfxTextureFlags);
DECL_SYSCALL(UmGfxVertexBufferData);
DECL_SYSCALL(UmGfxDrawPrimitives);
DECL_SYSCALL(UmGfxDrawRectangle);
DECL_SYSCALL(UmGfxClearColor);
DECL_SYSCALL(UmGfxClear);
DECL_SYSCALL(UmGfxBitBlit);
DECL_SYSCALL(UmGfxUploadToDisplay);
DECL_SYSCALL(UmGfxRenderBufferData);