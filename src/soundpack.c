#include "../include/soundpack.h"

static const char *json_find_key(const char *json, const char *key)
{
    char needle[256];
    snprintf(needle, sizeof(needle), "\"%s\"", key);
    const char *p = json;
    while ((p = strstr(p, needle)) != NULL) {
        p += strlen(needle);
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
        if (*p == ':') { p++; return p; }
    }
    return NULL;
}

static BOOL json_get_string(const char *json, const char *key, char *dst, int dst_size)
{
    const char *p = json_find_key(json, key);
    if (!p) return FALSE;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return FALSE;
    p++;
    int i = 0;
    while (*p && *p != '"' && i < dst_size - 1) {
        if (*p == '\\') p++;
        if (*p) dst[i++] = *p++;
    }
    dst[i] = '\0';
    return TRUE;
}

typedef struct { char key_name[MAX_KEY_NAME]; char sound_file[MAX_PATH]; } RawKS;

static int json_parse_defines(const char *json, const char *section, RawKS *out, int max_entries)
{
    const char *p = json_find_key(json, section);
    if (!p) return 0;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    if (*p != '{') return 0;
    p++;
    int count = 0;
    while (*p && *p != '}' && count < max_entries) {
        while (*p && (*p==' '||*p=='\t'||*p=='\r'||*p=='\n'||*p==',')) p++;
        if (*p == '}' || !*p) break;
        if (*p != '"') { p++; continue; }
        p++;
        int ki = 0;
        while (*p && *p != '"' && ki < MAX_KEY_NAME - 1)
            out[count].key_name[ki++] = *p++;
        out[count].key_name[ki] = '\0';
        if (*p == '"') p++;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == ':') p++;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '[') { p++; while (*p && *p!='"' && *p!=']') p++; }
        if (*p == '"') {
            p++;
            int ti = 0;
            while (*p && *p != '"' && ti < MAX_PATH - 1) {
                if (*p == '\\') p++;
                if (*p) out[count].sound_file[ti++] = *p++;
            }
            out[count].sound_file[ti] = '\0';
            if (*p == '"') p++;
            count++;
        }
        int depth = 0;
        while (*p && !((*p==','||*p=='}') && depth==0)) {
            if (*p=='['||*p=='{') depth++;
            else if (*p==']'||*p=='}') { if (depth>0) depth--; else break; }
            p++;
        }
    }
    return count;
}

