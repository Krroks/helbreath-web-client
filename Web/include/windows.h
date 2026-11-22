// Win32 surface used by the client, for the Emscripten build only.
// Only what Client/ actually calls lives here; GDI/DirectX pieces are in Web/ddraw_web.h.
#pragma once
#ifndef HB_WEB_WINDOWS_H
#define HB_WEB_WINDOWS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <errno.h>

typedef int BOOL;
typedef unsigned char BYTE, UCHAR, BOOLEAN;
typedef unsigned short WORD, USHORT;
typedef unsigned long DWORD, ULONG; // 32-bit on wasm32, as on Win32
typedef long LONG;
typedef unsigned int UINT;
typedef int INT;
typedef short SHORT;
typedef char CHAR, TCHAR;
typedef float FLOAT;
typedef char *LPSTR, *LPTSTR, *PSTR;
typedef const char *LPCSTR, *LPCTSTR;
typedef void *LPVOID, *PVOID;
typedef const void *LPCVOID;
typedef WORD *LPWORD;
typedef DWORD *LPDWORD, *PDWORD;
typedef BYTE *LPBYTE, *PBYTE;
typedef long *LPLONG;
typedef int *LPINT;
typedef long long LONGLONG; // __int64 is a clang builtin with -fms-extensions
typedef unsigned long long ULONGLONG;
typedef long LRESULT, HRESULT;
typedef unsigned int WPARAM;
typedef long LPARAM;
typedef DWORD COLORREF;
typedef UINT MMRESULT;
typedef unsigned int SOCKET;
typedef WORD ATOM;
typedef unsigned short WCHAR;

typedef struct HWND__ *HWND;
typedef struct HINSTANCE__ *HINSTANCE, *HMODULE;
typedef struct HDC__ *HDC;
typedef struct HFONT__ *HFONT;
typedef struct HGDIOBJ__ *HGDIOBJ, *HBITMAP, *HBRUSH, *HPEN;
typedef struct HKEY__ *HKEY;
typedef struct HCURSOR__ *HCURSOR, *HICON;
typedef struct HIMC__ *HIMC;
typedef void *HANDLE;
typedef HANDLE *LPHANDLE;
typedef int (*FARPROC)();

#define TRUE 1
#define FALSE 0
#define CALLBACK
#define WINAPI
#define APIENTRY
#define PASCAL
#define FAR
#define NEAR
#define far
#define near
#define __stdcall
#define __cdecl
#define __declspec(x)
#define __forceinline inline
#define _inline inline
#define __inline inline
#define IN
#define OUT
#define CONST const
#define MAX_PATH 260
#define INFINITE 0xFFFFFFFF

#if !defined(max) && !defined(NOMINMAX)
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#if !defined(min) && !defined(NOMINMAX)
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define MAKEWORD(a, b) ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b) ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l) ((WORD)((DWORD)(l) & 0xffff))
#define HIWORD(l) ((WORD)((DWORD)(l) >> 16))
#define LOBYTE(w) ((BYTE)((DWORD)(w) & 0xff))
#define HIBYTE(w) ((BYTE)((DWORD)(w) >> 8))
#define RGB(r, g, b) ((COLORREF)(((BYTE)(r) | ((WORD)((BYTE)(g)) << 8)) | (((DWORD)(BYTE)(b)) << 16)))
#define GetRValue(rgb) ((BYTE)(rgb))
#define GetGValue(rgb) ((BYTE)(((WORD)(rgb)) >> 8))
#define GetBValue(rgb) ((BYTE)((rgb) >> 16))

#define ZeroMemory(p, n) memset((p), 0, (n))
#define FillMemory(p, n, v) memset((p), (v), (n))
#define CopyMemory(d, s, n) memcpy((d), (s), (n))
#define MoveMemory(d, s, n) memmove((d), (s), (n))

