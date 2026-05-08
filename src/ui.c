#include "../include/ui.h"
#include "../include/hooks.h"
#include "../include/hotkey.h"

static LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);

#define PAD          16
#define INNER_PAD    12
#define CARD_R       8
#define CHK_H        32
#define CHK_BOX      16
#define SLIDER_H     28
#define BTN_H        34
#define LIST_H       140
#define LABEL_H      18
#define STATUS_H     28
#define SECTION_GAP  14
#define ICON_BTN_W   80
#define ICON_BTN_H   24

static HWND    s_hwnd      = NULL;
static HWND    s_list      = NULL;
static PFN_PACK_SELECTED  s_on_select = NULL;
static PFN_VOLUME_CHANGED s_on_volume = NULL;

#define NUM_CHK 4
static BOOL     s_chk_vals[NUM_CHK];
static int      s_chk_hover    = -1;
static int      s_vol          = 80;
static BOOL     s_slider_drag  = FALSE;
static BOOL     s_slider_hover = FALSE;
static BOOL     s_muted        = FALSE;
static BOOL     s_mute_hover   = FALSE;
static int      s_btn_hover    = -1;
static wchar_t  s_status_text[512] = L"Select a soundpack to begin";
static COLORREF s_status_color     = (COLORREF)-1;
static HFONT    s_font_body  = NULL;
static HFONT    s_font_small = NULL;
static HFONT    s_font_bold  = NULL;
static int      s_sel_idx    = -1;

static RECT get_icon_btn(int idx) {
    int y = PAD + LABEL_H + 4 - ICON_BTN_H - 4;
    int x = WIN_W - PAD - ICON_BTN_W * 2 - 6 + idx * (ICON_BTN_W + 6);
    RECT r = { x, y, x + ICON_BTN_W, y + ICON_BTN_H };
    return r;
}

static RECT get_list_card(void) {
    int y = PAD + LABEL_H + 4;
    RECT r = { PAD, y, WIN_W - PAD, y + LIST_H };
    return r;
}

static RECT get_settings_card(void) {
    RECT lc = get_list_card();
    int y = lc.bottom + SECTION_GAP + LABEL_H + 4;
    int h = INNER_PAD + CHK_H * NUM_CHK + 6 + SLIDER_H + 6 + BTN_H + 6 + CHK_H + INNER_PAD;
    RECT r = { PAD, y, WIN_W - PAD, y + h };
    return r;
}

static RECT get_chk_rect(int idx) {
    RECT sc = get_settings_card();
    int y = sc.top + INNER_PAD + idx * CHK_H;
    RECT r = { sc.left + INNER_PAD, y, sc.right - INNER_PAD, y + CHK_H };
    return r;
}

static RECT get_slider_rect(void) {
    RECT sc = get_settings_card();
    int y = sc.top + INNER_PAD + CHK_H * NUM_CHK + 6;
    RECT r = { sc.left + INNER_PAD, y, sc.right - INNER_PAD, y + SLIDER_H };
    return r;
}

static RECT get_mute_btn_rect(void) {
    RECT sr = get_slider_rect();
    RECT sc = get_settings_card();
    int y = sr.bottom + 6;
    RECT r = { sc.left + INNER_PAD, y, sc.right - INNER_PAD, y + BTN_H };
    return r;
}

static RECT get_hotkey_rect(void) {
    RECT mb = get_mute_btn_rect();
    RECT sc = get_settings_card();
    int y = mb.bottom + 6;
    RECT r = { sc.left + INNER_PAD, y, sc.right - INNER_PAD, y + CHK_H };
    return r;
}

static RECT get_status_rect(void) {
    RECT sc = get_settings_card();
    RECT r = { PAD, sc.bottom + 10, WIN_W - PAD, sc.bottom + 10 + STATUS_H };
    return r;
}

static int vol_to_x(const RECT *sr, int vol) {
    return sr->left + 8 + (sr->right - sr->left - 16) * vol / 100;
}

