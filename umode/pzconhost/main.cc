#include <pzapi.h>
#include <list.h>
#include <font_render.hh>
#include "math.hh"

PzHandle font_texture;

struct ConsoleWindow
{
    uptr OwnerProcessId;
    PzHandle WindowHandle;
    PzHandle RenderBuffer1, RenderBuffer2;
    PzHandle StdOutRead, StdInWrite;
    int XPointer, YPointer, WinWidth, WinHeight;
    char *TextBuffer;
    int XScroll, YScroll;
    int TextBufferWidth, TextBufferHeight;
};

LinkedList windows;

template<typename T> inline void Swap(T &a, T &b)
{
    T t = a;
    a = b;
    b = t;
}

int ConWindowThread(void *param)
{
    auto *conWindow = (ConsoleWindow *)param;

    int width  = 320;
    int height = 200;
    int bufferHeight = 1600;

    conWindow->TextBufferWidth = width / 8;
    conWindow->TextBufferHeight = bufferHeight / 16;

    conWindow->TextBuffer = (char*)PzAllocateHeapMemory(nullptr,
        conWindow->TextBufferWidth * conWindow->TextBufferHeight, 0);

    conWindow->XPointer = 0;
    conWindow->YPointer = 0;
    conWindow->WinWidth = width;
    conWindow->WinHeight = height;

    PzString testName = PZ_CONST_STRING("conhost");

    GfxHandle render_handles[2];

    GfxGenerateBuffers(GFX_RENDER_BUFFER, 2, render_handles);

    GfxRenderBufferData(render_handles[0], width, height, GFX_PIXEL_ARGB32);
    GfxRenderBufferData(render_handles[1], width, height, GFX_PIXEL_ARGB32);

    GfxClearColor(render_handles[0], ARGB(255, 0, 0, 0));
    GfxClearColor(render_handles[1], ARGB(255, 0, 0, 0));

    PzStatus wndCreate = PzCreateWindow(&conWindow->WindowHandle, 0,
        &testName, 200, 200, 0, width, height, 0);

    PzSetWindowBuffer(conWindow->WindowHandle, 0, conWindow->RenderBuffer1 = render_handles[0]);
    PzSetWindowBuffer(conWindow->WindowHandle, 1, conWindow->RenderBuffer2 = render_handles[1]);

    for (;;)
    {
        PzIoStatusBlock iosb;
        char ch;

        PzReadFile(conWindow->StdOutRead, &ch, &iosb, 1, nullptr);
        
        switch (ch) {
        case '\r': conWindow->XPointer = 0; break;
        case '\n': conWindow->YPointer++; conWindow->XPointer = 0; break;
        case '\t': conWindow->XPointer += 4; break;
        default:
            conWindow->TextBuffer[conWindow->YPointer *
                conWindow->TextBufferWidth + conWindow->XPointer] = ch;

            if (conWindow->XPointer++ >= conWindow->TextBufferWidth) {
                conWindow->XPointer = 0;
                conWindow->YPointer++;
            }
            break;
        }

        int tbw = conWindow->TextBufferWidth;
        int tbh = conWindow->TextBufferHeight;
        int heightOnScreen = conWindow->WinHeight / 16;
        int widthOnScreen  = conWindow->WinWidth  / 8;

        if (conWindow->YPointer >= tbh)
        {
            MemMove(conWindow->TextBuffer, conWindow->TextBuffer + tbw,
                tbw * (tbh - 1));
            MemSet(conWindow->TextBuffer + tbw * (tbh - 1), 0, tbw);
            conWindow->YPointer--;
        }

        if (conWindow->YPointer >= heightOnScreen)
            conWindow->YScroll = (conWindow->YPointer - heightOnScreen);

        GfxClearColor(conWindow->RenderBuffer2, ARGB(255, 0, 0, 0));

        int xs = conWindow->XScroll, ys = conWindow->YScroll;

        for (int y = ys; y < ys + heightOnScreen; y++) {
            for (int x = xs; x < xs + widthOnScreen; x++) {
                char ch = conWindow->TextBuffer[y * conWindow->TextBufferWidth + x];

                if (ch && ch != ' ')
                    FontRenderChar(conWindow->RenderBuffer2, font_texture,
                        (x - xs) * 8, (y - ys) * 16, ch);
            }
        }

        FontRenderChar(conWindow->RenderBuffer2, font_texture,
            (conWindow->XPointer - xs) * 8, (conWindow->YPointer - ys) * 16, '_');

        Swap(conWindow->RenderBuffer1, conWindow->RenderBuffer2);
        PzSwapBuffers(conWindow->WindowHandle);
    }

    return 0;
}

extern "C" PzStatus PzProcessStartup(void *base)
{
    FontOpenTexture(&font_texture);

    PzStatus result = PzRegisterConsoleHost();
    printf("PzRegisterConsoleHost: %i\r\n", result);

    LLInitialize(&windows);

    for (;;) {
        PzMessage message;

        if (PzReceiveThreadMessage(&message) == STATUS_SUCCESS) {
            if (message.Type == CONSOLE_HOST_ALLOC_CONSOLE) {
                auto *window = (ConsoleWindow*)PzAllocateHeapMemory(nullptr, sizeof(ConsoleWindow), 0);

                window->OwnerProcessId = message.Params[0];
                window->StdInWrite = message.Params[1];
                window->StdOutRead = message.Params[2];

                PzHandle thread_handle;
                PzCreateThread(&thread_handle, CPROC_HANDLE, ConWindowThread, window, 4096, 1);
            }
        }
    }

    return STATUS_SUCCESS;
}