typedef struct tagRECT { LONG left, top, right, bottom; } RECT, *LPRECT, *PRECT;
typedef const RECT *LPCRECT;
typedef struct tagPOINT { LONG x, y; } POINT, *LPPOINT;
typedef struct tagSIZE { LONG cx, cy; } SIZE, *LPSIZE;
typedef struct _SYSTEMTIME {
    WORD wYear, wMonth, wDayOfWeek, wDay, wHour, wMinute, wSecond, wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;
typedef struct tagMSG { HWND hwnd; UINT message; WPARAM wParam; LPARAM lParam; DWORD time; POINT pt; } MSG;

#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1, bfReserved2;
    DWORD bfOffBits;
} BITMAPFILEHEADER;
#pragma pack(pop)
typedef struct tagBITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth, biHeight;
    WORD biPlanes, biBitCount;
    DWORD biCompression, biSizeImage;
    LONG biXPelsPerMeter, biYPelsPerMeter;
    DWORD biClrUsed, biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;
typedef struct tagRGBQUAD { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD;
typedef struct tagBITMAPINFO { BITMAPINFOHEADER bmiHeader; RGBQUAD bmiColors[1]; } BITMAPINFO, *LPBITMAPINFO;
#define BI_RGB 0
#define DIB_RGB_COLORS 0

// ---- strings -------------------------------------------------------------
#define wsprintf sprintf
#define lstrcpy strcpy
#define lstrcat strcat
#define lstrlen(s) ((int)strlen(s))
#define _stricmp strcasecmp
#define stricmp strcasecmp
#define _strnicmp strncasecmp
#define strnicmp strncasecmp
#define _snprintf snprintf
#define _vsnprintf vsnprintf
inline char *_strlwr(char *s) { for (char *p = s; *p; p++) *p = (char)tolower((unsigned char)*p); return s; }
inline char *_strupr(char *s) { for (char *p = s; *p; p++) *p = (char)toupper((unsigned char)*p); return s; }
inline char *_strrev(char *s) { size_t n = strlen(s); for (size_t i = 0; i < n / 2; i++) { char t = s[i]; s[i] = s[n - 1 - i]; s[n - 1 - i] = t; } return s; }
#define strrev _strrev
#define strlwr _strlwr
#define strupr _strupr
inline char *_itoa(int v, char *buf, int radix) {
    if (radix == 10) { sprintf(buf, "%d", v); return buf; }
    char tmp[40]; int i = 0; unsigned u = (unsigned)v;
    do { int d = u % radix; tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10); u /= radix; } while (u);
    int j = 0; while (i) buf[j++] = tmp[--i]; buf[j] = 0; return buf;
}
#define itoa _itoa
inline char *_ltoa(long v, char *buf, int radix) { return _itoa((int)v, buf, radix); }

// ---- time ----------------------------------------------------------------
DWORD timeGetTime();
#define GetTickCount timeGetTime
inline void Sleep(DWORD) {} // shortcut: the browser main loop cannot block; callers only used it as a short wait
void GetLocalTime(LPSYSTEMTIME st);
typedef struct timecaps_tag { UINT wPeriodMin, wPeriodMax; } TIMECAPS;
inline MMRESULT timeGetDevCaps(TIMECAPS *c, UINT) { c->wPeriodMin = 1; c->wPeriodMax = 1000; return 0; }
inline MMRESULT timeBeginPeriod(UINT) { return 0; }
inline MMRESULT timeEndPeriod(UINT) { return 0; }

// ---- memory --------------------------------------------------------------
#define HEAP_ZERO_MEMORY 0x8
#define HEAP_NO_SERIALIZE 0x1
inline HANDLE GetProcessHeap() { return (HANDLE)1; }
inline LPVOID HeapAlloc(HANDLE, DWORD, size_t n) { return calloc(1, n); }
inline BOOL HeapFree(HANDLE, DWORD, LPVOID p) { free(p); return TRUE; }
#define GMEM_FIXED 0
#define GPTR 0x40
inline HANDLE GlobalAlloc(UINT, size_t n) { return calloc(1, n); }
inline HANDLE GlobalFree(HANDLE p) { free(p); return NULL; }

