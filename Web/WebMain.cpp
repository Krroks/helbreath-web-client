// Browser entry point: replaces UI/Wmain.cpp in the Emscripten build.
// WinMain + EventLoop become main() + a requestAnimationFrame loop; WndProc's message routing is kept
// (keys, text, sockets, the 1 s timer) so CGame sees the same calls in the same order.
#include <windows.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "../Net/XSocket.h"
#include "../UI/Winmain.h"
#include "../Game.h"
#include "../GlobalDef.h"
#include "dinput_web.h"

int G_iAddTable31[64][510], G_iAddTable63[64][510];
int G_iAddTransTable31[510][64], G_iAddTransTable63[510][64];
long G_lTransG100[64][64], G_lTransRB100[64][64];
long G_lTransG70[64][64], G_lTransRB70[64][64];
long G_lTransG50[64][64], G_lTransRB50[64][64];
long G_lTransG25[64][64], G_lTransRB25[64][64];
long G_lTransG2[64][64], G_lTransRB2[64][64];

char szAppClass[32];
HWND G_hWnd = (HWND)1;
HWND G_hEditWnd = NULL;
HINSTANCE G_hInstance = (HINSTANCE)1;
MMRESULT G_mmTimer;
char G_cSpriteAlphaDegree;
class CGame *G_pGame;
class XSocket *G_pCalcSocket = NULL;
BOOL G_bIsCalcSocketConnected = TRUE;
DWORD G_dwCalcSocketTime = NULL, G_dwCalcSocketSendTime = NULL;
char G_cCmdLine[256], G_cCmdLineTokenA[120], G_cCmdLineTokenA_Lowercase[120], G_cCmdLineTokenB[120], G_cCmdLineTokenC[120], G_cCmdLineTokenD[120], G_cCmdLineTokenE[120];

#define WM_USER_TIMERSIGNAL WM_USER + 500

void hb_key_state(int vk, bool down); // win32_web.cpp

static DWORD g_timerPeriod = 0, g_timerNext = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (G_pGame->GetText(hWnd, message, wParam, lParam)) return 0;
    switch (message) {
    case WM_USER_TIMERSIGNAL: G_pGame->OnTimer(); break;
    case WM_KEYDOWN: G_pGame->OnKeyDown(wParam); break;
    case WM_KEYUP: G_pGame->OnKeyUp(wParam); break;
    case WM_SYSKEYDOWN: G_pGame->OnSysKeyDown(wParam); break;
    case WM_SYSKEYUP: G_pGame->OnSysKeyUp(wParam); break;
    case WM_USER_GAMESOCKETEVENT: G_pGame->OnGameSocketEvent(wParam, lParam); break;
    case WM_USER_LOGSOCKETEVENT: G_pGame->OnLogSocketEvent(wParam, lParam); break;
    }
    return 0;
}

void hb_game_cursor(short *x, short *y) { *x = G_pGame->m_DInput.m_sX; *y = G_pGame->m_DInput.m_sY; }

void hb_socket_event(unsigned int uiMsg, SOCKET sock, LPARAM lParam) { WndProc(G_hWnd, uiMsg, (WPARAM)sock, lParam); }

// ---- page input -------------------------------------------------------------
static int canvas_x(const EmscriptenMouseEvent *e, int *y) {
    double w, h;
    emscripten_get_element_css_size("#canvas", &w, &h);
    int x = (int)(e->targetX * SCREENX / (w > 0 ? w : SCREENX));
    *y = (int)(e->targetY * SCREENY / (h > 0 ? h : SCREENY));
    if (*y < 0) *y = 0;
    if (*y > SCREENY - 1) *y = SCREENY - 1;
    return x < 0 ? 0 : x > SCREENX - 1 ? SCREENX - 1 : x;
}
// The browser keeps some Ctrl combos (Ctrl+1..9 switch tabs) with their key-ups, which left m_bCtrlPressed stuck:
// every input event carries the real modifier state, so follow it.
static void sync_mods(bool ctrl, bool shift) {
    G_pGame->m_bCtrlPressed = ctrl;
    G_pGame->m_bShiftPressed = shift;
    hb_key_state(VK_CONTROL, ctrl);
    hb_key_state(VK_SHIFT, shift);
}
static EM_BOOL on_mouse(int type, const EmscriptenMouseEvent *e, void *) {
    sync_mods(e->ctrlKey, e->shiftKey);
    int y, x = canvas_x(e, &y);
    hb_mouse_move(x, y);
    if (type == EMSCRIPTEN_EVENT_MOUSEDOWN || type == EMSCRIPTEN_EVENT_MOUSEUP) {
        int b = e->button == 0 ? 0 : e->button == 2 ? 1 : e->button == 1 ? 2 : -1;
        hb_mouse_button(b, type == EMSCRIPTEN_EVENT_MOUSEDOWN);
    }
    return type != EMSCRIPTEN_EVENT_MOUSEMOVE;
}
static EM_BOOL on_wheel(int, const EmscriptenWheelEvent *e, void *) {
    hb_mouse_wheel(e->deltaY < 0 ? 120 : e->deltaY > 0 ? -120 : 0);
    return EM_TRUE;
}

