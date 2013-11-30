// nbd-common.h
// Copyright (c) 2013 Mark Raymond
// Released under the MIT license

#include <stdbool.h>

extern const char * const dev;
extern const char * const qemu_nbd_path;
extern const char * const qemu_nbd;

bool parent_dir_is_writeable(const char * const path);
bool does_not_exist(char *file);
bool is_nbd(const char *file);
bool starts_with(const char *str, const char *pre);
bool each_nbd(const char *nbd, const char *tgt, bool (*func)(const char*, const char*, int));