static int x_to_vol(const RECT *sr, int mx) {
    int l = sr->left + 8, r = sr->right - 8;
    if (mx <= l) return 0;
    if (mx >= r) return 100;
    return (int)((mx - l) * 100LL / (r - l));
}

static int hit_checkbox(int mx, int my) {
    for (int i = 0; i < NUM_CHK; i++) {
        RECT r = get_chk_rect(i);
        if (mx >= r.left && mx < r.right && my >= r.top && my < r.bottom) return i;
    }
    return -1;
}

static BOOL hit_slider(int mx, int my) {
    RECT r = get_slider_rect(); r.top -= 6; r.bottom += 6;
    return mx >= r.left && mx < r.right && my >= r.top && my < r.bottom;
}

static BOOL hit_mute(int mx, int my) {
    RECT r = get_mute_btn_rect();
    return mx >= r.left && mx < r.right && my >= r.top && my < r.bottom;
}

static BOOL hit_hotkey_edit(int mx, int my) {
    RECT hr = get_hotkey_rect();
    RECT badge = { hr.right - 160, hr.top + 5, hr.right, hr.bottom - 5 };
    RECT er    = { badge.left - 42, hr.top, badge.left - 4, hr.bottom };
    return mx >= er.left && mx < er.right && my >= er.top && my < er.bottom;
}

static int hit_icon_btn(int mx, int my) {
    for (int i = 0; i < 2; i++) {
        RECT r = get_icon_btn(i);
        if (mx >= r.left && mx < r.right && my >= r.top && my < r.bottom) return i;
    }
    return -1;
}

static COLORREF blend(COLORREF a, COLORREF b, int pct) {
    return RGB((GetRValue(a)*(100-pct) + GetRValue(b)*pct)/100, (GetGValue(a)*(100-pct) + GetGValue(b)*pct)/100, (GetBValue(a)*(100-pct) + GetBValue(b)*pct)/100);
}

static void rounded_rect(HDC hdc, RECT *r, int radius, COLORREF fill, COLORREF stroke) {
    HBRUSH b = CreateSolidBrush(fill);
    HPEN   p = CreatePen(PS_SOLID, 1, stroke);
    HBRUSH ob = (HBRUSH)SelectObject(hdc, b);
    HPEN   op = (HPEN)  SelectObject(hdc, p);
    RoundRect(hdc, r->left, r->top, r->right, r->bottom, radius*2, radius*2);
    SelectObject(hdc, ob); SelectObject(hdc, op);
    DeleteObject(b); DeleteObject(p);
}

static void draw_list_item(DRAWITEMSTRUCT *di) {
    if (di->itemID == (UINT)LB_ERR) return;

    BOOL sel = (di->itemState & ODS_SELECTED) != 0;
    COLORREF bg = sel ? g_theme.accent : ((di->itemID & 1) == 0 ? g_theme.surface : blend(g_theme.surface, g_theme.surface_hover, 40));
    HBRUSH hbr = CreateSolidBrush(bg);
    FillRect(di->hDC, &di->rcItem, hbr);
    DeleteObject(hbr);

    wchar_t raw[MAX_PACK_NAME] = {0};
    SendMessageW(di->hwndItem, LB_GETTEXT, di->itemID, (LPARAM)raw);

    SetBkMode(di->hDC, TRANSPARENT);

    RECT tr = di->rcItem;
    tr.left  += 8;
    tr.right -= 4;

    if ((int)di->itemID == s_sel_idx) {
        RECT ck = { di->rcItem.left + 2, di->rcItem.top, di->rcItem.left + 14, di->rcItem.bottom };
        SetTextColor(di->hDC, sel ? g_theme.accent_fg : g_theme.accent);
        SelectObject(di->hDC, s_font_small);
        DrawTextW(di->hDC, L"\u2713", -1, &ck, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX);
        tr.left = di->rcItem.left + 16;
    }

    SetTextColor(di->hDC, sel ? g_theme.accent_fg : g_theme.text);
    SelectObject(di->hDC, s_font_body);
    DrawTextW(di->hDC, raw, -1, &tr, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_NOPREFIX | DT_END_ELLIPSIS);
}

