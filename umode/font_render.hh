#include <pzapi.h>

void FontRenderChar(GfxHandle surface, GfxHandle font_texture, int x, int y, char c)
{
    int tx = (c % 16) * 9;
    int ty = (c / 16) * 17;

    int uvs[] = {
        tx, ty,
        tx + 8, ty,
        tx, ty + 16,
        tx + 8, ty + 16,
    };

    GfxDrawRectangle(surface, font_texture, x, y, 8, 16, 0, true, uvs);
}

void FontRenderString(
    GfxHandle surface, GfxHandle font_texture, int& x, int& y,
    const char *str, int length, int xwrap,
    int lx, int rx, int ty, int by)
{
    for (int i = 0; i < length && *str; i++, str++) {
        if (*str == '\n')
            y += 16;
        else if (*str == '\r')
            x = xwrap;
        else {
            if (x >= rx) {
                x = xwrap;
                y += 16;
            }
            if (lx <= x && x + 8 <= rx && ty <= y && y + 16 <= by)
                FontRenderChar(surface, font_texture, x, y, *str);
            x += 8;
        }
    }
}

void FontGetStringSize(const char *str, int length, int &w, int &h)
{
    int width = 0, cur_width = 0, height;

    for (int i = 0; i < length && *str; i++, str++) {

        if (*str == '\n')
            height += 16;
        else if (*str == '\r')
            cur_width = 0;
        else
            cur_width += 8;

        if (width < cur_width)
            width = cur_width;
    }

    w = width;
    h = height + 16;
}

PzStatus FontOpenTexture(GfxHandle *font_texture)
{
    u32 *font_data = nullptr;
    PzHandle font_handle;
    static PzString font_file = PZ_CONST_STRING("SystemDir\\default_font.bmp");
    u64 offset = 0x96;
    u32 font_file_size = 0x26496;
    u32 font_pixel_size = font_file_size - offset;
    PzIoStatusBlock io_status;

    PzAllocateVirtualMemory(CPROC_HANDLE,
        (void **)&font_data, font_pixel_size, PAGE_READWRITE);

    if (PzStatus cf = PzCreateFile(&font_handle, &font_file, &io_status, ACCESS_READ, OPEN_EXISTING))
        return cf;

    if (PzStatus cf = PzReadFile(font_handle, font_data, &io_status, font_file_size, &offset))
        return cf;

    for (int i = 0; i < font_pixel_size / 4; i++)
        font_data[i] = (font_data[i] & 1) * ARGB(255, 255, 255, 255);

    GfxGenerateBuffers(GFX_TEXTURE_BUFFER, 1, font_texture);
    GfxTextureData(*font_texture, 144, 272, GFX_PIXEL_ARGB32, 0, font_data);
    return STATUS_SUCCESS;
}