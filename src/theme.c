#include "../include/common.h"
#include <dwmapi.h>

Theme g_theme;

static BOOL system_is_dark(void)
{
    DWORD val = 1, size = sizeof(val);
    RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", L"AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &val, &size);
    return (val == 0);
}

static COLORREF mk(int r, int g, int b) { return RGB(r,g,b); }
static HBRUSH   br(COLORREF c)          { return CreateSolidBrush(c); }

void theme_init(void)
{
    Theme *t = &g_theme;
    t->dark = system_is_dark();

    if (t->dark) {
        t->bg             = mk( 9,  9, 11);
        t->surface        = mk(24, 24, 27);
        t->surface_hover  = mk(39, 39, 42);
        t->border         = mk(39, 39, 42);
        t->border_input   = mk(63, 63, 70);
        t->text           = mk(250,250,250);
        t->text_muted     = mk(161,161,170);
        t->text_faint     = mk( 82, 82, 91);
        t->accent         = mk(250,250,250);
        t->accent_fg      = mk(  9,  9, 11);
        t->success        = mk( 34,197, 94);
        t->destructive    = mk(239, 68, 68);
        t->muted_fg       = mk(161,161,170);
        t->track          = mk( 39, 39, 42);
        t->track_fill     = mk(250,250,250);
    } else {
        t->bg             = mk(255,255,255);
        t->surface        = mk(250,250,250);
        t->surface_hover  = mk(244,244,245);
        t->border         = mk(228,228,231);
        t->border_input   = mk(212,212,216);
        t->text           = mk(  9,  9, 11);
        t->text_muted     = mk(113,113,122);
        t->text_faint     = mk(161,161,170);
        t->accent         = mk(  9,  9, 11);
        t->accent_fg      = mk(250,250,250);
        t->success        = mk( 22,163, 74);
        t->destructive    = mk(220, 38, 38);
        t->muted_fg       = mk(113,113,122);
        t->track          = mk(228,228,231);
        t->track_fill     = mk(  9,  9, 11);
    }

    t->br_bg            = br(t->bg);
    t->br_surface       = br(t->surface);
    t->br_surface_hover = br(t->surface_hover);
    t->br_border        = br(t->border);
    t->br_accent        = br(t->accent);
    t->br_track         = br(t->track);
    t->br_track_fill    = br(t->track_fill);
}

void theme_free(void)
{
    Theme *t = &g_theme;
    DeleteObject(t->br_bg);
    DeleteObject(t->br_surface);
    DeleteObject(t->br_surface_hover);
    DeleteObject(t->br_border);
    DeleteObject(t->br_accent);
    DeleteObject(t->br_track);
    DeleteObject(t->br_track_fill);
}

void theme_apply_titlebar(HWND hwnd)
{
    BOOL dark = g_theme.dark;
    if (FAILED(DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark))))
        DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
}