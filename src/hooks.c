#include "../include/hooks.h"
#include "../include/hotkey.h"

BOOL g_hook_keydown = TRUE;
BOOL g_hook_keyup   = TRUE;
BOOL g_hook_mouse   = TRUE;
BOOL g_muted        = FALSE;

static HHOOK s_kbd_hook   = NULL;
static HHOOK s_mouse_hook = NULL;
static HWND  s_notify_wnd = NULL;

static LRESULT CALLBACK LLKeyProc(int nCode, WPARAM wp, LPARAM lp)
{
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT *ks = (KBDLLHOOKSTRUCT *)lp;
        BOOL is_dn = (wp == WM_KEYDOWN   || wp == WM_SYSKEYDOWN);
        BOOL is_up = (wp == WM_KEYUP     || wp == WM_SYSKEYUP);

        if (hotkey_is_remapping() && is_dn) {
            DWORD vk = ks->vkCode;
            if (vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL &&
                vk != VK_MENU    && vk != VK_LMENU    && vk != VK_RMENU    &&
                vk != VK_SHIFT   && vk != VK_LSHIFT   && vk != VK_RSHIFT   &&
                vk != VK_LWIN    && vk != VK_RWIN) {

                UINT mods = 0;
                if (GetAsyncKeyState(VK_CONTROL) & 0x8000) mods |= MOD_CONTROL;
                if (GetAsyncKeyState(VK_MENU)    & 0x8000) mods |= MOD_ALT;
                if (GetAsyncKeyState(VK_SHIFT)   & 0x8000) mods |= MOD_SHIFT;
                if (GetAsyncKeyState(VK_LWIN)    & 0x8000) mods |= MOD_WIN;
                if (GetAsyncKeyState(VK_RWIN)    & 0x8000) mods |= MOD_WIN;

                hotkey_finish_remap(vk, mods);
                if (s_notify_wnd)
                    PostMessageW(s_notify_wnd, WM_HOTKEY_PRESSED, 1, 0);
                return 1;
            }
        }

        if (!g_muted && s_notify_wnd) {
            if (is_dn && g_hook_keydown)
                PostMessageW(s_notify_wnd, WM_HOOK_KEY, (WPARAM)(ks->vkCode & 0xFFFF), 0);
            else if (is_up && g_hook_keyup)
                PostMessageW(s_notify_wnd, WM_HOOK_KEY, (WPARAM)((ks->vkCode & 0xFFFF) | (1 << 16)), 0);
        }
    }
    return CallNextHookEx(s_kbd_hook, nCode, wp, lp);
}

static LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wp, LPARAM lp)
{
    if (nCode == HC_ACTION && !g_muted && g_hook_mouse && s_notify_wnd) {
        if (wp == WM_LBUTTONDOWN || wp == WM_RBUTTONDOWN || wp == WM_MBUTTONDOWN)
            PostMessageW(s_notify_wnd, WM_HOOK_MOUSE, 0, 0);
        else if (wp == WM_LBUTTONUP || wp == WM_RBUTTONUP || wp == WM_MBUTTONUP)
            PostMessageW(s_notify_wnd, WM_HOOK_MOUSE, (WPARAM)(1 << 16), 0);
    }
    return CallNextHookEx(s_mouse_hook, nCode, wp, lp);
}

void hooks_install(HWND hwnd_notify)
{
    s_notify_wnd = hwnd_notify;
    s_kbd_hook   = SetWindowsHookExW(WH_KEYBOARD_LL, LLKeyProc,   NULL, 0);
    s_mouse_hook = SetWindowsHookExW(WH_MOUSE_LL,    LLMouseProc, NULL, 0);
}

void hooks_uninstall(void)
{
    if (s_kbd_hook)   { UnhookWindowsHookEx(s_kbd_hook);   s_kbd_hook   = NULL; }
    if (s_mouse_hook) { UnhookWindowsHookEx(s_mouse_hook); s_mouse_hook = NULL; }
}