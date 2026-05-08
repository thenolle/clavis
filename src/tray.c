#include "../include/tray.h"
#include "../include/hotkey.h"

static NOTIFYICONDATAW s_nid;

void tray_add(HWND hwnd, HICON icon) {
    memset(&s_nid, 0, sizeof(s_nid));
    s_nid.cbSize           = sizeof(s_nid);
    s_nid.hWnd             = hwnd;
    s_nid.uID              = 1;
    s_nid.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    s_nid.uCallbackMessage = WM_TRAY_MSG;
    s_nid.hIcon            = icon;
    wcsncpy(s_nid.szTip, APP_NAME L" \u2014 Active", 128);
    Shell_NotifyIconW(NIM_ADD, &s_nid);
}

void tray_remove(void) {
    Shell_NotifyIconW(NIM_DELETE, &s_nid);
}

void tray_update_tooltip(const wchar_t *text) {
    wcsncpy(s_nid.szTip, text, 128);
    s_nid.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &s_nid);
}

BOOL tray_show_menu(HWND hwnd, BOOL muted, BOOL start_with_win, BOOL close_to_tray) {
    wchar_t hk[64];
    hotkey_get_label(hk, 64);
    wchar_t toggle_label[192];
    swprintf(toggle_label, 192, L"%s (%s)", muted ? L"Unmute" : L"Mute", hk);

    HMENU m = CreatePopupMenu();
    AppendMenuW(m, MF_STRING,                                    IDM_TRAYOPEN,        L"Open Clavis");
    AppendMenuW(m, MF_STRING | (muted ? MF_CHECKED : 0),        IDM_TRAYTOGGLE,      toggle_label);
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING | (start_with_win ? MF_CHECKED : 0), IDM_TRAYSTARTUP,   L"Start with Windows");
    AppendMenuW(m, MF_STRING | (close_to_tray  ? MF_CHECKED : 0), IDM_TRAYCLOSETOTRAY, L"Close to tray");
    AppendMenuW(m, MF_SEPARATOR, 0, NULL);
    AppendMenuW(m, MF_STRING,                                    IDM_TRAYEXIT,        L"Exit");

    POINT pt;
    GetCursorPos(&pt);
    SetForegroundWindow(hwnd);
    UINT cmd = (UINT)TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(m);
    PostMessageW(hwnd, WM_NULL, 0, 0);

    if      (cmd == IDM_TRAYOPEN)       { ShowWindow(hwnd, IsWindowVisible(hwnd) ? SW_HIDE : SW_SHOW); if (IsWindowVisible(hwnd)) SetForegroundWindow(hwnd); }
    else if (cmd == IDM_TRAYTOGGLE)     PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(IDC_MUTE_BTN, BN_CLICKED), 0);
    else if (cmd == IDM_TRAYSTARTUP)    PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(IDM_TRAYSTARTUP, 0), 0);
    else if (cmd == IDM_TRAYCLOSETOTRAY)PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(IDM_TRAYCLOSETOTRAY, 0), 0);
    else if (cmd == IDM_TRAYEXIT)       return TRUE;
    return FALSE;
}