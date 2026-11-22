// Software DirectDraw + GDI text, see ddraw_web.h.
#include "ddraw_web.h"
#include "../GlobalDef.h"
#include <vector>

const GUID_ IID_IDirectDraw7 = {0};
static IDirectDrawSurface7 *g_primary = nullptr;
static bool g_presented = false;

IDirectDrawSurface7 *hb_presented_surface() { return g_primary; }
bool hb_take_present_flag() { bool p = g_presented; g_presented = false; return p; }

static inline WORD to565(COLORREF c) {
    // GDI on a 16-bit 5:6:5 surface keeps the top bits of each channel
    return (WORD)(((GetRValue(c) >> 3) << 11) | ((GetGValue(c) >> 2) << 5) | (GetBValue(c) >> 3));
}
static inline COLORREF from565(WORD p) {
    return RGB(((p >> 11) & 31) << 3, ((p >> 5) & 63) << 2, (p & 31) << 3);
}

IDirectDrawSurface7::IDirectDrawSurface7(int w_, int h_) : w(w_), h(h_), pitch(w_) {
    px = (WORD *)calloc((size_t)w * h, sizeof(WORD));
}
IDirectDrawSurface7::~IDirectDrawSurface7() {
    if (g_primary == this) g_primary = nullptr;
    delete attached;
    free(px);
}

HRESULT IDirectDraw7::CreateSurface(DDSURFACEDESC2 *d, IDirectDrawSurface7 **out, void *) {
    if (d->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
        auto *s = new IDirectDrawSurface7(SCREENX, SCREENY);
        s->primary = true;
        if (d->ddsCaps.dwCaps & DDSCAPS_FLIP) s->attached = new IDirectDrawSurface7(SCREENX, SCREENY);
        g_primary = s;
        *out = s;
        return DD_OK;
    }
    if (!d->dwWidth || !d->dwHeight) return DDERR_INVALIDPARAMS;
    *out = new IDirectDrawSurface7((int)d->dwWidth, (int)d->dwHeight);
    return DD_OK;
}

HRESULT DirectDrawCreateEx(void *, void **out, const GUID_ &, void *) {
    *out = new IDirectDraw7;
    return DD_OK;
}

static void fill_desc(const IDirectDrawSurface7 *s, DDSURFACEDESC2 *d) {
    d->dwWidth = s->w;
    d->dwHeight = s->h;
    d->lPitch = s->pitch * 2;
    d->lpSurface = s->px;
    d->ddpfPixelFormat.dwRGBBitCount = 16;
    d->ddpfPixelFormat.dwRBitMask = 0xF800;
    d->ddpfPixelFormat.dwGBitMask = 0x07E0;
    d->ddpfPixelFormat.dwBBitMask = 0x001F;
}

HRESULT IDirectDrawSurface7::Lock(LPRECT, DDSURFACEDESC2 *d, DWORD, HANDLE) {
    fill_desc(this, d);
    return DD_OK;
}

HRESULT IDirectDrawSurface7::GetSurfaceDesc(DDSURFACEDESC2 *d) {
    fill_desc(this, d);
    return DD_OK;
}

HRESULT IDirectDrawSurface7::GetAttachedSurface(DDSCAPS2 *, IDirectDrawSurface7 **out) {
    if (!attached) return E_FAIL;
    *out = attached;
    return DD_OK;
}

HRESULT IDirectDrawSurface7::SetColorKey(DWORD, DDCOLORKEY *ck) {
    hasKey = ck != nullptr;
    if (ck) key = (WORD)ck->dwColorSpaceLowValue;
    return DD_OK;
}

