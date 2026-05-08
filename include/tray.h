#ifndef TRAY_H
#define TRAY_H

#include "common.h"

void tray_add(HWND hwnd, HICON icon);
void tray_remove(void);
void tray_update_tooltip(const wchar_t *text);
BOOL tray_show_menu(HWND hwnd, BOOL muted, BOOL start_with_win, BOOL close_to_tray);

#endif