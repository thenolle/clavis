#include "../include/common.h"
#include "../include/audio.h"
#include "../include/hooks.h"
#include "../include/soundpack.h"
#include "../include/tray.h"
#include "../include/ui.h"
#include "../include/hotkey.h"

#define REG_RUN_KEY  L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define REG_APP_NAME L"Clavis"

static BOOL  s_start_with_windows = FALSE;
static BOOL  s_close_to_tray      = TRUE;
#define STATUS_RESET_TIMER  2
#define STATUS_RESET_DELAY  3000

static SoundPack s_pack        = {0};
static BOOL      s_pack_loaded = FALSE;
static wchar_t   s_pack_names[MAX_PACKS][MAX_PACK_NAME];
static wchar_t   s_pack_dirs [MAX_PACKS][MAX_PATH];
static int       s_pack_count  = 0;
static int       s_pack_sel    = -1;
static HWND      s_hwnd        = NULL;
static HINSTANCE s_hInst       = NULL;
static BOOL      s_remap_active = FALSE;

BOOL app_get_close_to_tray(void) { return s_close_to_tray; }

static void status_reset(void) {
    int total = s_pack_count + g_embedded_pack_count;
    wchar_t buf[64];
    swprintf(buf, 64, L"Loaded %d pack%s", total, total == 1 ? L"" : L"s");
    ui_set_status(buf, 0);
}

static void set_status_temp(const wchar_t *text, COLORREF col) {
    ui_set_status(text, col);
    KillTimer(s_hwnd, STATUS_RESET_TIMER);
    SetTimer(s_hwnd, STATUS_RESET_TIMER, STATUS_RESET_DELAY, NULL);
}

static BOOL get_start_with_windows(void) {
    HKEY hk;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_READ, &hk) != ERROR_SUCCESS)
        return FALSE;
    DWORD size = 0;
    BOOL found = (RegQueryValueExW(hk, REG_APP_NAME, NULL, NULL, NULL, &size) == ERROR_SUCCESS);
    RegCloseKey(hk);
    return found;
}

static void set_start_with_windows(BOOL enable) {
    HKEY hk;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_RUN_KEY, 0, KEY_SET_VALUE, &hk) != ERROR_SUCCESS)
        return;
    if (enable) {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        RegSetValueExW(hk, REG_APP_NAME, 0, REG_SZ, (BYTE *)path, (DWORD)((wcslen(path) + 1) * sizeof(wchar_t)));
    } else {
        RegDeleteValueW(hk, REG_APP_NAME);
    }
    RegCloseKey(hk);
    s_start_with_windows = enable;
}

static void get_soundpacks_dir(wchar_t *out, int len) {
    GetModuleFileNameW(NULL, out, len);
    wchar_t *bs = wcsrchr(out, L'\\');
    if (bs) { bs[1] = L'\0'; wcsncat(out, L"Soundpacks", len - (int)wcslen(out) - 1); }
}

static void do_load_pack(int idx) {
    if (s_pack_loaded) sp_unload(&s_pack);
    s_pack_loaded = FALSE;
    BOOL ok = FALSE;
    if (idx < g_embedded_pack_count)
        ok = sp_load_embedded(idx, &s_pack);
    else {
        int di = idx - g_embedded_pack_count;
        if (di >= 0 && di < s_pack_count)
            ok = sp_load(s_pack_dirs[di], &s_pack);
    }
    if (ok) {
        s_pack_loaded = TRUE;
        s_pack_sel    = idx;
        ui_set_selected_idx(idx);
        wchar_t msg[256];
        swprintf(msg, 256, L"Loaded %s", s_pack.name);
        set_status_temp(msg, g_theme.success);
    } else {
        ui_set_status(L"Failed to load soundpack", g_theme.destructive);
    }
}

static void on_pack_selected(int idx)  { do_load_pack(idx); }
static void on_volume_changed(int vol) { audio_set_volume(vol); }