static const struct { DWORD vk; const char *name; } s_vk_map[] = {
    {VK_BACK,    "KEY_BACKSPACE"}, {VK_TAB,     "KEY_TAB"},
    {VK_RETURN,  "KEY_RETURN"},    {VK_CAPITAL, "KEY_CAPSLOCK"},
    {VK_ESCAPE,  "KEY_ESC"},       {VK_SPACE,   "KEY_SPACE"},
    {VK_PRIOR,   "KEY_PAGEUP"},    {VK_NEXT,    "KEY_PAGEDOWN"},
    {VK_END,     "KEY_END"},       {VK_HOME,    "KEY_HOME"},
    {VK_LEFT,    "KEY_LEFT"},      {VK_UP,      "KEY_UP"},
    {VK_RIGHT,   "KEY_RIGHT"},     {VK_DOWN,    "KEY_DOWN"},
    {VK_INSERT,  "KEY_INSERT"},    {VK_DELETE,  "KEY_DELETE"},
    {VK_LWIN,    "KEY_LEFTMETA"},  {VK_RWIN,    "KEY_RIGHTMETA"},
    {VK_NUMPAD0,"KEY_KP0"},{VK_NUMPAD1,"KEY_KP1"},{VK_NUMPAD2,"KEY_KP2"},
    {VK_NUMPAD3,"KEY_KP3"},{VK_NUMPAD4,"KEY_KP4"},{VK_NUMPAD5,"KEY_KP5"},
    {VK_NUMPAD6,"KEY_KP6"},{VK_NUMPAD7,"KEY_KP7"},{VK_NUMPAD8,"KEY_KP8"},
    {VK_NUMPAD9,"KEY_KP9"},
    {VK_MULTIPLY,"KEY_KPASTERISK"},{VK_ADD,"KEY_KPPLUS"},
    {VK_SUBTRACT,"KEY_KPMINUS"},{VK_DECIMAL,"KEY_KPDOT"},{VK_DIVIDE,"KEY_KPSLASH"},
    {VK_F1,"KEY_F1"},{VK_F2,"KEY_F2"},{VK_F3,"KEY_F3"},{VK_F4,"KEY_F4"},
    {VK_F5,"KEY_F5"},{VK_F6,"KEY_F6"},{VK_F7,"KEY_F7"},{VK_F8,"KEY_F8"},
    {VK_F9,"KEY_F9"},{VK_F10,"KEY_F10"},{VK_F11,"KEY_F11"},{VK_F12,"KEY_F12"},
    {VK_NUMLOCK,"KEY_NUMLOCK"},{VK_SCROLL,"KEY_SCROLLLOCK"},
    {VK_LSHIFT,"KEY_LEFTSHIFT"},{VK_RSHIFT,"KEY_RIGHTSHIFT"},
    {VK_LCONTROL,"KEY_LEFTCTRL"},{VK_RCONTROL,"KEY_RIGHTCTRL"},
    {VK_LMENU,"KEY_LEFTALT"},{VK_RMENU,"KEY_RIGHTALT"},
    {VK_SHIFT,"KEY_LEFTSHIFT"},{VK_CONTROL,"KEY_LEFTCTRL"},{VK_MENU,"KEY_LEFTALT"},
    {VK_OEM_1,"KEY_SEMICOLON"},{VK_OEM_PLUS,"KEY_EQUAL"},
    {VK_OEM_COMMA,"KEY_COMMA"},{VK_OEM_MINUS,"KEY_MINUS"},
    {VK_OEM_PERIOD,"KEY_DOT"},{VK_OEM_2,"KEY_SLASH"},
    {VK_OEM_3,"KEY_GRAVE"},{VK_OEM_4,"KEY_LEFTBRACE"},
    {VK_OEM_5,"KEY_BACKSLASH"},{VK_OEM_6,"KEY_RIGHTBRACE"},
    {VK_OEM_7,"KEY_APOSTROPHE"},
    {0, NULL}
};

static const char *vk_to_name(DWORD vk)
{
    static char buf[16];
    if (vk >= '0' && vk <= '9') { snprintf(buf,16,"KEY_%c",(char)vk); return buf; }
    if (vk >= 'A' && vk <= 'Z') { snprintf(buf,16,"KEY_%c",(char)vk); return buf; }
    for (int i = 0; s_vk_map[i].name; i++)
        if (s_vk_map[i].vk == vk) return s_vk_map[i].name;
    return NULL;
}

static const EmbeddedFile *emb_find(const EmbeddedPack *ep, const char *fname)
{
    for (int i = 0; i < ep->file_count; i++)
        if (_stricmp(ep->files[i].filename, fname) == 0)
            return &ep->files[i];
    return NULL;
}

static void emb_resolve_ks(KeySound *ks, const EmbeddedPack *ep, const char *fname)
{
    const EmbeddedFile *ef = emb_find(ep, fname);
    if (ef) {
        ks->emb_data = ef->data;
        ks->emb_size = ef->size;
    } else {
        ks->emb_data = NULL;
        ks->emb_size = 0;
    }
    ks->sound_file[0] = L'\0';
}

