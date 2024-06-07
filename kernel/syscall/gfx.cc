#include <syscall/gfx.hh>

DECL_SYSCALL(UmGfxGenerateBuffers)
{
    auto *prm = (UmGfxGenerateBuffersParams*)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm->Handles, prm->Count * sizeof(PzHandle), true))
        return STATUS_INVALID_ARGUMENT;

    return GfxGenerateBuffers(prm->Type, prm->Count, prm->Handles);
}

DECL_SYSCALL(UmGfxTextureData)
{
    auto *prm = (UmGfxTextureDataParams *)params;

    if (!MmVirtualProbeMemory(true, (uptr)prm->Data, prm->Width * prm->Height * sizeof(u32), false))
        return STATUS_INVALID_ARGUMENT;

    return GfxTextureData(prm->Handle, prm->Width, prm->Height, prm->PixelFormat, prm->Flags, prm->Data);
}

DECL_SYSCALL(UmGfxTextureFlags)
{
    auto *prm = (UmGfxTextureFlagsParams *)params;
    return GfxTextureFlags(prm->Handle, prm->Flags);
}

DECL_SYSCALL(UmGfxVertexBufferData)
{
    auto *prm = (UmGfxVertexBufferDataParams *)params;
    if (!MmVirtualProbeMemory(true, (uptr)prm->Data, prm->Size, false))
        return STATUS_INVALID_ARGUMENT;
    return GfxVertexBufferData(prm->Handle, prm->Data, prm->Size);
}

DECL_SYSCALL(UmGfxDrawPrimitives)
{
    auto *prm = (UmGfxDrawPrimitivesParams *)params;
    return GfxDrawTriangles(prm->RenderHandle, prm->VboHandle,
        prm->TextureHandle, prm->DataFormat, prm->StartIndex, prm->VertexCount);
}

DECL_SYSCALL(UmGfxDrawRectangle)
{
    auto *prm = (UmGfxDrawRectangleParams *)params;
    return GfxDrawRectangle(prm->RenderHandle, prm->TextureHandle,
        prm->X, prm->Y, prm->Width, prm->Height, prm->Color, prm->IntUVs, prm->UVs);
}

DECL_SYSCALL(UmGfxClearColor)
{
    auto *prm = (UmGfxClearColorParams *)params;
    return GfxClearColor(prm->RenderHandle, prm->Color);
}

DECL_SYSCALL(UmGfxClear)
{
    auto *prm = (UmGfxClearParams *)params;
    return GfxClear(prm->RenderHandle, prm->Flags);
}

DECL_SYSCALL(UmGfxBitBlit)
{
    auto *prm = (UmGfxBitBlitParams *)params;
    return GfxBitBlit(prm->DestBuffer, prm->Dx, prm->Dy, prm->Dw, prm->Dh, prm->SrcBuffer, prm->Sx, prm->Sy);
}

DECL_SYSCALL(UmGfxUploadToDisplay)
{
    auto *prm = (UmGfxUploadToDisplayParams *)params;
    return GfxUploadToDisplay(prm->RenderBuffer);
}

DECL_SYSCALL(UmGfxRenderBufferData)
{
    auto *prm = (UmGfxRenderBufferDataParams *)params;
    return GfxRenderBufferData(prm->Handle, prm->Width, prm->Height, prm->PixelFormat);
}