static void rebuild_pack_list(void) {
    wchar_t root[MAX_PATH];
    get_soundpacks_dir(root, MAX_PATH);
    CreateDirectoryW(root, NULL);

    s_pack_count = sp_discover(root, s_pack_names, s_pack_dirs, MAX_PACKS);

    int total = g_embedded_pack_count + s_pack_count;
    wchar_t all[MAX_PACKS][MAX_PACK_NAME];

    for (int i = 0; i < g_embedded_pack_count && i < MAX_PACKS; i++) {
        SoundPack tmp;
        all[i][0] = L'\0';

        if (sp_load_embedded(i, &tmp)) {
            _snwprintf(all[i], MAX_PACK_NAME - 1, L"\u2605 %s", tmp.name);
            all[i][MAX_PACK_NAME - 1] = L'\0';
        } else {
            wchar_t raw[MAX_PACK_NAME];
            MultiByteToWideChar(CP_UTF8, 0, g_embedded_packs[i].dir_name, -1, raw, MAX_PACK_NAME);
            _snwprintf(all[i], MAX_PACK_NAME - 1, L"\u2605 %s", raw);
            all[i][MAX_PACK_NAME - 1] = L'\0';
        }

        sp_unload(&tmp);
    }
    for (int i = 0; i < s_pack_count && (g_embedded_pack_count + i) < MAX_PACKS; i++)
        wcsncpy(all[g_embedded_pack_count + i], s_pack_names[i], MAX_PACK_NAME - 1);

    ui_populate_packs(all, total, s_pack_sel);
    status_reset();
}

static void open_soundpacks_folder(void) {
    wchar_t dir[MAX_PATH];
    get_soundpacks_dir(dir, MAX_PATH);
    CreateDirectoryW(dir, NULL);
    ShellExecuteW(NULL, L"explore", dir, NULL, NULL, SW_SHOWNORMAL);
}

static void toggle_mute(void) {
    g_muted = !g_muted;
    ui_set_muted(g_muted);
    wchar_t tip[64];
    swprintf(tip, 64, L"Clavis \u2014 %s", g_muted ? L"Muted" : L"Active");
    tray_update_tooltip(tip);
}