HRESULT IDirectDrawSurface7::BltFast(DWORD x, DWORD y, IDirectDrawSurface7 *src, LPRECT r, DWORD flags) {
    RECT whole = {0, 0, src->w, src->h};
    if (!r) r = &whole;
    int sw = r->right - r->left, sh = r->bottom - r->top;
    // DirectDraw does not clip BltFast: out-of-range rects fail and draw nothing
    if (sw <= 0 || sh <= 0 || r->left < 0 || r->top < 0 || r->right > src->w || r->bottom > src->h ||
        (int)x < 0 || (int)y < 0 || (int)x + sw > w || (int)y + sh > h)
        return DDERR_INVALIDRECT;
    bool keyed = (flags & DDBLTFAST_SRCCOLORKEY) && src->hasKey;
    for (int j = 0; j < sh; j++) {
        const WORD *s = src->px + (r->top + j) * src->pitch + r->left;
        WORD *d = px + (y + j) * pitch + x;
        if (!keyed) memmove(d, s, sw * 2);
        else
            for (int i = 0; i < sw; i++)
                if (s[i] != src->key) d[i] = s[i];
    }
    if (primary) g_presented = true;
    return DD_OK;
}

HRESULT IDirectDrawSurface7::Blt(LPRECT dr, IDirectDrawSurface7 *src, LPRECT sr, DWORD flags, DDBLTFX *) {
    if (!src) return DDERR_INVALIDPARAMS;
    RECT dw = {0, 0, w, h}, sw = {0, 0, src->w, src->h};
    if (!dr) dr = &dw;
    if (!sr) sr = &sw;
    int dW = dr->right - dr->left, dH = dr->bottom - dr->top, sW = sr->right - sr->left, sH = sr->bottom - sr->top;
    if (dW <= 0 || dH <= 0 || sW <= 0 || sH <= 0) return DDERR_INVALIDRECT;
    bool keyed = (flags & DDBLT_KEYSRC) && src->hasKey;
    for (int j = 0; j < dH; j++) {
        int dy = dr->top + j;
        if (dy < 0 || dy >= h) continue;
        int sy = sr->top + j * sH / dH;
        for (int i = 0; i < dW; i++) {
            int dx = dr->left + i;
            if (dx < 0 || dx >= w) continue;
            WORD p = src->px[sy * src->pitch + sr->left + i * sW / dW];
            if (!keyed || p != src->key) px[dy * pitch + dx] = p;
        }
    }
    if (primary) g_presented = true;
    return DD_OK;
}

HRESULT IDirectDrawSurface7::Flip(IDirectDrawSurface7 *, DWORD) {
    if (attached) memcpy(px, attached->px, (size_t)w * h * 2);
    g_presented = true;
    return DD_OK;
}

HRESULT IDirectDrawSurface7::GetDC(HDC *out) {
    *out = new HDC__{this, nullptr, RGB(0, 0, 0), RGB(255, 255, 255), OPAQUE};
    return DD_OK;
}

HRESULT IDirectDrawSurface7::ReleaseDC(HDC dc) {
    delete dc;
    return DD_OK;
}

// ---- GDI ------------------------------------------------------------------
struct Glyph { int adv, w, h, ox, oy; std::vector<BYTE> ink; };
static std::vector<Glyph> g_glyphs; // index = byte - 32
static int g_cellH = 16;

static bool load_font() {
    if (!g_glyphs.empty()) return true;
    FILE *f = ::hb_real_fopen("/web/trebuchet16.hbf", "rb"); // produced by Web/tools/make_font_atlas.py
    if (!f) { fprintf(stderr, "font atlas missing\n"); return false; }
    unsigned char hdr[6];
    if (fread(hdr, 1, 6, f) != 6 || memcmp(hdr, "HBF1", 4)) { fclose(f); return false; }
    g_cellH = hdr[4];
    for (int c = 32; c < 256; c++) {
        signed char g[5];
        if (fread(g, 1, 5, f) != 5) break;
        Glyph gl{(BYTE)g[0], (BYTE)g[1], (BYTE)g[2], g[3], g[4], {}};
        gl.ink.resize(gl.w * gl.h);
        if (!gl.ink.empty() && fread(gl.ink.data(), 1, gl.ink.size(), f) != gl.ink.size()) break;
        g_glyphs.push_back(std::move(gl));
    }
    fclose(f);
    return g_glyphs.size() == 224;
}

