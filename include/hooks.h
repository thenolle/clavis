#ifndef HOOKS_H
#define HOOKS_H

#include "common.h"

void hooks_install(HWND hwnd_notify);
void hooks_uninstall(void);

extern BOOL g_hook_keydown;
extern BOOL g_hook_keyup;
extern BOOL g_hook_mouse;
extern BOOL g_muted;

#endif