static LRESULT CALLBACK AppSubclass(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR uid, DWORD_PTR ref) {
    (void)uid; (void)ref;
    switch (msg) {
    case WM_HOOK_KEY:
        if (!s_pack_loaded || g_muted) break;
        {
            DWORD vk    = (DWORD)(wp & 0xFFFF);
            BOOL  is_up = (BOOL)((wp >> 16) & 1);
            const wchar_t       *path = NULL;
            const unsigned char *data = NULL;
            unsigned int         size = 0;
            sp_get_sound(&s_pack, vk, is_up, &path, &data, &size);
            int vol = ui_get_volume();
            if (data && size)         audio_play_mem(data, size, vol);
            else if (path && path[0]) audio_play(path, vol);
        }
        return 0;

    case WM_HOOK_MOUSE:
        if (!s_pack_loaded || g_muted || !g_hook_mouse) break;
        {
            BOOL is_up = (BOOL)((wp >> 16) & 1);
            const unsigned char *data = is_up ? s_pack.mouse_up_data   : s_pack.mouse_down_data;
            unsigned int         size = is_up ? s_pack.mouse_up_size   : s_pack.mouse_down_size;
            const wchar_t       *path = is_up ? s_pack.mouse_up_path   : s_pack.mouse_down_path;
            int vol = ui_get_volume();
            if (data && size)         audio_play_mem(data, size, vol);
            else if (path && path[0]) audio_play(path, vol);
        }
        return 0;

    case WM_HOTKEY_PRESSED:
        if ((WPARAM)wp == 1) {
            ui_update_hotkey_label();
            set_status_temp(L"Hotkey updated", g_theme.success);
            s_remap_active = FALSE;
            wchar_t hk[128], tip[192];
            hotkey_get_label(hk, 128);
            swprintf(tip, 192, L"Clavis \u2014 %s \u2022 %s", g_muted ? L"Muted" : L"Active", hk);
            tray_update_tooltip(tip);
        } else {
            toggle_mute();
        }
        return 0;

    case WM_TIMER:
        if (wp == STATUS_RESET_TIMER) {
            KillTimer(hwnd, STATUS_RESET_TIMER);
            status_reset();
        }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == IDC_MUTE_BTN && HIWORD(wp) == BN_CLICKED) {
            toggle_mute();
            return 0;
        }
        if (LOWORD(wp) == IDC_HOTKEY_BTN && HIWORD(wp) == BN_CLICKED) {
            if (!s_remap_active) {
                s_remap_active = TRUE;
                hotkey_start_remap();
                ui_set_status(L"Press a new key combination\u2026", g_theme.accent);
            } else {
                hotkey_cancel_remap();
                s_remap_active = FALSE;
                set_status_temp(L"Remap cancelled", g_theme.text_faint);
            }
            return 0;
        }
        if (LOWORD(wp) == IDC_PACK_RELOAD && HIWORD(wp) == BN_CLICKED) {
            rebuild_pack_list();
            return 0;
        }
        if (LOWORD(wp) == IDC_OPEN_FOLDER && HIWORD(wp) == BN_CLICKED) {
            open_soundpacks_folder();
            return 0;
        }
        if (LOWORD(wp) == IDM_TRAYSTARTUP) {
            set_start_with_windows(!s_start_with_windows);
            return 0;
        }
        if (LOWORD(wp) == IDM_TRAYCLOSETOTRAY) {
            s_close_to_tray = !s_close_to_tray;
            ui_set_close_to_tray(s_close_to_tray);
            return 0;
        }
        break;

    case WM_TRAY_MSG:
        if (lp == WM_LBUTTONDBLCLK) {
            if (IsWindowVisible(hwnd)) {
                ShowWindow(hwnd, SW_HIDE);
            } else {
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);
            }
        } else if (lp == WM_RBUTTONUP) {
            if (tray_show_menu(hwnd, g_muted, s_start_with_windows, s_close_to_tray))
                DestroyWindow(hwnd);
        }
        return 0;

    case WM_CLOSE:
        if (s_close_to_tray)
            ShowWindow(hwnd, SW_HIDE);
        else
            DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        tray_remove();
        hooks_uninstall();
        hotkey_shutdown();
        audio_shutdown();
        PostQuitMessage(0);
        return 0;
    }
    return DefSubclassProc(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmd, int show) {
    (void)hPrev; (void)cmd;
    s_hInst = hInst;

    HANDLE mtx = CreateMutexW(NULL, TRUE, APP_MUTEX);
    if (GetLastError() == ERROR_ALREADY_EXISTS) { CloseHandle(mtx); return 0; }

    InitCommonControls();
    theme_init();
    audio_init();

    s_start_with_windows = get_start_with_windows();

    s_hwnd = ui_create(hInst, show);
    if (!s_hwnd) return 1;

    ui_set_callbacks(on_pack_selected, on_volume_changed);
    ui_set_close_to_tray(s_close_to_tray);
    SetWindowSubclass(s_hwnd, AppSubclass, 1, 0);

    HICON ico = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APP));
    tray_add(s_hwnd, ico);

    hooks_install(s_hwnd);
    hotkey_init(s_hwnd);

    rebuild_pack_list();

    int total = g_embedded_pack_count + s_pack_count;
    if (total > 0) {
        do_load_pack(0);
        HWND lst = GetDlgItem(s_hwnd, IDC_PACK_LIST);
        if (lst) SendMessageW(lst, LB_SETCURSEL, 0, 0);
    }

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    theme_free();
    CloseHandle(mtx);
    return (int)msg.wParam;
}