// Win32 runtime pieces for the Emscripten build (time, files, message boxes, key state).
//
// Files: the game opens Win32 paths relative to its folder ("sprites\\New-Dialog.pak", "contents\\ItemName.cfg").
// They map to lowercase URLs under the page's data/ folder. .pak/.lpk files are read in place with HTTP Range
// requests (1.1 GB is too much to download); every other file is downloaded whole the first time and kept in
// the in-memory file system, where writes also go.
#define NOMINMAX
#include "include/windows.h"
#include <emscripten.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <string>
#undef fopen

FILE *hb_real_fopen(const char *path, const char *mode) { return fopen(path, mode); }

DWORD timeGetTime() { return (DWORD)(int64_t)emscripten_get_now(); }

void GetLocalTime(LPSYSTEMTIME st) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    double ms = tv.tv_sec * 1000.0 + tv.tv_usec / 1000;
    time_t t = (time_t)(ms / 1000);
    struct tm tmv;
    localtime_r(&t, &tmv);
    st->wYear = (WORD)(tmv.tm_year + 1900);
    st->wMonth = (WORD)(tmv.tm_mon + 1);
    st->wDayOfWeek = (WORD)tmv.tm_wday;
    st->wDay = (WORD)tmv.tm_mday;
    st->wHour = (WORD)tmv.tm_hour;
    st->wMinute = (WORD)tmv.tm_min;
    st->wSecond = (WORD)tmv.tm_sec;
    st->wMilliseconds = (WORD)((int64_t)ms % 1000);
}

int MessageBox(HWND, LPCSTR text, LPCSTR caption, UINT) {
    fprintf(stderr, "[MessageBox] %s: %s\n", caption ? caption : "", text ? text : "");
    return IDOK;
}

static BYTE g_keys[256];
void hb_key_state(int vk, bool down) { if (vk >= 0 && vk < 256) g_keys[vk] = down; }
short GetAsyncKeyState(int vk) { return (vk >= 0 && vk < 256 && g_keys[vk]) ? (short)0x8000 : 0; }

// ---- paths and remote data --------------------------------------------------
const char *hb_path(const char *win) {
    static char buf[MAX_PATH * 2];
    size_t n = 0;
    if (win[0] != '/') buf[n++] = '/';
    for (const char *p = win; *p && n < sizeof(buf) - 1; p++) buf[n++] = *p == '\\' ? '/' : (char)tolower((unsigned char)*p);
    buf[n] = 0;
    // collapse "./" and "//" left by Win32 style paths
    std::string s(buf), out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '/' && i + 2 <= s.size() && s.compare(i, 3, "/./") == 0) { i += 1; continue; }
        if (s[i] == '/' && !out.empty() && out.back() == '/') continue;
        out += s[i];
    }
    strcpy(buf, out.c_str());
    return buf;
}

// Synchronous XHR (main thread); binary through the x-user-defined charset trick. Returns -1 if missing.
EM_JS(int, hbjs_http_size, (const char *path), {
    const x = new XMLHttpRequest();
    x.open('HEAD', (Module.hbDataUrl || 'data') + UTF8ToString(path), false);
    try { x.send(); } catch (e) { return -1; }
    if (x.status !== 200) return -1;
    return parseInt(x.getResponseHeader('Content-Length') || '-1', 10);
});
EM_JS(int, hbjs_http_read, (const char *path, int start, int len, BYTE *dst), {
    const x = new XMLHttpRequest();
    x.open('GET', (Module.hbDataUrl || 'data') + UTF8ToString(path), false);
    x.overrideMimeType('text/plain; charset=x-user-defined');
    if (len >= 0) x.setRequestHeader('Range', 'bytes=' + start + '-' + (start + len - 1));
    try { x.send(); } catch (e) { return -1; }
    if (x.status !== 200 && x.status !== 206) return -1;
    const t = x.responseText, n = t.length;
    for (let i = 0; i < n; i++) HEAPU8[dst + i] = t.charCodeAt(i) & 0xff;
    return n;
});

