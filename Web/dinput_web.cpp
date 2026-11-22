// See dinput_web.h.
#include "dinput_web.h"

const DIDATAFORMAT_ c_dfDIMouse = {0};
const GUID_DI_ GUID_SysMouse = {0};

static int g_x, g_y, g_wheel;
static bool g_moved; // page pointer moved since the last poll
void hb_game_cursor(short *x, short *y); // WebMain.cpp
static BYTE g_buttons[4], g_pressed[4]; // g_pressed: went down since the last poll

void hb_mouse_move(int x, int y) {
    if (x != g_x || y != g_y) g_moved = true;
    g_x = x;
    g_y = y;
}
void hb_mouse_button(int b, bool down) {
    if (b < 0 || b >= 4) return;
    g_buttons[b] = down ? 0x80 : 0;
    if (down) g_pressed[b] = 0x80;
}
void hb_mouse_wheel(int d) { g_wheel += d; }

HRESULT DirectInputCreate(HINSTANCE, DWORD, IDirectInput **out, void *) {
    *out = new IDirectInput;
    return DI_OK;
}

HRESULT IDirectInputDevice::GetDeviceState(DWORD, LPVOID out) {
    DIMOUSESTATE *s = (DIMOUSESTATE *)out;
    // The game keeps its own cursor (DXC_dinput::m_sX/m_sY, which it also moves itself, e.g. the main menu
    // centres it). When the pointer moved, report the delta that lands it on the pointer; otherwise report no
    // motion so the game's own placement stays, as with a still DirectInput mouse.
    short cx, cy;
    hb_game_cursor(&cx, &cy);
    s->lX = g_moved ? g_x - cx : 0;
    s->lY = g_moved ? g_y - cy : 0;
    g_moved = false;
    s->lZ = g_wheel;
    g_wheel = 0;
    // a click shorter than a frame still shows as pressed once, as a held DirectInput button would
    for (int i = 0; i < 4; i++) { s->rgbButtons[i] = g_buttons[i] | g_pressed[i]; g_pressed[i] = 0; }
    return DI_OK;
}
