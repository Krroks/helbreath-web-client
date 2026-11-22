// DirectInput mouse for the Emscripten build: the page feeds absolute canvas positions, the device hands
// DXC_dinput.cpp the deltas it expects, so that file runs unchanged.
#pragma once
#ifndef HB_WEB_DINPUT_H
#define HB_WEB_DINPUT_H
#include "include/windows.h"

#define DIRECTINPUT_VERSION 0x0700
#define DI_OK 0
#define DISCL_EXCLUSIVE 0x1
#define DISCL_NONEXCLUSIVE 0x2
#define DISCL_FOREGROUND 0x4
#define DISCL_BACKGROUND 0x8

typedef struct _DIMOUSESTATE { LONG lX, lY, lZ; BYTE rgbButtons[4]; } DIMOUSESTATE;
struct DIDATAFORMAT_ { int unused; };
extern const DIDATAFORMAT_ c_dfDIMouse;
struct GUID_DI_ { int unused; };
extern const GUID_DI_ GUID_SysMouse;

struct IDirectInputDevice {
    HRESULT SetDataFormat(const DIDATAFORMAT_ *) { return DI_OK; }
    HRESULT SetCooperativeLevel(HWND, DWORD) { return DI_OK; }
    HRESULT Acquire() { return DI_OK; }
    HRESULT Unacquire() { return DI_OK; }
    HRESULT GetDeviceState(DWORD size, LPVOID out);
    ULONG Release() { delete this; return 0; }
};
struct IDirectInput {
    HRESULT CreateDevice(const GUID_DI_ &, IDirectInputDevice **out, void *) { *out = new IDirectInputDevice; return DI_OK; }
    ULONG Release() { delete this; return 0; }
};
HRESULT DirectInputCreate(HINSTANCE, DWORD, IDirectInput **out, void *);

// Called by the page's mouse events (WebMain.cpp).
void hb_mouse_move(int x, int y);
void hb_mouse_button(int button, bool down);
void hb_mouse_wheel(int delta);

#endif