// Downloads a whole remote file into the in-memory FS the first time it is needed.
static bool ensure_local(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) return true;
    int size = hbjs_http_size(path);
    if (size < 0) return false;
    std::vector<BYTE> data(size ? size : 1);
    if (size && hbjs_http_read(path, 0, -1, data.data()) != size) return false;
    std::string dir(path);
    for (size_t i = 1; i < dir.size(); i++)
        if (dir[i] == '/') { dir[i] = 0; mkdir(dir.c_str(), 0777); dir[i] = '/'; }
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    fwrite(data.data(), 1, size, f);
    fclose(f);
    return true;
}

FILE *hb_fopen(const char *winPath, const char *mode) {
    std::string p = hb_path(winPath);
    if (mode[0] == 'r') ensure_local(p.c_str());
    else {
        std::string dir(p);
        for (size_t i = 1; i < dir.size(); i++)
            if (dir[i] == '/') { dir[i] = 0; mkdir(dir.c_str(), 0777); dir[i] = '/'; }
    }
    return fopen(p.c_str(), mode);
}

int _mkdir(const char *path) { return mkdir(hb_path(path), 0777); }
int _access(const char *path, int) {
    std::string p = hb_path(path);
    struct stat st;
    return (stat(p.c_str(), &st) == 0 || hbjs_http_size(p.c_str()) >= 0) ? 0 : -1;
}
BOOL DeleteFile(LPCSTR name) { return unlink(hb_path(name)) == 0; }

// ---- HANDLE files -------------------------------------------------------------
// shortcut: remote .pak reads use synchronous XHR on the main thread (works, browsers warn); move the game into
// a worker with OPFS caching if loading stalls show up.
struct HbFile {
    FILE *f = nullptr;     // local file
    std::string url;       // remote, read by Range
    DWORD size = 0, pos = 0;
};
static const DWORD CHUNK = 1024 * 1024;
static const size_t CACHE_CAP = 256u * 1024 * 1024;
static std::unordered_map<std::string, std::vector<BYTE>> g_chunks; // "path#chunk" -> bytes
static size_t g_cached;
static std::unordered_map<std::string, int> g_sizes; // remote file sizes (HEAD once per file)

static std::vector<BYTE> *remote_chunk(const std::string &url, DWORD idx, DWORD fileSize) {
    std::string key = url + "#" + std::to_string(idx);
    auto it = g_chunks.find(key);
    if (it != g_chunks.end()) return &it->second;
    DWORD start = idx * CHUNK, len = std::min(CHUNK, fileSize - start);
    std::vector<BYTE> buf(len);
    if (hbjs_http_read(url.c_str(), (int)start, (int)len, buf.data()) != (int)len) return nullptr;
    // shortcut: drop the whole cache when full (sprites keep their own decoded copy); use an LRU if it thrashes
    if (g_cached + len > CACHE_CAP) { g_chunks.clear(); g_cached = 0; }
    g_cached += len;
    return &(g_chunks[key] = std::move(buf));
}

static int remote_size(const std::string &p) {
    auto it = g_sizes.find(p);
    if (it != g_sizes.end()) return it->second;
    return g_sizes[p] = hbjs_http_size(p.c_str());
}

// Hot reload: forget what is cached of a remote file (size and chunks), so the next CreateFile/ReadFile fetches it again.
void hb_invalidate_file(const char *winPath) {
    std::string p = hb_path(winPath), prefix = p + "#";
    g_sizes.erase(p);
    for (auto it = g_chunks.begin(); it != g_chunks.end();)
        if (it->first.compare(0, prefix.size(), prefix) == 0) { g_cached -= it->second.size(); it = g_chunks.erase(it); }
        else ++it;
    unlink(p.c_str()); // a whole copy left by hb_fopen, if any
}

