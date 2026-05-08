#ifndef SOUNDPACK_H
#define SOUNDPACK_H

#include "common.h"

typedef struct {
    const char          *filename;
    const unsigned char *data;
    unsigned int         size;
} EmbeddedFile;

typedef struct {
    const char         *dir_name;
    const EmbeddedFile *files;
    int                 file_count;
} EmbeddedPack;

extern const EmbeddedPack g_embedded_packs[];
extern const int          g_embedded_pack_count;

typedef struct {
    char    key_name[MAX_KEY_NAME];
    wchar_t sound_file[MAX_PATH];
    const unsigned char *emb_data;
    unsigned int         emb_size;
} KeySound;

typedef struct {
    wchar_t  name[MAX_PACK_NAME];
    wchar_t  dir[MAX_PATH];
    wchar_t  icon[MAX_PATH];

    KeySound sounds[MAX_SOUNDS];
    int      sound_count;

    KeySound up_sounds[MAX_SOUNDS];
    int      up_sound_count;

    wchar_t             mouse_down_path[MAX_PATH];
    const unsigned char *mouse_down_data;
    unsigned int         mouse_down_size;

    wchar_t             mouse_up_path[MAX_PATH];
    const unsigned char *mouse_up_data;
    unsigned int         mouse_up_size;

    BOOL is_embedded;
} SoundPack;

int  sp_discover(const wchar_t *root, wchar_t names[][MAX_PACK_NAME], wchar_t dirs[][MAX_PATH], int max_packs);

BOOL sp_load(const wchar_t *dir, SoundPack *out);

BOOL sp_load_embedded(int embedded_index, SoundPack *out);

void sp_get_sound(const SoundPack *pack, DWORD vk_code, BOOL key_up, const wchar_t **out_path, const unsigned char **out_data, unsigned int *out_size);
void sp_unload(SoundPack *pack);

#endif