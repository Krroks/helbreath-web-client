# Renders "Trebuchet MS" 16 px non-antialiased with Windows GDI (the same call as DXC_ddraw.cpp:168)
# and saves every cp1252 glyph 32..255 so the web build draws text pixel-identical.
# usage (on Windows): python make_font_atlas.py OUT.hbf
#
# OUT format, little endian:
#   "HBF1", u8 cellHeight, u8 ascent
#   224 x [u8 advance, u8 w, u8 h, i8 ox, i8 oy]  then w*h bytes (1 = ink), glyph drawn at (x+ox, y+oy)
import ctypes, struct, sys
from ctypes import wintypes as W

gdi, user = ctypes.windll.gdi32, ctypes.windll.user32
gdi.CreateFontA.restype = W.HFONT
gdi.CreateCompatibleDC.restype = W.HDC
gdi.CreateDIBSection.restype = W.HBITMAP
gdi.SelectObject.restype = W.HGDIOBJ
gdi.SelectObject.argtypes = [W.HDC, W.HGDIOBJ]
gdi.TextOutA.argtypes = [W.HDC, ctypes.c_int, ctypes.c_int, ctypes.c_char_p, ctypes.c_int]
gdi.GetTextExtentPoint32A.argtypes = [W.HDC, ctypes.c_char_p, ctypes.c_int, ctypes.POINTER(W.SIZE)]

class BITMAPINFOHEADER(ctypes.Structure):
    _fields_ = [("biSize", W.DWORD), ("biWidth", W.LONG), ("biHeight", W.LONG), ("biPlanes", W.WORD),
                ("biBitCount", W.WORD), ("biCompression", W.DWORD), ("biSizeImage", W.DWORD),
                ("biXPelsPerMeter", W.LONG), ("biYPelsPerMeter", W.LONG), ("biClrUsed", W.DWORD), ("biClrImportant", W.DWORD)]

class TEXTMETRICA(ctypes.Structure):
    _fields_ = [(n, W.LONG) for n in ("tmHeight", "tmAscent", "tmDescent", "tmInternalLeading", "tmExternalLeading",
                "tmAveCharWidth", "tmMaxCharWidth", "tmWeight", "tmOverhang", "tmDigitizedAspectX", "tmDigitizedAspectY")] + \
               [(n, ctypes.c_char) for n in ("tmFirstChar", "tmLastChar", "tmDefaultChar", "tmBreakChar")] + \
               [(n, ctypes.c_ubyte) for n in ("tmItalic", "tmUnderlined", "tmStruckOut", "tmPitchAndFamily", "tmCharSet")]

CW, CH, PAD = 64, 32, 16  # scratch cell, glyph drawn at (PAD, PAD/2) to catch overhangs

def main(out):
    dc = gdi.CreateCompatibleDC(None)
    bih = BITMAPINFOHEADER(ctypes.sizeof(BITMAPINFOHEADER), CW, -CH, 1, 32, 0, 0, 0, 0, 0, 0)
    bits = ctypes.c_void_p()
    bmp = gdi.CreateDIBSection(dc, ctypes.byref(bih), 0, ctypes.byref(bits), None, 0)
    gdi.SelectObject(dc, bmp)
    # FW_NORMAL, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY, VARIABLE_PITCH
    font = gdi.CreateFontA(16, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 3, 2, b"Trebuchet MS")
    gdi.SelectObject(dc, font)
    gdi.SetBkMode(dc, 1)  # TRANSPARENT
    gdi.SetTextColor(dc, 0xFFFFFF)
    tm = TEXTMETRICA()
    gdi.GetTextMetricsA(dc, ctypes.byref(tm))
    buf = (ctypes.c_uint32 * (CW * CH)).from_address(bits.value)
    out_b = bytearray(b"HBF1" + bytes([tm.tmHeight, tm.tmAscent]))
    oy0 = PAD // 2
    for code in range(32, 256):
        ctypes.memset(bits, 0, CW * CH * 4)
        ch = bytes([code])
        gdi.TextOutA(dc, PAD, oy0, ch, 1)
        gdi.GdiFlush()
        sz = W.SIZE()
        gdi.GetTextExtentPoint32A(dc, ch, 1, ctypes.byref(sz))
        ink = [(x, y) for y in range(CH) for x in range(CW) if buf[y * CW + x] & 0xFFFFFF]
        if ink:
            x0, x1 = min(p[0] for p in ink), max(p[0] for p in ink)
            y0, y1 = min(p[1] for p in ink), max(p[1] for p in ink)
        else:
            x0 = y0 = PAD; x1 = x0 - 1; y1 = y0 - 1
        w, h = x1 - x0 + 1, y1 - y0 + 1
        out_b += struct.pack("<BBBbb", sz.cx, w, h, x0 - PAD, y0 - oy0)
        out_b += bytes(1 if buf[y * CW + x] & 0xFFFFFF else 0 for y in range(y0, y1 + 1) for x in range(x0, x1 + 1))
    open(out, "wb").write(out_b)
    print(f"height {tm.tmHeight} ascent {tm.tmAscent}, {len(out_b)} bytes -> {out}")

if __name__ == "__main__":
    main(sys.argv[1])