static void on_paint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    RECT rc; GetClientRect(hwnd, &rc);
    HDC     mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, rc.right, rc.bottom);
    SelectObject(mem, bmp);

    FillRect(mem, &rc, g_theme.br_bg);
    SetBkMode(mem, TRANSPARENT);

    RECT lc  = get_list_card();
    RECT lbl = { lc.left, lc.top - LABEL_H - 4, lc.right, lc.top - 4 };
    SelectObject(mem, s_font_small);
    SetTextColor(mem, g_theme.text_muted);
    DrawTextW(mem, L"SOUNDPACK", -1, &lbl, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    for (int i = 0; i < 2; i++) {
        RECT r = get_icon_btn(i);
        COLORREF fill = (s_btn_hover == i) ? g_theme.surface_hover : g_theme.surface;
        rounded_rect(mem, &r, 4, fill, g_theme.border);
        SelectObject(mem, s_font_small);
        SetTextColor(mem, g_theme.text_muted);
        DrawTextW(mem, i == 0 ? L"\u21BA Reload" : L"\U0001F4C2 Folder", -1, &r, DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX);
    }

    rounded_rect(mem, &lc, CARD_R, g_theme.surface, g_theme.border);

    RECT sc   = get_settings_card();
    RECT slbl = { sc.left, sc.top - LABEL_H - 4, sc.right, sc.top - 4 };
    SelectObject(mem, s_font_small);
    SetTextColor(mem, g_theme.text_muted);
    DrawTextW(mem, L"SETTINGS", -1, &slbl, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
    rounded_rect(mem, &sc, CARD_R, g_theme.surface, g_theme.border);

    static const wchar_t *chk_labels[NUM_CHK] = {L"Key down sounds", L"Key up sounds", L"Mouse click sounds", L"Close to tray on \u00D7"};
    for (int i = 0; i < NUM_CHK; i++) {
        RECT cr = get_chk_rect(i);
        if (s_chk_hover == i) {
            HBRUSH hb = CreateSolidBrush(blend(g_theme.surface, g_theme.surface_hover, 60));
            FillRect(mem, &cr, hb); DeleteObject(hb);
        }
        RECT box = { cr.left, cr.top + (CHK_H-CHK_BOX)/2, cr.left + CHK_BOX, cr.top + (CHK_H-CHK_BOX)/2 + CHK_BOX };
        rounded_rect(mem, &box, 3, s_chk_vals[i] ? g_theme.accent : g_theme.bg, s_chk_vals[i] ? g_theme.accent : g_theme.border_input);
        if (s_chk_vals[i]) {
            HPEN pen = CreatePen(PS_SOLID, 2, g_theme.accent_fg);
            HPEN op  = (HPEN)SelectObject(mem, pen);
            int cx = box.left, cy = box.top + (CHK_BOX-10)/2;
            MoveToEx(mem, cx+3,  cy+5, NULL);
            LineTo  (mem, cx+6,  cy+8);
            LineTo  (mem, cx+12, cy+2);
            SelectObject(mem, op); DeleteObject(pen);
        }
        RECT tl = { box.right + 8, cr.top, cr.right, cr.bottom };
        SelectObject(mem, s_font_body);
        SetTextColor(mem, g_theme.text);
        DrawTextW(mem, chk_labels[i], -1, &tl, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
    }

    RECT sr = get_slider_rect();
    wchar_t vol_txt[32]; swprintf(vol_txt, 32, L"Volume  %d%%", s_vol);
    RECT vol_lbl = { sr.left, sr.top - LABEL_H, sr.right, sr.top };
    SelectObject(mem, s_font_small); SetTextColor(mem, g_theme.text_muted);
    DrawTextW(mem, vol_txt, -1, &vol_lbl, DT_SINGLELINE | DT_VCENTER | DT_LEFT);

    RECT track = { sr.left+4, sr.top+SLIDER_H/2-3, sr.right-4, sr.top+SLIDER_H/2+3 };
    rounded_rect(mem, &track, 3, g_theme.track, g_theme.track);
    int tx = vol_to_x(&sr, s_vol);
    RECT fill_track = { track.left, track.top, tx, track.bottom };
    rounded_rect(mem, &fill_track, 3, g_theme.track_fill, g_theme.track_fill);
    int tr2 = (s_slider_hover || s_slider_drag) ? 9 : 7;
    RECT thumb = { tx-tr2, sr.top+SLIDER_H/2-tr2, tx+tr2, sr.top+SLIDER_H/2+tr2 };
    rounded_rect(mem, &thumb, tr2, g_theme.accent, g_theme.accent);

    RECT mb = get_mute_btn_rect();
    COLORREF mf = s_muted ? (s_mute_hover ? blend(g_theme.destructive, g_theme.surface, 20) : g_theme.destructive) : (s_mute_hover ? blend(g_theme.accent,      g_theme.surface, 20) : g_theme.accent);
    rounded_rect(mem, &mb, CARD_R, mf, mf);
    SelectObject(mem, s_font_bold); SetTextColor(mem, g_theme.accent_fg);
    DrawTextW(mem, s_muted ? L"Unmute" : L"Active - click to mute", -1, &mb, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

    RECT hr = get_hotkey_rect();
    wchar_t hk[128]; hotkey_get_label(hk, 128);
    RECT badge  = { hr.right-160, hr.top+5,        hr.right,      hr.bottom-5 };
    RECT edit_r = { badge.left-42, hr.top,          badge.left-4,  hr.bottom   };
    RECT txt_r  = { hr.left,       hr.top,          edit_r.left-4, hr.bottom   };

    SelectObject(mem, s_font_body); SetTextColor(mem, g_theme.text);
    DrawTextW(mem, L"Toggle hotkey", -1, &txt_r, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
    rounded_rect(mem, &edit_r, 4, g_theme.surface_hover, g_theme.border);
    SetTextColor(mem, g_theme.text_muted); SelectObject(mem, s_font_small);
    DrawTextW(mem, hotkey_is_remapping() ? L"cancel" : L"edit", -1, &edit_r, DT_SINGLELINE | DT_VCENTER | DT_CENTER);
    rounded_rect(mem, &badge, 4, g_theme.surface_hover, g_theme.border);
    SetTextColor(mem, g_theme.accent); SelectObject(mem, s_font_small);
    DrawTextW(mem, hk, -1, &badge, DT_SINGLELINE | DT_VCENTER | DT_CENTER);

    RECT str2 = get_status_rect();
    SetTextColor(mem, s_status_color == (COLORREF)-1 ? g_theme.text_muted : s_status_color);
    SelectObject(mem, s_font_small);
    DrawTextW(mem, s_status_text, -1, &str2, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_END_ELLIPSIS);

    BitBlt(hdc, 0, 0, rc.right, rc.bottom, mem, 0, 0, SRCCOPY);
    DeleteObject(bmp); DeleteDC(mem);
    EndPaint(hwnd, &ps);
}

static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT:      on_paint(hwnd); return 0;
    case WM_ERASEBKGND: return 1;

    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *di = (DRAWITEMSTRUCT *)lp;
        if (di->CtlID == IDC_PACK_LIST) { draw_list_item(di); return TRUE; }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT *mi = (MEASUREITEMSTRUCT *)lp;
        if (mi->CtlID == IDC_PACK_LIST) { mi->itemHeight = 26; return TRUE; }
        return DefWindowProcW(hwnd, msg, wp, lp);
    }

    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wp;
        SetBkColor(hdc, g_theme.surface);
        SetTextColor(hdc, g_theme.text);
        return (LRESULT)g_theme.br_surface;
    }

    case WM_MOUSEMOVE: {
        int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
        TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        int  ph = s_chk_hover; BOOL pm = s_mute_hover, ps2 = s_slider_hover; int pb = s_btn_hover;
        s_chk_hover    = hit_checkbox(mx, my);
        s_mute_hover   = hit_mute(mx, my);
        s_slider_hover = hit_slider(mx, my);
        s_btn_hover    = hit_icon_btn(mx, my);
        if (s_slider_drag) {
            RECT sr = get_slider_rect();
            int nv = x_to_vol(&sr, mx);
            if (nv != s_vol) { s_vol = nv; if (s_on_volume) s_on_volume(s_vol); }
        }
        if (s_chk_hover != ph || s_mute_hover != pm || s_slider_hover != ps2 || s_slider_drag || s_btn_hover != pb)
            InvalidateRect(hwnd, NULL, FALSE);
        SetCursor(LoadCursorW(NULL, (s_slider_hover || s_slider_drag || hit_hotkey_edit(mx, my) || s_chk_hover >= 0 || s_mute_hover || s_btn_hover >= 0) ? IDC_HAND : IDC_ARROW));
        return 0;
    }

    case WM_MOUSELEAVE:
        s_chk_hover = -1; s_mute_hover = FALSE;
        s_slider_hover = FALSE; s_btn_hover = -1;
        if (!s_slider_drag) InvalidateRect(hwnd, NULL, FALSE);
        return 0;

    case WM_LBUTTONDOWN: {
        int mx = GET_X_LPARAM(lp), my = GET_Y_LPARAM(lp);
        int bi = hit_icon_btn(mx, my);
        if (bi == 0) { PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(IDC_PACK_RELOAD, BN_CLICKED), 0); return 0; }
        if (bi == 1) { PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(IDC_OPEN_FOLDER, BN_CLICKED), 0); return 0; }
        int ci = hit_checkbox(mx, my);
        if (ci >= 0) {
            s_chk_vals[ci] = !s_chk_vals[ci];
            switch (ci) {
              case 0: g_hook_keydown = s_chk_vals[0]; break;
              case 1: g_hook_keyup   = s_chk_vals[1]; break;
              case 2: g_hook_mouse   = s_chk_vals[2]; break;
              case 3: PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(IDM_TRAYCLOSETOTRAY, 0), 0); break;
            }
            InvalidateRect(hwnd, NULL, FALSE); return 0;
        }
        if (hit_mute(mx, my))       { PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(IDC_MUTE_BTN,   BN_CLICKED), 0); return 0; }
        if (hit_hotkey_edit(mx, my)){ PostMessageW(hwnd, WM_COMMAND, MAKEWPARAM(IDC_HOTKEY_BTN, BN_CLICKED), 0); return 0; }
        if (hit_slider(mx, my)) {
            SetCapture(hwnd); s_slider_drag = TRUE;
            RECT sr = get_slider_rect();
            int nv = x_to_vol(&sr, mx);
            if (nv != s_vol) { s_vol = nv; if (s_on_volume) s_on_volume(s_vol); }
            InvalidateRect(hwnd, NULL, FALSE); return 0;
        }
        return 0;
    }

    case WM_LBUTTONUP:
        if (s_slider_drag) { s_slider_drag = FALSE; ReleaseCapture(); InvalidateRect(hwnd, NULL, FALSE); }
        return 0;

    case WM_CAPTURECHANGED:
        if (s_slider_drag) { s_slider_drag = FALSE; InvalidateRect(hwnd, NULL, FALSE); }
        return 0;

    case WM_COMMAND:
        if (LOWORD(wp) == IDC_PACK_LIST && HIWORD(wp) == LBN_SELCHANGE) {
            int idx = (int)SendMessageW(s_list, LB_GETCURSEL, 0, 0);
            if (idx != LB_ERR && s_on_select) s_on_select(idx);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wp, lp);

    case WM_SIZE:
        if (s_list) {
            RECT lc = get_list_card();
            SetWindowPos(s_list, NULL, lc.left+1, lc.top+1, lc.right-lc.left-2, lc.bottom-lc.top-2, SWP_NOZORDER);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;

    case WM_CLOSE:   return DefWindowProcW(hwnd, msg, wp, lp);
    case WM_DESTROY:
        if (s_font_body)  DeleteObject(s_font_body);
        if (s_font_small) DeleteObject(s_font_small);
        if (s_font_bold)  DeleteObject(s_font_bold);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

HWND ui_create(HINSTANCE hInst, int nCmdShow) {
    s_chk_vals[0] = TRUE; s_chk_vals[1] = FALSE;
    s_chk_vals[2] = FALSE; s_chk_vals[3] = TRUE;
    g_hook_keydown = TRUE; g_hook_keyup = FALSE; g_hook_mouse = FALSE;

    s_font_body  = CreateFontW(-13,0,0,0,FW_NORMAL,  0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    s_font_small = CreateFontW(-11,0,0,0,FW_NORMAL,  0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    s_font_bold  = CreateFontW(-13,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");

    WNDCLASSEXW wc = {0};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = g_theme.br_bg;
    wc.lpszClassName = APP_CLASS;
    wc.hIcon         = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_APP));
    RegisterClassExW(&wc);

    RECT wr = {0,0,WIN_W,WIN_H};
    AdjustWindowRectEx(&wr, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX, FALSE, 0);
    s_hwnd = CreateWindowExW(0, APP_CLASS, APP_NAME, WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, wr.right-wr.left, wr.bottom-wr.top, NULL, NULL, hInst, NULL);
    if (!s_hwnd) return NULL;

    theme_apply_titlebar(s_hwnd);

    RECT lc = get_list_card();
    s_list = CreateWindowExW(0, L"LISTBOX", NULL, WS_CHILD|WS_VISIBLE|LBS_NOTIFY|LBS_OWNERDRAWFIXED|LBS_HASSTRINGS|LBS_NOINTEGRALHEIGHT|WS_VSCROLL, lc.left+1, lc.top+1, lc.right-lc.left-2, lc.bottom-lc.top-2, s_hwnd, (HMENU)(UINT_PTR)IDC_PACK_LIST, hInst, NULL);

    ShowWindow(s_hwnd, nCmdShow);
    UpdateWindow(s_hwnd);
    return s_hwnd;
}

void ui_set_callbacks(PFN_PACK_SELECTED a, PFN_VOLUME_CHANGED b) { s_on_select = a; s_on_volume = b; }

void ui_populate_packs(const wchar_t names[][MAX_PACK_NAME], int count, int sel) {
    if (!s_list) return;
    s_sel_idx = sel;
    SendMessageW(s_list, LB_RESETCONTENT, 0, 0);
    for (int i = 0; i < count; i++)
        SendMessageW(s_list, LB_ADDSTRING, 0, (LPARAM)names[i]);
    if (sel >= 0 && sel < count)
        SendMessageW(s_list, LB_SETCURSEL, (WPARAM)sel, 0);
}

void ui_set_selected_idx(int idx) {
    s_sel_idx = idx;
    if (s_list) { SendMessageW(s_list, LB_SETCURSEL, (WPARAM)idx, 0); InvalidateRect(s_list, NULL, FALSE); }
}

void ui_set_status(const wchar_t *text, COLORREF color) {
    wcsncpy(s_status_text, text, 511); s_status_text[511] = L'\0';
    s_status_color = (color == 0) ? (COLORREF)-1 : color;
    if (s_hwnd) InvalidateRect(s_hwnd, NULL, FALSE);
}

void ui_set_muted(BOOL muted)       { s_muted = muted;    if (s_hwnd) InvalidateRect(s_hwnd, NULL, FALSE); }
int  ui_get_volume(void)            { return s_vol; }
void ui_update_hotkey_label(void)   { if (s_hwnd) InvalidateRect(s_hwnd, NULL, FALSE); }
void ui_set_close_to_tray(BOOL en)  { s_chk_vals[3] = en; if (s_hwnd) InvalidateRect(s_hwnd, NULL, FALSE); }