static BOOL parse_config(const char *buf, const wchar_t *dir, const EmbeddedPack *ep, SoundPack *out)
{
    char n8[MAX_PACK_NAME] = {0};
    if (!json_get_string(buf, "name", n8, MAX_PACK_NAME)) {
        if (dir) {
            WideCharToMultiByte(CP_UTF8,0,dir,-1,n8,MAX_PACK_NAME,NULL,NULL);
            char *last = strrchr(n8, '\\');
            if (last) memmove(n8, last+1, strlen(last+1)+1);
        } else if (ep) {
            strncpy(n8, ep->dir_name, MAX_PACK_NAME - 1);
        }
    }
    MultiByteToWideChar(CP_UTF8, 0, n8, -1, out->name, MAX_PACK_NAME);

    char kdt[32] = "single";
    json_get_string(buf, "key_define_type", kdt, sizeof(kdt));

    char single[MAX_PATH] = {0};
    json_get_string(buf, "sound", single, MAX_PATH);

    if (strcmp(kdt, "single") == 0 && single[0]) {
        strncpy(out->sounds[0].key_name, "default", MAX_KEY_NAME - 1);
        out->sounds[0].key_name[MAX_KEY_NAME - 1] = '\0';
        if (dir) {
            wchar_t ws[MAX_PATH];
            MultiByteToWideChar(CP_UTF8,0,single,-1,ws,MAX_PATH);
            swprintf(out->sounds[0].sound_file, MAX_PATH, L"%s\\%s", dir, ws);
            out->sounds[0].emb_data = NULL;
            out->sounds[0].emb_size = 0;
        } else {
            emb_resolve_ks(&out->sounds[0], ep, single);
        }
        out->sound_count = 1;
    } else {
        RawKS raw[MAX_SOUNDS];
        int n = json_parse_defines(buf, "defines", raw, MAX_SOUNDS);
        for (int i = 0; i < n; i++) {
            memcpy(out->sounds[i].key_name, raw[i].key_name, MAX_KEY_NAME - 1);
            out->sounds[i].key_name[MAX_KEY_NAME - 1] = '\0';
            if (dir) {
                wchar_t ws[MAX_PATH];
                MultiByteToWideChar(CP_UTF8,0,raw[i].sound_file,-1,ws,MAX_PATH);
                swprintf(out->sounds[i].sound_file, MAX_PATH, L"%s\\%s", dir, ws);
                out->sounds[i].emb_data = NULL;
                out->sounds[i].emb_size = 0;
            } else {
                emb_resolve_ks(&out->sounds[i], ep, raw[i].sound_file);
            }
        }
        out->sound_count = n;
    }

    RawKS uraw[MAX_SOUNDS];
    int un = json_parse_defines(buf, "defines_up", uraw, MAX_SOUNDS);
    for (int i = 0; i < un; i++) {
        memcpy(out->up_sounds[i].key_name, uraw[i].key_name, MAX_KEY_NAME - 1);
        out->up_sounds[i].key_name[MAX_KEY_NAME - 1] = '\0';
        if (dir) {
            wchar_t ws[MAX_PATH];
            MultiByteToWideChar(CP_UTF8,0,uraw[i].sound_file,-1,ws,MAX_PATH);
            swprintf(out->up_sounds[i].sound_file, MAX_PATH, L"%s\\%s", dir, ws);
            out->up_sounds[i].emb_data = NULL;
            out->up_sounds[i].emb_size = 0;
        } else {
            emb_resolve_ks(&out->up_sounds[i], ep, uraw[i].sound_file);
        }
    }
    out->up_sound_count = un;

    char ms[MAX_PATH]={0}, mu[MAX_PATH]={0};
    json_get_string(buf, "mouse_down", ms, MAX_PATH);
    json_get_string(buf, "mouse_up",   mu, MAX_PATH);

    if (ms[0]) {
        if (dir) {
            wchar_t t[MAX_PATH]; MultiByteToWideChar(CP_UTF8,0,ms,-1,t,MAX_PATH);
            swprintf(out->mouse_down_path, MAX_PATH, L"%s\\%s", dir, t);
            out->mouse_down_data = NULL; out->mouse_down_size = 0;
        } else {
            const EmbeddedFile *ef = emb_find(ep, ms);
            if (ef) { out->mouse_down_data=ef->data; out->mouse_down_size=ef->size; }
        }
    }
    if (mu[0]) {
        if (dir) {
            wchar_t t[MAX_PATH]; MultiByteToWideChar(CP_UTF8,0,mu,-1,t,MAX_PATH);
            swprintf(out->mouse_up_path, MAX_PATH, L"%s\\%s", dir, t);
            out->mouse_up_data = NULL; out->mouse_up_size = 0;
        } else {
            const EmbeddedFile *ef = emb_find(ep, mu);
            if (ef) { out->mouse_up_data=ef->data; out->mouse_up_size=ef->size; }
        }
    }

    if (dir) {
        wchar_t ico[MAX_PATH];
        swprintf(ico, MAX_PATH, L"%s\\icon.png", dir);
        if (GetFileAttributesW(ico) != INVALID_FILE_ATTRIBUTES)
            wcsncpy(out->icon, ico, MAX_PATH - 1);
    }

    return (out->sound_count > 0);
}