// ---- files: Win32 paths ("sprites\\x.pak") map to lowercase '/' paths in the virtual FS ----
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#define GENERIC_READ 0x80000000
#define GENERIC_WRITE 0x40000000
#define FILE_SHARE_READ 1
#define FILE_SHARE_WRITE 2
#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define TRUNCATE_EXISTING 5
#define FILE_ATTRIBUTE_NORMAL 0x80
#define FILE_BEGIN 0
#define FILE_CURRENT 1
#define FILE_END 2
#define INVALID_FILE_SIZE 0xFFFFFFFF
typedef struct _OVERLAPPED *LPOVERLAPPED;
typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;
const char *hb_path(const char *winPath); // returns a static buffer
FILE *hb_fopen(const char *path, const char *mode);
FILE *hb_real_fopen(const char *path, const char *mode); // no path mapping, no fetch
HANDLE CreateFile(LPCSTR name, DWORD access, DWORD share, LPSECURITY_ATTRIBUTES, DWORD disposition, DWORD flags, HANDLE);
BOOL ReadFile(HANDLE h, LPVOID buf, DWORD n, LPDWORD read, LPOVERLAPPED);
BOOL WriteFile(HANDLE h, LPCVOID buf, DWORD n, LPDWORD written, LPOVERLAPPED);
DWORD SetFilePointer(HANDLE h, LONG dist, LONG *high, DWORD method);
DWORD GetFileSize(HANDLE h, LPDWORD high);
BOOL CloseHandle(HANDLE h);
BOOL DeleteFile(LPCSTR name);
int _mkdir(const char *path);
int _access(const char *path, int mode);
#define fopen hb_fopen

