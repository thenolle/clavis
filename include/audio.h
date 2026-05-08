#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"

void audio_init(void);
void audio_shutdown(void);
void audio_play(const wchar_t *filepath, int volume_pct);
void audio_play_mem(const unsigned char *data, unsigned int size, int volume_pct);
void audio_set_volume(int pct);
int  audio_get_volume(void);

#endif