static const Glyph *glyph(unsigned char c) {
    if (!load_font() || c < 32) return nullptr;
    return &g_glyphs[c - 32];
}

static int text_width(const char *s, int n) {
    int x = 0;
    for (int i = 0; i < n; i++)
        if (const Glyph *g = glyph((unsigned char)s[i])) x += g->adv;
    return x;
}

HFONT CreateFont(int h, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, LPCSTR face) {
    if (h != 16 || !face || strcmp(face, "Trebuchet MS"))
        fprintf(stderr, "CreateFont(%d, %s): only Trebuchet MS 16 is baked, using it\n", h, face ? face : "");
    return new HFONT__{h};
}
HGDIOBJ SelectObject(HDC dc, void *obj) {
    if (!dc) return NULL;
    HFONT old = dc->font;
    dc->font = (HFONT)obj;
    return (HGDIOBJ)old;
}
BOOL DeleteObject(void *obj) { delete (HFONT__ *)obj; return TRUE; }
COLORREF SetTextColor(HDC dc, COLORREF c) { COLORREF o = dc->textColor; dc->textColor = c; return o; }
COLORREF SetBkColor(HDC dc, COLORREF c) { COLORREF o = dc->bkColor; dc->bkColor = c; return o; }
int SetBkMode(HDC dc, int m) { int o = dc->bkMode; dc->bkMode = m; return o; }

BOOL TextOut(HDC dc, int x, int y, LPCSTR s, int n) {
    if (!dc || !dc->surf) return FALSE;
    IDirectDrawSurface7 *sf = dc->surf;
    WORD fg = to565(dc->textColor), bg = to565(dc->bkColor);
    for (int k = 0; k < n; k++) {
        const Glyph *g = glyph((unsigned char)s[k]);
        if (!g) continue;
        if (dc->bkMode == OPAQUE)
            for (int j = 0; j < g_cellH; j++)
                for (int i = 0; i < g->adv; i++) {
                    int px = x + i, py = y + j;
                    if (px >= 0 && py >= 0 && px < sf->w && py < sf->h) sf->px[py * sf->pitch + px] = bg;
                }
        for (int j = 0; j < g->h; j++)
            for (int i = 0; i < g->w; i++) {
                if (!g->ink[j * g->w + i]) continue;
                int px = x + g->ox + i, py = y + g->oy + j;
                if (px >= 0 && py >= 0 && px < sf->w && py < sf->h) sf->px[py * sf->pitch + px] = fg;
            }
        x += g->adv;
    }
    return TRUE;
}

BOOL GetTextExtentPoint32(HDC, LPCSTR s, int n, LPSIZE sz) {
    sz->cx = text_width(s, n);
    sz->cy = g_cellH;
    return TRUE;
}

// GDI DrawText subset: DT_CENTER / DT_RIGHT, DT_WORDBREAK, DT_SINGLELINE, DT_CALCRECT.
int DrawText(HDC dc, LPCSTR s, int n, LPRECT r, UINT fmt) {
    if (n < 0) n = (int)strlen(s);
    int maxW = r->right - r->left, y = r->top, widest = 0;
    int i = 0;
    while (i < n) {
        int lineEnd = i, lastFit = i;
        if (fmt & DT_SINGLELINE || !(fmt & DT_WORDBREAK)) {
            while (lineEnd < n && s[lineEnd] != '\n') lineEnd++;
            lastFit = lineEnd;
        } else {
            // take words while they fit; a single word wider than the box still gets its own line
            int j = i;
            while (j < n && s[j] != '\n') {
                int k = j;
                while (k < n && s[k] == ' ') k++;
                while (k < n && s[k] != ' ' && s[k] != '\n') k++;
                if (text_width(s + i, k - i) > maxW && j > i) break;
                j = k;
            }
            lastFit = j;
            lineEnd = j;
        }
        int len = lastFit - i;
        while (len > 0 && s[i + len - 1] == ' ') len--;
        int lw = text_width(s + i, len);
        if (lw > widest) widest = lw;
        if (!(fmt & DT_CALCRECT)) {
            int x = r->left;
            if (fmt & DT_CENTER) x = r->left + (maxW - lw) / 2;
            else if (fmt & DT_RIGHT) x = r->right - lw;
            TextOut(dc, x, y, s + i, len);
        }
        y += g_cellH;
        i = lineEnd;
        if (i < n && s[i] == '\n') i++;
        while (i < n && s[i] == ' ' && (fmt & DT_WORDBREAK)) i++;
    }
    if (fmt & DT_CALCRECT) { r->right = r->left + widest; r->bottom = y; }
    return y - r->top;
}

