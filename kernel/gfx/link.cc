#include <gfx/link.hh>
#include <lib/string.hh>
#include <io/manager.hh>

#define DECL_DRV_FUNCTION(name) static decltype(name) *Drv##name;

DECL_DRV_FUNCTION(GfxGenerateBuffers)
DECL_DRV_FUNCTION(GfxTextureData)
DECL_DRV_FUNCTION(GfxTextureFlags)
DECL_DRV_FUNCTION(GfxVertexBufferData)
DECL_DRV_FUNCTION(GfxDrawTriangles)
DECL_DRV_FUNCTION(GfxDrawRectangle)
DECL_DRV_FUNCTION(GfxClearColor)
DECL_DRV_FUNCTION(GfxBitBlit)
DECL_DRV_FUNCTION(GfxUploadToDisplay)
DECL_DRV_FUNCTION(GfxClear)
DECL_DRV_FUNCTION(GfxRenderBufferData)
DECL_DRV_FUNCTION(GfxDestroyObject)

PzStatus GfxInitializeDriver()
{
    const PzString fb_driver = PZ_CONST_STRING("SystemDir\\gfx.sys");
    PzDriverObject *driver;
    if (PzStatus drv_load = IoLoadDriver(&fb_driver, &driver))
        return drv_load;
#define IMPORT_FUNCTION(name) if (!(Drv##name = \
    (decltype(Drv##name))LdriGetSymbolAddress(driver->AssociatedModule, #name))) return STATUS_FAILED;

    IMPORT_FUNCTION(GfxGenerateBuffers)
    IMPORT_FUNCTION(GfxTextureData)
    IMPORT_FUNCTION(GfxTextureFlags)
    IMPORT_FUNCTION(GfxVertexBufferData)
    IMPORT_FUNCTION(GfxDrawTriangles)
    IMPORT_FUNCTION(GfxDrawRectangle)
    IMPORT_FUNCTION(GfxClearColor)
    IMPORT_FUNCTION(GfxBitBlit)
    IMPORT_FUNCTION(GfxClear)
    IMPORT_FUNCTION(GfxUploadToDisplay)
    IMPORT_FUNCTION(GfxRenderBufferData)
    IMPORT_FUNCTION(GfxDestroyObject)

#undef IMPORT_FUNCTION
    return STATUS_SUCCESS;
}

void GfxDestroyObject(PzGraphicsObject *gfx)
{
    return DrvGfxDestroyObject(gfx);
}

PzStatus GfxGenerateBuffers(int type, int count, GfxHandle *handles)
{
    return DrvGfxGenerateBuffers(type, count, handles);
}

PzStatus GfxTextureData(GfxHandle handle,
    int width, int height,
    int pixel_format, int flags, void *data)
{
    return DrvGfxTextureData(handle, width, height, pixel_format, flags, data);
}

PzStatus GfxTextureFlags(GfxHandle handle, u32 flags)
{
    return DrvGfxTextureFlags(handle, flags);
}

PzStatus GfxVertexBufferData(GfxHandle handle, void *data, u32 size)
{
    return DrvGfxVertexBufferData(handle, data, size);
}

PzStatus GfxDrawTriangles(
    GfxHandle render_handle, GfxHandle vbo_handle,
    GfxHandle texture_handle, int data_format, int start_index, int vertex_count)
{
    return DrvGfxDrawTriangles(
        render_handle, vbo_handle, texture_handle, data_format, start_index, vertex_count);
}

PzStatus GfxDrawRectangle(
    GfxHandle render_handle, GfxHandle texture_handle,
    int x, int y, int width, int height, u32 color, bool int_uvs, const void *uvs)
{
    return DrvGfxDrawRectangle(render_handle, texture_handle,
        x, y, width, height, color, int_uvs, uvs);
}

PzStatus GfxClear(GfxHandle render_handle, u32 flags)
{
    return DrvGfxClear(render_handle, flags);
}

PzStatus GfxClearColor(GfxHandle render_handle, u32 color)
{
    return DrvGfxClearColor(render_handle, color);
}

PzStatus GfxBitBlit(
    GfxHandle dest_buffer, int dx, int dy, int dw, int dh,
    GfxHandle src_buffer, int sx, int sy)
{
    return DrvGfxBitBlit(dest_buffer, dx, dy, dw, dh, src_buffer, sx, sy);
}

PzStatus GfxUploadToDisplay(GfxHandle render_buffer)
{
    return DrvGfxUploadToDisplay(render_buffer);
}

PzStatus GfxRenderBufferData(GfxHandle handle,
    int width, int height,
    int pixel_format)
{
    return DrvGfxRenderBufferData(handle, width, height, pixel_format);
}