#include "../include/hotkey.h"
#include "../include/hooks.h"

DWORD g_hotkey_vk   = HOTKEY_DEFAULT_VK;
UINT  g_hotkey_mods = HOTKEY_DEFAULT_MODS;

static HWND   s_hwnd_notify = NULL;
static HWND   s_msg_wnd     = NULL;
static HANDLE s_thread      = NULL;
static DWORD  s_thread_id   = 0;
static BOOL   s_remapping   = FALSE;

static void mod_label(UINT mods, wchar_t *buf, int len) {
    buf[0] = L'\0';
    if (mods & MOD_CONTROL) { wcsncat(buf, L"Ctrl",  len - (int)wcslen(buf) - 1); }
    if (mods & MOD_ALT)     { 
        if (buf[0]) wcsncat(buf, L"+", len - (int)wcslen(buf) - 1);
        wcsncat(buf, L"Alt",   len - (int)wcslen(buf) - 1); 
    }
    if (mods & MOD_SHIFT)   { 
        if (buf[0]) wcsncat(buf, L"+", len - (int)wcslen(buf) - 1);
        wcsncat(buf, L"Shift", len - (int)wcslen(buf) - 1); 
    }
    if (mods & MOD_WIN)     { 
        if (buf[0]) wcsncat(buf, L"+", len - (int)wcslen(buf) - 1);
        wcsncat(buf, L"Win",   len - (int)wcslen(buf) - 1); 
    }
}

static void vk_label(DWORD vk, wchar_t *buf, int len) {
    UINT ch = MapVirtualKeyW(vk, MAPVK_VK_TO_CHAR);
    if (ch >= 0x20 && ch != 0) {
        if (ch >= 'a' && ch <= 'z') ch -= 32;
        buf[0] = (wchar_t)ch; buf[1] = L'\0';
        return;
    }

    UINT sc = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
    LONG lp = (LONG)(sc << 16);
    switch (vk) {
    case VK_INSERT: case VK_DELETE: case VK_HOME:  case VK_END:
    case VK_PRIOR:  case VK_NEXT:   case VK_LEFT:  case VK_RIGHT:
    case VK_UP:     case VK_DOWN:   case VK_NUMLOCK: case VK_RCONTROL:
    case VK_RMENU:  case VK_DIVIDE:
        lp |= (1 << 24);
        break;
    }
    if (GetKeyNameTextW(lp, buf, len) && buf[0] != L'\0')
        return;

    swprintf(buf, len, L"Key(%lu)", vk);
}

void hotkey_get_label(wchar_t *buf, int len) {
    wchar_t mods[64] = {0}, key[64] = {0};
    mod_label(g_hotkey_mods, mods, 64);
    vk_label(g_hotkey_vk, key, 64);
    swprintf(buf, len, L"%s+%s", mods, key);
}

#define WM_HK_REGISTER   (WM_USER + 10)
#define WM_HK_QUIT       (WM_USER + 12)
#define HK_WINDOW_CLASS  L"ClavisHotkeyWnd"

static LRESULT CALLBACK HkWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_HOTKEY && (int)wp == HOTKEY_TOGGLE) {
        if (s_hwnd_notify)
            PostMessageW(s_hwnd_notify, WM_HOTKEY_PRESSED, 0, 0);
        return 0;
    }
    if (msg == WM_HK_REGISTER) {
        UnregisterHotKey(hwnd, HOTKEY_TOGGLE);
        if (!RegisterHotKey(hwnd, HOTKEY_TOGGLE, g_hotkey_mods | MOD_NOREPEAT, g_hotkey_vk))
            RegisterHotKey(hwnd, HOTKEY_TOGGLE, g_hotkey_mods, g_hotkey_vk);
        return 0;
    }
    if (msg == WM_HK_QUIT) {
        UnregisterHotKey(hwnd, HOTKEY_TOGGLE);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static DWORD WINAPI hotkey_thread(LPVOID param) {
    (void)param;
    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = HkWndProc;
    wc.lpszClassName = HK_WINDOW_CLASS;
    RegisterClassExW(&wc);

    s_msg_wnd = CreateWindowExW(0, HK_WINDOW_CLASS, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);
    if (!s_msg_wnd) return 1;

    PostMessageW(s_msg_wnd, WM_HK_REGISTER, 0, 0);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

void hotkey_init(HWND hwnd_notify) {
    s_hwnd_notify = hwnd_notify;
    s_thread = CreateThread(NULL, 0, hotkey_thread, NULL, 0, &s_thread_id);
}

void hotkey_shutdown(void) {
    if (s_msg_wnd) {
        PostMessageW(s_msg_wnd, WM_HK_QUIT, 0, 0);
        if (s_thread) {
            WaitForSingleObject(s_thread, 2000);
            CloseHandle(s_thread);
            s_thread = NULL;
        }
        s_msg_wnd = NULL;
    }
}

void hotkey_start_remap(void)  { s_remapping = TRUE; }
void hotkey_cancel_remap(void) { s_remapping = FALSE; }
BOOL hotkey_is_remapping(void) { return s_remapping; }

void hotkey_finish_remap(DWORD vk, UINT mods) {
    if (!s_remapping) return;
    s_remapping   = FALSE;
    g_hotkey_vk   = vk;
    g_hotkey_mods = mods;
    if (s_msg_wnd)
        PostMessageW(s_msg_wnd, WM_HK_REGISTER, 0, 0);
}