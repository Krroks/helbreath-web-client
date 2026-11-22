// DirectSound buffers backed by Web Audio (see dsound_web.h).
#include "dsound_web.h"
#include <emscripten.h>

// JS keeps one AudioBuffer per id plus the source currently playing it.
EM_JS(int, hbjs_snd_create, (const BYTE *pcm, int bytes, int rate, int channels, int bits), {
    const S = Module.hbSound || (Module.hbSound = { ctx: null, next: 1, bufs: {} });
    if (!S.ctx) S.ctx = new (window.AudioContext || window.webkitAudioContext)();
    const frames = Math.floor(bytes / (channels * bits / 8));
    if (frames <= 0 || rate <= 0) return 0;
    const buf = S.ctx.createBuffer(channels, frames, rate);
    for (let c = 0; c < channels; c++) {
        const out = buf.getChannelData(c);
        for (let i = 0; i < frames; i++) {
            const k = i * channels + c;
            out[i] = bits === 8 ? (HEAPU8[pcm + k] - 128) / 128 : HEAP16[(pcm >> 1) + k] / 32768;
        }
    }
    const id = S.next++;
    S.bufs[id] = { buf: buf, src: null };
    return id;
});
EM_JS(void, hbjs_snd_play, (int id, int loop, int vol, int pan), {
    const S = Module.hbSound, b = S && S.bufs[id];
    if (!b) return;
    if (b.src) { try { b.src.stop(); } catch (e) {} }
    const src = S.ctx.createBufferSource();
    src.buffer = b.buf;
    src.loop = !!loop;
    const gain = S.ctx.createGain();
    gain.gain.value = Math.pow(10, vol / 2000);  // DirectSound volume is in 1/100 dB
    // The music is 8-8.8 kHz: the browser's upsampling to 48 kHz leaves a metallic hiss above the source Nyquist
    // that DirectSound's mixer filtered out. Two low-passes just under it remove it (22 kHz sounds skip this).
    let head = src;
    if (b.buf.sampleRate < 16000)
        for (let k = 0; k < 2; k++) {
            const lp = S.ctx.createBiquadFilter();
            lp.type = 'lowpass'; lp.frequency.value = b.buf.sampleRate * 0.45; lp.Q.value = 0.7071;
            head = head.connect(lp);
        }
    const panner = S.ctx.createStereoPanner ? S.ctx.createStereoPanner() : null;
    if (panner) { panner.pan.value = Math.max(-1, Math.min(1, pan / 10000)); head.connect(gain).connect(panner).connect(S.ctx.destination); }
    else head.connect(gain).connect(S.ctx.destination);
    src.onended = () => { if (b.src === src) b.src = null; };
    b.src = src; b.gain = gain; b.panner = panner;
    src.start();
});
EM_JS(void, hbjs_snd_stop, (int id), {
    const b = Module.hbSound && Module.hbSound.bufs[id];
    if (b && b.src) { try { b.src.stop(); } catch (e) {} b.src = null; }
});
EM_JS(int, hbjs_snd_playing, (int id), {
    const b = Module.hbSound && Module.hbSound.bufs[id];
    return b && b.src ? 1 : 0;
});
EM_JS(void, hbjs_snd_set, (int id, int vol, int pan), {
    const b = Module.hbSound && Module.hbSound.bufs[id];
    if (!b || !b.src) return;
    b.gain.gain.value = Math.pow(10, vol / 2000);
    if (b.panner) b.panner.pan.value = Math.max(-1, Math.min(1, pan / 10000));
});
EM_JS(void, hbjs_snd_free, (int id), {
    const S = Module.hbSound, b = S && S.bufs[id];
    if (!b) return;
    if (b.src) { try { b.src.stop(); } catch (e) {} }
    delete S.bufs[id];
});

HRESULT DirectSoundCreate(void *, IDirectSound **out, void *) {
    *out = new IDirectSound;
    return DS_OK;
}

HRESULT IDirectSound::CreateSoundBuffer(DSBUFFERDESC *d, IDirectSoundBuffer **out, void *) {
    auto *b = new IDirectSoundBuffer;
    if (!(d->dwFlags & DSBCAPS_PRIMARYBUFFER) && d->lpwfxFormat) {
        b->fmt = *d->lpwfxFormat;
        b->size = d->dwBufferBytes;
        b->data = (BYTE *)calloc(1, b->size ? b->size : 1);
    }
    *out = b;
    return DS_OK;
}

IDirectSoundBuffer::~IDirectSoundBuffer() {
    if (id) hbjs_snd_free(id);
    free(data);
}

HRESULT IDirectSoundBuffer::Lock(DWORD, DWORD, LPVOID *p1, LPDWORD n1, LPVOID *p2, LPDWORD n2, DWORD) {
    if (!data) return DSERR_GENERIC;
    *p1 = data;
    *n1 = size;
    if (p2) *p2 = nullptr;
    if (n2) *n2 = 0;
    return DS_OK;
}

HRESULT IDirectSoundBuffer::Unlock(LPVOID, DWORD, LPVOID, DWORD) {
    if (id) hbjs_snd_free(id);
    id = hbjs_snd_create(data, (int)size, (int)fmt.nSamplesPerSec, fmt.nChannels ? fmt.nChannels : 1, fmt.wBitsPerSample ? fmt.wBitsPerSample : 16);
    return DS_OK;
}

HRESULT IDirectSoundBuffer::Play(DWORD, DWORD, DWORD flags) {
    if (!id) return DSERR_GENERIC;
    hbjs_snd_play(id, (flags & DSBPLAY_LOOPING) ? 1 : 0, (int)volume, (int)pan);
    return DS_OK;
}

HRESULT IDirectSoundBuffer::Stop() {
    if (id) hbjs_snd_stop(id);
    return DS_OK;
}

HRESULT IDirectSoundBuffer::GetStatus(LPDWORD status) {
    *status = (id && hbjs_snd_playing(id)) ? DSBSTATUS_PLAYING : 0;
    return DS_OK;
}

HRESULT IDirectSoundBuffer::SetVolume(LONG v) {
    volume = v;
    if (id) hbjs_snd_set(id, (int)volume, (int)pan);
    return DS_OK;
}

HRESULT IDirectSoundBuffer::SetPan(LONG p) {
    pan = p;
    if (id) hbjs_snd_set(id, (int)volume, (int)pan);
    return DS_OK;
}