// ---- windowing / misc: the browser page is the window ---------------------
#define WM_DESTROY 0x0002
#define WM_CLOSE 0x0010
#define WM_ACTIVATEAPP 0x001C
#define WM_USER 0x0400
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_IME_COMPOSITION 0x010F
#define WM_IME_STARTCOMPOSITION 0x010D
#define WM_IME_ENDCOMPOSITION 0x010E
#define WM_IME_NOTIFY 0x0282
#define GCS_RESULTSTR 0x0800
#define GCS_COMPSTR 0x0008
#define MB_OK 0
#define MB_ICONEXCLAMATION 0x30
#define MB_ICONSTOP 0x10
#define MB_YESNO 4
#define IDOK 1
#define IDYES 6
#define SM_CXSCREEN 0
#define SM_CYSCREEN 1
#define SM_CXFULLSCREEN 16
#define SM_CYFULLSCREEN 17
#define HWND_TOP ((HWND)0)
#define SWP_SHOWWINDOW 0x40
#define SW_SHOW 5
#define SW_SHOWDEFAULT 10
#define HINSTANCE_ERROR 32
#define VK_BACK 0x08
#define VK_TAB 0x09
#define VK_RETURN 0x0D
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_ESCAPE 0x1B
#define VK_SPACE 0x20
#define VK_PRIOR 0x21
#define VK_NEXT 0x22
#define VK_END 0x23
#define VK_HOME 0x24
#define VK_LEFT 0x25
#define VK_UP 0x26
#define VK_RIGHT 0x27
#define VK_DOWN 0x28
#define VK_SNAPSHOT 0x2C
#define VK_INSERT 0x2D
#define VK_DELETE 0x2E
#define VK_LWIN 0x5B
#define VK_RWIN 0x5C
#define VK_MULTIPLY 0x6A
#define VK_ADD 0x6B
#define VK_SEPARATOR 0x6C
#define VK_SUBTRACT 0x6D
#define VK_DECIMAL 0x6E
#define VK_DIVIDE 0x6F
#define VK_F1 0x70
#define VK_F2 0x71
#define VK_F3 0x72
#define VK_F4 0x73
#define VK_F5 0x74
#define VK_F6 0x75
#define VK_F7 0x76
#define VK_F8 0x77
#define VK_F9 0x78
#define VK_F10 0x79
#define VK_F11 0x7A
#define VK_F12 0x7B
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91
int MessageBox(HWND, LPCSTR text, LPCSTR caption, UINT);
inline int GetSystemMetrics(int i) { return (i == SM_CYSCREEN || i == SM_CYFULLSCREEN) ? 480 : 640; }
inline HCURSOR SetCursor(HCURSOR) { return NULL; }
inline int ShowCursor(BOOL) { return 0; }
inline BOOL SetWindowPos(HWND, HWND, int, int, int, int, UINT) { return TRUE; }
inline BOOL SetWindowText(HWND, LPCSTR) { return TRUE; }
inline BOOL FlashWindow(HWND, BOOL) { return TRUE; }
inline int GetWindowText(HWND, LPSTR s, int) { if (s) s[0] = 0; return 0; }
inline void PostQuitMessage(int) {}
inline BOOL PostMessage(HWND, UINT, WPARAM, LPARAM) { return TRUE; }
inline LRESULT SendMessage(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline LRESULT DefWindowProc(HWND, UINT, WPARAM, LPARAM) { return 0; }
inline void OutputDebugString(LPCSTR s) { fputs(s, stderr); }
inline BOOL GetCursorPos(LPPOINT p) { p->x = p->y = 0; return TRUE; }
short GetAsyncKeyState(int vk);
inline HINSTANCE ShellExecute(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, int) { return (HINSTANCE)(intptr_t)33; }
inline UINT WinExec(LPCSTR, UINT) { return 33; }

// IME: the browser hands text in through WM_CHAR instead
inline HIMC ImmGetContext(HWND) { return NULL; }
inline BOOL ImmReleaseContext(HWND, HIMC) { return TRUE; }
inline LONG ImmGetCompositionString(HIMC, DWORD, LPVOID, DWORD) { return 0; }
typedef struct tagCOMPOSITIONFORM { DWORD dwStyle; POINT ptCurrentPos; RECT rcArea; } COMPOSITIONFORM;
#define CFS_POINT 2
inline BOOL ImmSetCompositionWindow(HIMC, COMPOSITIONFORM *) { return TRUE; }

// Registry: nothing is stored there in the browser
#define HKEY_CLASSES_ROOT ((HKEY)(uintptr_t)0x80000000)
#define HKEY_CURRENT_USER ((HKEY)(uintptr_t)0x80000001)
#define HKEY_LOCAL_MACHINE ((HKEY)(uintptr_t)0x80000002)
#define ERROR_SUCCESS 0L
#define KEY_QUERY_VALUE 1
#define KEY_ALL_ACCESS 0xF003F
#define KEY_WRITE 0x20006
#define REG_SZ 1
#define REG_DWORD 4
#define REG_OPTION_NON_VOLATILE 0
typedef DWORD REGSAM;
inline LONG RegOpenKeyEx(HKEY, LPCSTR, DWORD, REGSAM, HKEY *) { return 2; }
inline LONG RegCreateKeyEx(HKEY, LPCSTR, DWORD, LPSTR, DWORD, REGSAM, LPSECURITY_ATTRIBUTES, HKEY *, LPDWORD) { return 2; }
inline LONG RegQueryValueEx(HKEY, LPCSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD) { return 2; }
inline LONG RegQueryValue(HKEY, LPCSTR, LPSTR, LONG *) { return 2; }
inline LONG RegSetValueEx(HKEY, LPCSTR, DWORD, DWORD, const BYTE *, DWORD) { return 2; }
inline LONG RegCloseKey(HKEY) { return 0; }

// DLLs: none in the browser
inline HMODULE LoadLibrary(LPCSTR) { return NULL; }
inline FARPROC GetProcAddress(HMODULE, LPCSTR) { return NULL; }
inline BOOL FreeLibrary(HMODULE) { return TRUE; }

#include "../ddraw_web.h"

#endif
