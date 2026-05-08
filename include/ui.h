#ifndef UI_H
#define UI_H

#include "common.h"
#include "soundpack.h"

typedef void (*PFN_PACK_SELECTED)(int index);
typedef void (*PFN_VOLUME_CHANGED)(int volume);

HWND ui_create(HINSTANCE hInst, int nCmdShow);
void ui_set_callbacks(PFN_PACK_SELECTED on_select, PFN_VOLUME_CHANGED on_volume);
void ui_populate_packs(const wchar_t names[][MAX_PACK_NAME], int count, int selected);
void ui_set_selected_idx(int idx);
void ui_set_status(const wchar_t *text, COLORREF color);
void ui_set_muted(BOOL muted);
int  ui_get_volume(void);
void ui_update_hotkey_label(void);
void ui_set_close_to_tray(BOOL enabled);

#endif