int sp_discover(const wchar_t *root, wchar_t names[][MAX_PACK_NAME], wchar_t dirs[][MAX_PATH], int max_packs)
{
    wchar_t pat[MAX_PATH];
    swprintf(pat, MAX_PATH, L"%s\\*", root);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pat, &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    int count = 0;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (!wcscmp(fd.cFileName,L".")||!wcscmp(fd.cFileName,L"..")) continue;
        if (count >= max_packs) break;
        wchar_t cfg[MAX_PATH];
        swprintf(cfg, MAX_PATH, L"%s\\%s\\config.json", root, fd.cFileName);
        if (GetFileAttributesW(cfg) == INVALID_FILE_ATTRIBUTES) continue;
        swprintf(dirs[count], MAX_PATH, L"%s\\%s", root, fd.cFileName);
        wcsncpy(names[count], fd.cFileName, MAX_PACK_NAME - 1);
        names[count][MAX_PACK_NAME - 1] = L'\0';
        count++;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return count;
}

BOOL sp_load(const wchar_t *dir, SoundPack *out)
{
    memset(out, 0, sizeof(*out));
    wcsncpy(out->dir, dir, MAX_PATH - 1);
    out->is_embedded = FALSE;

    wchar_t cfg_path[MAX_PATH];
    swprintf(cfg_path, MAX_PATH, L"%s\\config.json", dir);
    HANDLE hf = CreateFileW(cfg_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hf == INVALID_HANDLE_VALUE) return FALSE;
    DWORD size = GetFileSize(hf, NULL);
    if (!size || size > 2*1024*1024) { CloseHandle(hf); return FALSE; }
    char *buf = malloc(size + 1);
    if (!buf) { CloseHandle(hf); return FALSE; }
    DWORD nr = 0;
    ReadFile(hf, buf, size, &nr, NULL);
    buf[nr] = '\0';
    CloseHandle(hf);
    BOOL ok = parse_config(buf, dir, NULL, out);
    free(buf);
    return ok;
}

BOOL sp_load_embedded(int idx, SoundPack *out)
{
    if (idx < 0 || idx >= g_embedded_pack_count) return FALSE;
    const EmbeddedPack *ep = &g_embedded_packs[idx];
    memset(out, 0, sizeof(*out));
    out->is_embedded = TRUE;

    const EmbeddedFile *cfg_file = emb_find(ep, "config.json");
    if (!cfg_file) return FALSE;

    char *buf = malloc(cfg_file->size + 1);
    if (!buf) return FALSE;
    memcpy(buf, cfg_file->data, cfg_file->size);
    buf[cfg_file->size] = '\0';

    BOOL ok = parse_config(buf, NULL, ep, out);
    free(buf);
    return ok;
}

void sp_get_sound(const SoundPack *pack, DWORD vk_code, BOOL key_up, const wchar_t **out_path, const unsigned char **out_data, unsigned int *out_size)
{
    *out_path = NULL;
    *out_data = NULL;
    *out_size = 0;

    const KeySound *sounds = key_up ? pack->up_sounds : pack->sounds;
    int             count  = key_up ? pack->up_sound_count : pack->sound_count;
    if (count == 0) return;

    const KeySound *found = NULL;
    const char *kn = vk_to_name(vk_code);

    if (kn) {
        for (int i = 0; i < count; i++)
            if (_stricmp(sounds[i].key_name, kn) == 0) { found=&sounds[i]; break; }
    }
    if (!found) {
        for (int i = 0; i < count; i++)
            if (_stricmp(sounds[i].key_name, "default") == 0) { found=&sounds[i]; break; }
    }
    if (!found) found = &sounds[0];

    if (found->emb_data) {
        *out_data = found->emb_data;
        *out_size = found->emb_size;
    } else if (found->sound_file[0]) {
        *out_path = found->sound_file;
    }
}

void sp_unload(SoundPack *pack) { memset(pack, 0, sizeof(*pack)); }