#ifndef COMMON_H
#define COMMON_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define APP_NAME     L"Clavis"
#define APP_VERSION  L"1.0.0"
#define APP_CLASS    L"ClavisClass"
#define APP_MUTEX    L"ClavisSingleInstance"

#define WIN_W  400
#define WIN_H  630

#define IDI_APP              101
#define IDM_TRAYOPEN         1001
#define IDM_TRAYTOGGLE       1002
#define IDM_TRAYEXIT         1003
#define IDM_TRAYSTARTUP      1004
#define IDM_TRAYCLOSETOTRAY  1005
#define IDM_TRAYRELOAD       1006
#define IDM_TRAYOPENFOLDER   1007

#define WM_TRAY_MSG       (WM_USER + 1)
#define WM_HOOK_KEY       (WM_USER + 2)
#define WM_HOOK_MOUSE     (WM_USER + 3)
#define WM_HOTKEY_PRESSED (WM_USER + 4)

#define HOTKEY_TOGGLE 1

#define IDC_PACK_LIST     2001
#define IDC_PACK_RELOAD   2002
#define IDC_VOLUME_LABEL  2003
#define IDC_VOLUME_SLIDER 2004
#define IDC_MUTE_BTN      2005
#define IDC_OPEN_FOLDER   2007
#define IDC_KEYDOWN_CHK   2008
#define IDC_KEYUP_CHK     2009
#define IDC_MOUSE_CHK     2010
#define IDC_HOTKEY_BTN    2011
#define IDC_HOTKEY_LABEL  2012

#define MAX_PACK_NAME 128
#define MAX_SOUNDS    256
#define MAX_KEY_NAME  32
#define MAX_PACKS     64

typedef struct {
    BOOL dark;
    COLORREF bg, surface, surface_hover, border, border_input;
    COLORREF text, text_muted, text_faint;
    COLORREF accent, accent_fg;
    COLORREF success, destructive, muted_fg;
    COLORREF track, track_fill;
    HBRUSH br_bg, br_surface, br_surface_hover, br_border, br_accent;
    HBRUSH br_track, br_track_fill;
} Theme;

extern Theme g_theme;
void theme_init(void);
void theme_free(void);
void theme_apply_titlebar(HWND hwnd);

#endif