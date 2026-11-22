// Software DirectDraw 7 + the GDI calls the client makes, for the Emscripten build.
// Surfaces are plain RGB565 buffers, so Sprite.cpp/Game.cpp pixel code runs unchanged.
#pragma once
#ifndef HB_WEB_DDRAW_H
#define HB_WEB_DDRAW_H

#ifndef HB_WEB_WINDOWS_H
#include "include/windows.h"
#endif

#define VOID void
#define S_OK 0
#define E_FAIL ((HRESULT)0x80004005L)
#define FAILED(hr) (((HRESULT)(hr)) < 0)
#define SUCCEEDED(hr) (((HRESULT)(hr)) >= 0)
#define DD_OK 0
#define DDERR_SURFACELOST ((HRESULT)0x887601C2L)
#define DDERR_WASSTILLDRAWING ((HRESULT)0x8876021CL)
#define DDERR_INVALIDRECT ((HRESULT)0x88760096L)
#define DDERR_INVALIDPARAMS E_FAIL

#define DDLOCK_WAIT 0x1
#define DDBLTFAST_NOCOLORKEY 0x0
#define DDBLTFAST_SRCCOLORKEY 0x1
#define DDBLTFAST_DESTCOLORKEY 0x2
#define DDBLTFAST_WAIT 0x10
#define DDBLT_WAIT 0x01000000
#define DDBLT_KEYSRC 0x00008000
#define DDFLIP_WAIT 0x1
#define DDSCL_NORMAL 0x8
#define DDSCL_FULLSCREEN 0x1
#define DDSCL_EXCLUSIVE 0x10
#define DDSD_CAPS 0x1
#define DDSD_HEIGHT 0x2
#define DDSD_WIDTH 0x4
#define DDSD_PITCH 0x8
#define DDSD_BACKBUFFERCOUNT 0x20
#define DDSD_PIXELFORMAT 0x1000
#define DDSCAPS_BACKBUFFER 0x4
#define DDSCAPS_COMPLEX 0x8
#define DDSCAPS_FLIP 0x10
#define DDSCAPS_OFFSCREENPLAIN 0x40
#define DDSCAPS_PRIMARYSURFACE 0x200
#define DDSCAPS_SYSTEMMEMORY 0x800
#define DDCKEY_SRCBLT 0x8
#define DDCAPS2_CANRENDERWINDOWED 0x80000
#define CLR_INVALID 0xFFFFFFFF
#define BI_BITFIELDS 3

typedef struct _DDPIXELFORMAT {
    DWORD dwSize, dwFlags, dwFourCC, dwRGBBitCount, dwRBitMask, dwGBitMask, dwBBitMask, dwRGBAlphaBitMask;
} DDPIXELFORMAT;
typedef struct _DDSCAPS2 { DWORD dwCaps, dwCaps2, dwCaps3, dwCaps4; } DDSCAPS2;
typedef struct _DDCOLORKEY { DWORD dwColorSpaceLowValue, dwColorSpaceHighValue; } DDCOLORKEY;
typedef struct _DDSURFACEDESC2 {
    DWORD dwSize, dwFlags, dwHeight, dwWidth;
    LONG lPitch;
    DWORD dwBackBufferCount, dwMipMapCount, dwAlphaBitDepth, dwReserved;
    LPVOID lpSurface;
    DDCOLORKEY ddckCKDestOverlay, ddckCKDestBlt, ddckCKSrcOverlay, ddckCKSrcBlt;
    DDPIXELFORMAT ddpfPixelFormat;
    DDSCAPS2 ddsCaps;
    DWORD dwTextureStage;
} DDSURFACEDESC2;
typedef struct _DDCAPS { DWORD dwSize, dwCaps, dwCaps2; } DDCAPS;
typedef struct _DDBLTFX { DWORD dwSize; } DDBLTFX;

struct IDirectDrawClipper {
    HRESULT SetHWnd(DWORD, HWND) { return DD_OK; }
    ULONG Release() { delete this; return 0; }
};
typedef IDirectDrawClipper *LPDIRECTDRAWCLIPPER;

struct IDirectDrawSurface7 {
    int w = 0, h = 0, pitch = 0; // pitch in pixels
    WORD *px = nullptr;
    bool hasKey = false;
    WORD key = 0;
    bool primary = false;
    IDirectDrawSurface7 *attached = nullptr; // back buffer of a flipping primary
    IDirectDrawSurface7(int w_, int h_);
    ~IDirectDrawSurface7();
    HRESULT Lock(LPRECT, DDSURFACEDESC2 *d, DWORD, HANDLE);
    HRESULT Unlock(LPRECT) { return DD_OK; }
    HRESULT BltFast(DWORD x, DWORD y, IDirectDrawSurface7 *src, LPRECT srcRect, DWORD flags);
    HRESULT Blt(LPRECT dstRect, IDirectDrawSurface7 *src, LPRECT srcRect, DWORD flags, DDBLTFX *);
    HRESULT Flip(IDirectDrawSurface7 *, DWORD);
    HRESULT GetDC(HDC *out);
    HRESULT ReleaseDC(HDC dc);
    HRESULT SetColorKey(DWORD, DDCOLORKEY *ck);
    HRESULT Restore() { return DD_OK; }
    HRESULT IsLost() { return DD_OK; }
    HRESULT GetSurfaceDesc(DDSURFACEDESC2 *d);
    HRESULT GetAttachedSurface(DDSCAPS2 *, IDirectDrawSurface7 **out);
    HRESULT SetClipper(IDirectDrawClipper *) { return DD_OK; }
    ULONG Release() { delete this; return 0; }
};
typedef IDirectDrawSurface7 *LPDIRECTDRAWSURFACE7;

