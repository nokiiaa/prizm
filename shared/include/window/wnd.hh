#pragma once

#include <obj/window.hh>
#include <obj/thread.hh>

#define WINDOW_SERVER_CREATE_WINDOW  1
#define WINDOW_SERVER_DESTROY_WINDOW 2

extern PzSpinlock WndServerRegSpinlock;
extern PzThreadObject *WndServerThread;

PzStatus WndCreateWindow(
    PzHandle *handle, PzHandle parent, const PzString *title,
    int x, int y, int z,
    u32 width, u32 height,
    int style);
PzStatus WndGetWindowTitle(PzHandle window, char *out_title, u32 *max_size);
PzStatus WndSetWindowTitle(PzHandle window, const PzString *out_title);
PzStatus WndGetWindowBuffer(PzHandle window, int index, GfxHandle *handle);
PzStatus WndSetWindowBuffer(PzHandle window, int index, GfxHandle handle);
PzStatus WndSwapBuffers(PzHandle window);
PzStatus WndGetWindowParameter(PzHandle window, int index, uptr *value);
PzStatus WndSetWindowParameter(PzHandle window, int index, uptr value);
PzStatus WndEnumerateChildWindows(PzHandle window, u32 *max_count, PzHandle *handles, bool recursive);
PzStatus WndRegisterWindowServer();
PzStatus WndUnregisterWindowServer();