COLORREF GetPixel(HDC dc, int x, int y) {
    IDirectDrawSurface7 *s = dc ? dc->surf : nullptr;
    if (!s || x < 0 || y < 0 || x >= s->w || y >= s->h) return CLR_INVALID;
    return from565(s->px[y * s->pitch + x]);
}

COLORREF SetPixel(HDC dc, int x, int y, COLORREF c) {
    IDirectDrawSurface7 *s = dc ? dc->surf : nullptr;
    if (!s || x < 0 || y < 0 || x >= s->w || y >= s->h) return CLR_INVALID;
    s->px[y * s->pitch + x] = to565(c);
    return c;
}

int SetDIBitsToDevice(HDC dc, int xDest, int yDest, DWORD w, DWORD h, int xSrc, int ySrc, UINT, UINT,
                      const void *bits, const BITMAPINFO *bmi, UINT) {
    IDirectDrawSurface7 *s = dc ? dc->surf : nullptr;
    if (!s || !bits) return 0;
    const BITMAPINFOHEADER &bh = bmi->bmiHeader;
    int bpp = bh.biBitCount, bw = bh.biWidth, bhH = bh.biHeight < 0 ? -bh.biHeight : bh.biHeight;
    bool bottomUp = bh.biHeight > 0;
    int stride = ((bw * bpp + 31) / 32) * 4;
    const BYTE *src = (const BYTE *)bits;
    const RGBQUAD *pal = bmi->bmiColors;
    for (DWORD j = 0; j < h; j++) {
        int sy = ySrc + (int)j, dy = yDest + (int)j;
        if (sy >= bhH || dy < 0 || dy >= s->h) continue;
        const BYTE *row = src + (bottomUp ? (bhH - 1 - sy) : sy) * stride;
        for (DWORD i = 0; i < w; i++) {
            int sx = xSrc + (int)i, dx = xDest + (int)i;
            if (sx >= bw || dx < 0 || dx >= s->w) continue;
            COLORREF c;
            switch (bpp) {
            case 24: c = RGB(row[sx * 3 + 2], row[sx * 3 + 1], row[sx * 3]); break;
            case 32: c = RGB(row[sx * 4 + 2], row[sx * 4 + 1], row[sx * 4]); break;
            case 8: { const RGBQUAD &q = pal[row[sx]]; c = RGB(q.rgbRed, q.rgbGreen, q.rgbBlue); break; }
            case 4: { int v = (row[sx >> 1] >> ((sx & 1) ? 0 : 4)) & 15; c = RGB(pal[v].rgbRed, pal[v].rgbGreen, pal[v].rgbBlue); break; }
            case 1: { int v = (row[sx >> 3] >> (7 - (sx & 7))) & 1; c = RGB(pal[v].rgbRed, pal[v].rgbGreen, pal[v].rgbBlue); break; }
            default: return 0;
            }
            s->px[dy * s->pitch + dx] = to565(c);
        }
    }
    return (int)h;
}