struct IDirectDraw7 {
    HRESULT SetCooperativeLevel(HWND, DWORD) { return DD_OK; }
    HRESULT SetDisplayMode(DWORD, DWORD, DWORD, DWORD, DWORD) { return DD_OK; }
    HRESULT RestoreDisplayMode() { return DD_OK; }
    HRESULT CreateSurface(DDSURFACEDESC2 *d, IDirectDrawSurface7 **out, void *);
    HRESULT GetCaps(DDCAPS *c, DDCAPS *) { c->dwCaps = 0; c->dwCaps2 = DDCAPS2_CANRENDERWINDOWED; return DD_OK; }
    HRESULT CreateClipper(DWORD, IDirectDrawClipper **out, void *) { *out = new IDirectDrawClipper; return DD_OK; }
    HRESULT FlipToGDISurface() { return DD_OK; }
    ULONG Release() { delete this; return 0; }
};
typedef IDirectDraw7 *LPDIRECTDRAW7;
struct GUID_ { int unused; };
extern const GUID_ IID_IDirectDraw7;
HRESULT DirectDrawCreateEx(void *, void **out, const GUID_ &, void *);

// The surface the page shows: set by the primary surface on Flip/Blt.
IDirectDrawSurface7 *hb_presented_surface();
bool hb_take_present_flag();

// ---- GDI on surfaces ------------------------------------------------------
#define FW_NORMAL 400
#define FW_BOLD 700
#define ANSI_CHARSET 0
#define DEFAULT_CHARSET 1
#define HANGEUL_CHARSET 129
#define OUT_DEFAULT_PRECIS 0
#define CLIP_DEFAULT_PRECIS 0
#define NONANTIALIASED_QUALITY 3
#define DEFAULT_QUALITY 0
#define VARIABLE_PITCH 2
#define FIXED_PITCH 1
#define DEFAULT_PITCH 0
#define FF_DONTCARE 0
#define TRANSPARENT 1
#define OPAQUE 2
#define DT_TOP 0x0
#define DT_LEFT 0x0
#define DT_CENTER 0x1
#define DT_RIGHT 0x2
#define DT_VCENTER 0x4
#define DT_BOTTOM 0x8
#define DT_WORDBREAK 0x10
#define DT_SINGLELINE 0x20
#define DT_NOCLIP 0x100
#define DT_CALCRECT 0x400
#define DT_NOPREFIX 0x800
#define SRCCOPY 0x00CC0020

struct HFONT__ { int height; };
struct HDC__ {
    IDirectDrawSurface7 *surf;
    HFONT font;
    COLORREF textColor, bkColor;
    int bkMode;
};

HFONT CreateFont(int h, int, int, int, int weight, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCSTR face);
HGDIOBJ SelectObject(HDC dc, void *obj);
BOOL DeleteObject(void *obj);
COLORREF SetTextColor(HDC dc, COLORREF c);
COLORREF SetBkColor(HDC dc, COLORREF c);
int SetBkMode(HDC dc, int mode);
BOOL TextOut(HDC dc, int x, int y, LPCSTR s, int n);
int DrawText(HDC dc, LPCSTR s, int n, LPRECT r, UINT fmt);
BOOL GetTextExtentPoint32(HDC dc, LPCSTR s, int n, LPSIZE sz);
COLORREF GetPixel(HDC dc, int x, int y);
COLORREF SetPixel(HDC dc, int x, int y, COLORREF c);
int SetDIBitsToDevice(HDC dc, int xDest, int yDest, DWORD w, DWORD h, int xSrc, int ySrc, UINT startScan, UINT lines,
                      const void *bits, const BITMAPINFO *bmi, UINT usage);
// Only DXC_ddraw::Screenshot uses these; they fail so it reports "Screenshot error".
inline HBITMAP CreateCompatibleBitmap(HDC, int, int) { return NULL; }
inline HDC CreateCompatibleDC(HDC) { return NULL; }
inline BOOL DeleteDC(HDC) { return TRUE; }
inline BOOL BitBlt(HDC, int, int, int, int, HDC, int, int, DWORD) { return FALSE; }
inline int GetDIBits(HDC, HBITMAP, UINT, UINT, LPVOID, LPBITMAPINFO, UINT) { return 0; }
inline BOOL SetRect(LPRECT r, int l, int t, int ri, int b) { r->left = l; r->top = t; r->right = ri; r->bottom = b; return TRUE; }

#endif
