// DirectSound subset on top of Web Audio, for the Emscripten build: SoundBuffer.cpp and YWSound.cpp run unchanged.
#pragma once
#ifndef HB_WEB_DSOUND_H
#define HB_WEB_DSOUND_H
#include <stddef.h>
#include "include/windows.h"

#define DS_OK 0
#define DSERR_GENERIC E_FAIL
#define DSSCL_NORMAL 1
#define DSSCL_PRIORITY 2
#define DSBCAPS_PRIMARYBUFFER 0x1
#define DSBCAPS_STATIC 0x2
#define DSBCAPS_CTRLFREQUENCY 0x20
#define DSBCAPS_CTRLPAN 0x40
#define DSBCAPS_CTRLVOLUME 0x80
#define DSBCAPS_GETCURRENTPOSITION2 0x10000
#define DSBLOCK_ENTIREBUFFER 0x2
#define DSBPLAY_LOOPING 0x1
#define DSBSTATUS_PLAYING 0x1
#define DSBSTATUS_BUFFERLOST 0x2
#define DSBSTATUS_LOOPING 0x4
#define DSBPAN_LEFT -10000
#define DSBPAN_RIGHT 10000
#define DSBPAN_CENTER 0
#define DSBVOLUME_MIN -10000
#define DSBVOLUME_MAX 0
#define DSBFREQUENCY_ORIGINAL 0
#define WAVE_FORMAT_PCM 1

// Packed like mmsystem.h: without it PCMWAVEFORMAT's inner struct pads to 16 bytes, so wBitsPerSample no longer
// sits where WAVEFORMATEX reads it and every 8-bit WAV (most of the music) was played as 16-bit noise.
#pragma pack(push, 1)
typedef struct tWAVEFORMATEX {
    WORD wFormatTag, nChannels;
    DWORD nSamplesPerSec, nAvgBytesPerSec;
    WORD nBlockAlign, wBitsPerSample, cbSize;
} WAVEFORMATEX, *LPWAVEFORMATEX;
typedef struct pcmwaveformat_tag { struct { WORD wFormatTag, nChannels; DWORD nSamplesPerSec, nAvgBytesPerSec; WORD nBlockAlign; } wf; WORD wBitsPerSample; } PCMWAVEFORMAT;
#pragma pack(pop)
static_assert(sizeof(PCMWAVEFORMAT) == 16 && offsetof(WAVEFORMATEX, wBitsPerSample) == 14, "mmsystem layout");
typedef struct _DSBUFFERDESC { DWORD dwSize, dwFlags, dwBufferBytes, dwReserved; LPWAVEFORMATEX lpwfxFormat; } DSBUFFERDESC;
typedef struct _DSCAPS { DWORD dwSize, dwFlags; } DSCAPS;

struct IDirectSoundBuffer {
    int id = 0;            // Web Audio side handle, 0 for the primary buffer
    BYTE *data = nullptr;  // raw PCM as written through Lock
    DWORD size = 0;
    WAVEFORMATEX fmt{};
    LONG volume = 0, pan = 0;
    ~IDirectSoundBuffer();
    HRESULT Lock(DWORD, DWORD, LPVOID *p1, LPDWORD n1, LPVOID *p2, LPDWORD n2, DWORD);
    HRESULT Unlock(LPVOID, DWORD, LPVOID, DWORD);
    HRESULT Play(DWORD, DWORD, DWORD flags);
    HRESULT Stop();
    HRESULT SetCurrentPosition(DWORD) { return DS_OK; } // Play always starts at 0
    HRESULT GetStatus(LPDWORD status);
    HRESULT SetVolume(LONG v);
    HRESULT SetPan(LONG p);
    HRESULT SetFrequency(DWORD) { return DS_OK; }
    HRESULT SetFormat(const WAVEFORMATEX *) { return DS_OK; }
    ULONG Release() { delete this; return 0; }
};
typedef IDirectSoundBuffer *LPDIRECTSOUNDBUFFER;

struct IDirectSound {
    HRESULT SetCooperativeLevel(HWND, DWORD) { return DS_OK; }
    HRESULT CreateSoundBuffer(DSBUFFERDESC *d, IDirectSoundBuffer **out, void *);
    ULONG Release() { delete this; return 0; }
};
typedef IDirectSound *LPDIRECTSOUND;
HRESULT DirectSoundCreate(void *, IDirectSound **out, void *);

#endif
