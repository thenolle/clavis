#ifndef HOTKEY_H
#define HOTKEY_H

#include "common.h"

#define HOTKEY_DEFAULT_VK   0x4D
#define HOTKEY_DEFAULT_MODS (MOD_CONTROL | MOD_SHIFT)

void hotkey_init(HWND hwnd_notify);
void hotkey_shutdown(void);

void hotkey_start_remap(void);
void hotkey_cancel_remap(void);
BOOL hotkey_is_remapping(void);

void hotkey_finish_remap(DWORD vk, UINT mods);

void hotkey_get_label(wchar_t *buf, int len);

extern DWORD g_hotkey_vk;
extern UINT  g_hotkey_mods;

#endif