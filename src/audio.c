#include "../include/audio.h"
#include <mmsystem.h>

static int  s_volume = 80;
static BOOL s_inited = FALSE;

static BOOL is_wav_data(const unsigned char *data, unsigned int size) {
    return size > 4 && data[0]=='R' && data[1]=='I' && data[2]=='F' && data[3]=='F';
}

static BOOL is_wav_path(const wchar_t *p) {
    const wchar_t *e = wcsrchr(p, L'.');
    return e && _wcsicmp(e, L".wav") == 0;
}

static void apply_volume(BYTE *pcm, DWORD bytes, WORD bps, int vol) {
    if (vol >= 100) return;
    float scale = vol / 100.0f;
    if (bps == 16) {
        short *s = (short *)pcm; DWORD n = bytes / 2;
        for (DWORD i = 0; i < n; i++) s[i] = (short)(s[i] * scale);
    } else if (bps == 8) {
        for (DWORD i = 0; i < bytes; i++) {
            int v = (int)pcm[i] - 128;
            pcm[i] = (BYTE)(v * scale + 128);
        }
    }
}

#define NUM_SLOTS 16

typedef struct {
    HWAVEOUT         hwo;
    WAVEHDR          hdr;
    BYTE            *pcm;
    WAVEFORMATEX     wfx;
    BOOL             busy;
    CRITICAL_SECTION cs;
} Slot;

static Slot              s_slots[NUM_SLOTS];
static CRITICAL_SECTION  s_pick_cs;

static void CALLBACK slot_cb(HWAVEOUT hwo, UINT msg, DWORD_PTR inst, DWORD_PTR p1, DWORD_PTR p2) {
    (void)hwo; (void)p1; (void)p2;
    if (msg != WOM_DONE) return;
    Slot *sl = (Slot *)inst;
    EnterCriticalSection(&sl->cs);
    waveOutUnprepareHeader(sl->hwo, &sl->hdr, sizeof(WAVEHDR));
    free(sl->pcm); sl->pcm = NULL;
    memset(&sl->hdr, 0, sizeof(sl->hdr));
    sl->busy = FALSE;
    LeaveCriticalSection(&sl->cs);
}

static void play_wav(const unsigned char *data, unsigned int sz, int vol) {
    if (sz < 44) return;

    DWORD pos = 12;
    WAVEFORMATEX wfx    = {0};
    BOOL         got_fmt = FALSE;
    const BYTE  *pcm_src = NULL;
    DWORD        pcm_sz  = 0;

    while (pos + 8 <= sz) {
        DWORD tag = *(DWORD *)(data + pos);
        DWORD csz = *(DWORD *)(data + pos + 4);
        pos += 8;
        if (pos + csz > sz) break;
        if (tag == 0x20746d66) {
            WAVEFORMAT *wf      = (WAVEFORMAT *)(data + pos);
            wfx.wFormatTag      = wf->wFormatTag;
            wfx.nChannels       = wf->nChannels;
            wfx.nSamplesPerSec  = wf->nSamplesPerSec;
            wfx.nAvgBytesPerSec = wf->nAvgBytesPerSec;
            wfx.nBlockAlign     = wf->nBlockAlign;
            if (csz >= 16) wfx.wBitsPerSample = *(WORD *)(data + pos + 14);
            got_fmt = TRUE;
        } else if (tag == 0x61746164) {
            pcm_src = data + pos;
            pcm_sz  = csz;
        }
        pos += csz + (csz & 1);
    }
    if (!got_fmt || !pcm_src || pcm_sz == 0) return;

    EnterCriticalSection(&s_pick_cs);
    Slot *sl = NULL;
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (!s_slots[i].busy) { sl = &s_slots[i]; sl->busy = TRUE; break; }
    }
    LeaveCriticalSection(&s_pick_cs);
    if (!sl) return;

    BOOL fmt_changed = (memcmp(&sl->wfx, &wfx, sizeof(WAVEFORMATEX)) != 0);
    if (fmt_changed) {
        if (sl->hwo) { waveOutClose(sl->hwo); sl->hwo = NULL; }
        if (waveOutOpen(&sl->hwo, WAVE_MAPPER, &wfx, (DWORD_PTR)slot_cb, (DWORD_PTR)sl, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
            sl->busy = FALSE; return;
        }
        sl->wfx = wfx;
    }

    BYTE *pcm = (BYTE *)malloc(pcm_sz);
    if (!pcm) { sl->busy = FALSE; return; }
    memcpy(pcm, pcm_src, pcm_sz);
    if (wfx.wBitsPerSample) apply_volume(pcm, pcm_sz, wfx.wBitsPerSample, vol);

    sl->pcm = pcm;
    memset(&sl->hdr, 0, sizeof(WAVEHDR));
    sl->hdr.lpData         = (LPSTR)pcm;
    sl->hdr.dwBufferLength = pcm_sz;
    waveOutPrepareHeader(sl->hwo, &sl->hdr, sizeof(WAVEHDR));
    waveOutWrite(sl->hwo, &sl->hdr, sizeof(WAVEHDR));
}

void audio_init(void) {
    InitializeCriticalSection(&s_pick_cs);
    memset(s_slots, 0, sizeof(s_slots));
    for (int i = 0; i < NUM_SLOTS; i++)
        InitializeCriticalSection(&s_slots[i].cs);
    s_inited = TRUE;
}

void audio_shutdown(void) {
    if (!s_inited) return;
    s_inited = FALSE;
    Sleep(30);
    for (int i = 0; i < NUM_SLOTS; i++) {
        if (s_slots[i].hwo) {
            waveOutReset(s_slots[i].hwo);
            if (s_slots[i].hdr.dwFlags & WHDR_PREPARED)
                waveOutUnprepareHeader(s_slots[i].hwo, &s_slots[i].hdr, sizeof(WAVEHDR));
            if (s_slots[i].pcm) { free(s_slots[i].pcm); s_slots[i].pcm = NULL; }
            waveOutClose(s_slots[i].hwo);
            s_slots[i].hwo = NULL;
        }
        DeleteCriticalSection(&s_slots[i].cs);
    }
    DeleteCriticalSection(&s_pick_cs);
}

void audio_set_volume(int pct) {
    s_volume = pct < 0 ? 0 : pct > 100 ? 100 : pct;
}

int audio_get_volume(void) { return s_volume; }

void audio_play_mem(const unsigned char *data, unsigned int size, int vol) {
    if (!s_inited || !data || size == 0) return;
    s_volume = vol < 0 ? 0 : vol > 100 ? 100 : vol;
    if (is_wav_data(data, size))
        play_wav(data, size, s_volume);
}

void audio_play(const wchar_t *filepath, int vol) {
    if (!s_inited || !filepath || !filepath[0]) return;
    if (GetFileAttributesW(filepath) == INVALID_FILE_ATTRIBUTES) return;
    s_volume = vol < 0 ? 0 : vol > 100 ? 100 : vol;

    if (is_wav_path(filepath)) {
        HANDLE hf = CreateFileW(filepath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hf == INVALID_HANDLE_VALUE) return;
        DWORD fsz = GetFileSize(hf, NULL);
        if (!fsz || fsz == INVALID_FILE_SIZE) { CloseHandle(hf); return; }
        unsigned char *buf = (unsigned char *)malloc(fsz);
        if (!buf) { CloseHandle(hf); return; }
        DWORD rd = 0;
        ReadFile(hf, buf, fsz, &rd, NULL);
        CloseHandle(hf);
        if (rd == fsz) play_wav(buf, fsz, s_volume);
        free(buf);
        return;
    }
    PlaySoundW(filepath, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}