// Browser keyCode values are the Win32 virtual-key codes for every key the game reads.
static EM_BOOL on_key(int type, const EmscriptenKeyboardEvent *e, void *) {
    int vk = (int)e->keyCode;
    bool down = type == EMSCRIPTEN_EVENT_KEYDOWN;
    sync_mods(e->ctrlKey, e->shiftKey);
    // In the world and not typing, 1..9 and 0 act as F1..F9 and F10 (browsers own F5, F11...), and Shift+digit
    // opens the magic book at that circle, as Ctrl+digit does (the browser keeps Ctrl+digit). Chat gets digits.
    if (vk >= '0' && vk <= '9' && !e->altKey && G_pGame->m_cGameMode == GAMEMODE_ONMAINGAME && !G_pGame->m_bInputStatus) {
        if (e->ctrlKey || e->shiftKey) {
            if (down) {
                G_pGame->m_bCtrlPressed = TRUE;
                WndProc(G_hWnd, WM_KEYDOWN, vk, 0);
                G_pGame->m_bCtrlPressed = e->ctrlKey;
            }
            return EM_TRUE;
        }
        vk = vk == '0' ? VK_F10 : VK_F1 + (vk - '1');
        hb_key_state(vk, down);
        WndProc(G_hWnd, down ? (vk == VK_F10 ? WM_SYSKEYDOWN : WM_KEYDOWN) : (vk == VK_F10 ? WM_SYSKEYUP : WM_KEYUP), vk, 0);
        return EM_TRUE;
    }
    hb_key_state(vk, down);
    bool sys = e->altKey || vk == VK_MENU || vk == VK_F10;
    WndProc(G_hWnd, down ? (sys ? WM_SYSKEYDOWN : WM_KEYDOWN) : (sys ? WM_SYSKEYUP : WM_KEYUP), vk, 0);
    if (down && !e->ctrlKey && !e->altKey) {
        // TranslateMessage: printable keys and Backspace/Tab/Enter/Esc become WM_CHAR (cp1252)
        int ch = 0;
        if (vk == VK_BACK || vk == VK_TAB || vk == VK_RETURN || vk == VK_ESCAPE) ch = vk;
        else if (e->key[0] && !e->key[1]) ch = (unsigned char)e->key[0];
        else if ((unsigned char)e->key[0] == 0xC2 || (unsigned char)e->key[0] == 0xC3) // 2-byte UTF-8 in Latin-1
            ch = (((unsigned char)e->key[0] & 0x1F) << 6) | ((unsigned char)e->key[1] & 0x3F);
        if (ch) WndProc(G_hWnd, WM_CHAR, ch, 0);
    }
    return EM_TRUE; // keep F-keys, Tab, Backspace away from the browser
}

// ---- frame loop -----------------------------------------------------------------
EM_JS(void, hbjs_present, (const WORD *px, int pitch, int W, int H), { // EM_JS bodies are stringified: no macros in here
    const c = Module.canvas, ctx = Module.hbCtx || (Module.hbCtx = c.getContext('2d'));
    const img = Module.hbImg || (Module.hbImg = ctx.createImageData(W, H));
    const d = img.data;
    for (let y = 0, o = 0; y < H; y++) {
        let p = (px >> 1) + y * pitch;
        for (let x = 0; x < W; x++, o += 4) {
            const v = HEAPU16[p++];
            const r = v >> 11, g = (v >> 5) & 63, b = v & 31;
            d[o] = (r << 3) | (r >> 2); d[o + 1] = (g << 2) | (g >> 4); d[o + 2] = (b << 3) | (b >> 2); d[o + 3] = 255;
        }
    }
    ctx.putImageData(img, 0, 0);
});

static void frame() {
    static int lastMode = -1;
    if (G_pGame->m_cGameMode != lastMode) {
        lastMode = G_pGame->m_cGameMode;
        printf("[hb] game mode %d\n", lastMode); // console trace while the port is young
    }
    DWORD now = timeGetTime();
    if (g_timerPeriod && (int)(now - g_timerNext) >= 0) {
        g_timerNext = now + g_timerPeriod;
        WndProc(G_hWnd, WM_USER_TIMERSIGNAL, 0, 0);
    }
    if (G_pGame->m_bIsProgramActive) G_pGame->UpdateScreen();
    else if (G_pGame->m_cGameMode == GAMEMODE_ONLOADING) G_pGame->UpdateScreen_OnLoading(FALSE);
    if (hb_take_present_flag())
        if (IDirectDrawSurface7 *s = hb_presented_surface()) hbjs_present(s->px, s->pitch, SCREENX, SCREENY);
}

MMRESULT _StartTimer(DWORD dwTime) {
    g_timerPeriod = dwTime;
    g_timerNext = timeGetTime() + dwTime;
    return 1;
}
void _StopTimer(MMRESULT) { g_timerPeriod = 0; }
void CALLBACK _TimerFunc(UINT, UINT, DWORD, DWORD, DWORD) { WndProc(G_hWnd, WM_USER_TIMERSIGNAL, 0, 0); }