static bool is_pak(const std::string &p) {
    size_t n = p.size();
    return n > 4 && (p.compare(n - 4, 4, ".pak") == 0 || p.compare(n - 4, 4, ".lpk") == 0);
}

HANDLE CreateFile(LPCSTR name, DWORD access, DWORD, LPSECURITY_ATTRIBUTES, DWORD disposition, DWORD, HANDLE) {
    std::string p = hb_path(name);
    auto *h = new HbFile;
    if (!(access & GENERIC_WRITE) && is_pak(p)) {
        int size = remote_size(p);
        if (size < 0) { delete h; return INVALID_HANDLE_VALUE; }
        h->url = p;
        h->size = (DWORD)size;
        return h;
    }
    const char *mode = "rb";
    if (access & GENERIC_WRITE) mode = (disposition == CREATE_ALWAYS || disposition == CREATE_NEW || disposition == TRUNCATE_EXISTING) ? "w+b" : "r+b";
    h->f = hb_fopen(name, mode);
    if (!h->f && (access & GENERIC_WRITE) && disposition == OPEN_ALWAYS) h->f = hb_fopen(name, "w+b");
    if (!h->f) { delete h; return INVALID_HANDLE_VALUE; }
    return h;
}

BOOL ReadFile(HANDLE hh, LPVOID buf, DWORD n, LPDWORD read, LPOVERLAPPED) {
    if (read) *read = 0;
    if (!hh || hh == INVALID_HANDLE_VALUE) return FALSE;
    HbFile *h = (HbFile *)hh;
    if (h->f) {
        size_t r = fread(buf, 1, n, h->f);
        if (read) *read = (DWORD)r;
        return TRUE;
    }
    BYTE *out = (BYTE *)buf;
    DWORD done = 0;
    while (done < n && h->pos < h->size) {
        DWORD idx = h->pos / CHUNK, off = h->pos % CHUNK;
        std::vector<BYTE> *c = remote_chunk(h->url, idx, h->size);
        if (!c) return FALSE;
        DWORD take = std::min(n - done, (DWORD)c->size() - off);
        memcpy(out + done, c->data() + off, take);
        done += take;
        h->pos += take;
    }
    if (read) *read = done;
    return TRUE;
}

BOOL WriteFile(HANDLE hh, LPCVOID buf, DWORD n, LPDWORD written, LPOVERLAPPED) {
    HbFile *h = (HbFile *)hh;
    if (!h || hh == INVALID_HANDLE_VALUE || !h->f) return FALSE;
    size_t w = fwrite(buf, 1, n, h->f);
    if (written) *written = (DWORD)w;
    return w == n;
}

DWORD SetFilePointer(HANDLE hh, LONG dist, LONG *, DWORD method) {
    HbFile *h = (HbFile *)hh;
    if (!h || hh == INVALID_HANDLE_VALUE) return INVALID_FILE_SIZE;
    if (h->f) {
        fseek(h->f, dist, method == FILE_BEGIN ? SEEK_SET : method == FILE_CURRENT ? SEEK_CUR : SEEK_END);
        return (DWORD)ftell(h->f);
    }
    LONG base = method == FILE_BEGIN ? 0 : method == FILE_CURRENT ? (LONG)h->pos : (LONG)h->size;
    h->pos = (DWORD)(base + dist);
    return h->pos;
}

DWORD GetFileSize(HANDLE hh, LPDWORD) {
    HbFile *h = (HbFile *)hh;
    if (!h || hh == INVALID_HANDLE_VALUE) return INVALID_FILE_SIZE;
    if (!h->f) return h->size;
    long cur = ftell(h->f);
    fseek(h->f, 0, SEEK_END);
    long size = ftell(h->f);
    fseek(h->f, cur, SEEK_SET);
    return (DWORD)size;
}

BOOL CloseHandle(HANDLE hh) {
    if (!hh || hh == INVALID_HANDLE_VALUE) return FALSE;
    HbFile *h = (HbFile *)hh;
    if (h->f) fclose(h->f);
    delete h;
    return TRUE;
}