void OnDestroy() {
    G_pGame->m_bIsProgramActive = FALSE;
    _StopTimer(G_mmTimer);
    G_pGame->Quit();
}

void Initialize(char *pCmdLine) {
    if (G_pGame->bInit(G_hWnd, G_hInstance, pCmdLine) == FALSE) {
        fprintf(stderr, "CGame::bInit failed\n");
        return;
    }
    G_mmTimer = _StartTimer(1000);
    // same tables as UI/Wmain.cpp
    for (int iX = 0; iX < 64; iX++)
        for (int iY = 0; iY < 510; iY++) {
            int iSum = iX + (iY - 255);
            if (iSum <= 0) iSum = 1;
            if (iSum >= 31) iSum = 31;
            G_iAddTable31[iX][iY] = iSum;
            iSum = iX + (iY - 255);
            if (iSum <= 0) iSum = 1;
            if (iSum >= 63) iSum = 63;
            G_iAddTable63[iX][iY] = iSum;
            if ((iY - 255) < iX) G_iAddTransTable31[iY][iX] = iX;
            else if ((iY - 255) > 31) G_iAddTransTable31[iY][iX] = 31;
            else G_iAddTransTable31[iY][iX] = iY - 255;
            if ((iY - 255) < iX) G_iAddTransTable63[iY][iX] = iX;
            else if ((iY - 255) > 63) G_iAddTransTable63[iY][iX] = 63;
            else G_iAddTransTable63[iY][iX] = iY - 255;
        }
}

LONG GetRegKey(HKEY, LPCTSTR, LPTSTR) { return 2; }
EM_JS(void, hbjs_open_url, (const char *u), { window.open(UTF8ToString(u), '_blank'); });
void GoHomepage() { hbjs_open_url(MSG_HOMEPAGE); }

// Hot reload of sprite paks: the dev server (tools/serve.mjs) sends "sprites/<name>.pak" on /events when one changes.
void hb_invalidate_file(const char *winPath); // win32_web.cpp
extern "C" EMSCRIPTEN_KEEPALIVE int hb_reload_pak(const char *name) {
    char path[64];
    snprintf(path, sizeof path, "sprites\\%s.pak", name);
    hb_invalidate_file(path);
    int n = G_pGame->iReloadPak((char *)name);
    printf("[hb] reload %s.pak: %d sprites\n", name, n);
    return n;
}
EM_JS(void, hbjs_watch_paks, (), {
    if (typeof EventSource === 'undefined') return;
    new EventSource('/events').onmessage = (e) => {
        const f = e.data.toLowerCase();
        if (!f.startsWith('sprites/') || !f.endsWith('.pak')) return;
        _hb_reload_pak(stringToNewUTF8(e.data.slice(8, -4))); // shortcut: a few bytes leak per reload, dev only
    };
});

// requestAnimationFrame stops in hidden tabs; the original loop never stopped (sockets, timers), so fall back
// to a timer there.
static EM_BOOL on_visibility(int, const EmscriptenVisibilityChangeEvent *e, void *) {
    if (e->hidden) emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT, 16);
    else emscripten_set_main_loop_timing(EM_TIMING_RAF, 1);
    return EM_FALSE;
}

int main() {
    srand((unsigned)time(NULL));
    G_pGame = new class CGame;
    Initialize((char *)"");
    G_pGame->m_bIsProgramActive = TRUE; // the page is the window; WM_ACTIVATEAPP never comes
    emscripten_set_mousemove_callback("#canvas", NULL, EM_TRUE, on_mouse);
    emscripten_set_mousedown_callback("#canvas", NULL, EM_TRUE, on_mouse);
    emscripten_set_mouseup_callback("#canvas", NULL, EM_TRUE, on_mouse);
    emscripten_set_wheel_callback("#canvas", NULL, EM_TRUE, on_wheel);
    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, on_key);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_DOCUMENT, NULL, EM_TRUE, on_key);
    emscripten_set_visibilitychange_callback(NULL, EM_FALSE, on_visibility);
    hbjs_watch_paks();
    emscripten_set_main_loop(frame, 0, 0);
    EmscriptenVisibilityChangeEvent v;
    if (emscripten_get_visibility_status(&v) == EMSCRIPTEN_RESULT_SUCCESS) on_visibility(0, &v, NULL);
    return 0;
}

// Raw RGB565 frame for tests: Module._hb_frame_ptr() into HEAPU16, pitch in pixels (tools compare it with the exe).
extern "C" EMSCRIPTEN_KEEPALIVE const WORD *hb_frame_ptr() { IDirectDrawSurface7 *s = hb_presented_surface(); return s ? s->px : nullptr; }
extern "C" EMSCRIPTEN_KEEPALIVE int hb_frame_pitch() { IDirectDrawSurface7 *s = hb_presented_surface(); return s ? s->pitch : 0; }
