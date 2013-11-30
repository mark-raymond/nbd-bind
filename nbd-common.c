// nbd-common.c
// Copyright (c) 2013 Mark Raymond
// Released under the MIT license

#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>

const char * const dev = "/dev/";
// Stringification - see http://gcc.gnu.org/onlinedocs/cpp/Stringification.html
#define xstr(s) str(s)
#define str(s) #s
const char * const qemu_nbd_path = xstr(QEMUNBD);
const char * const qemu_nbd = "qemu-nbd";

// Returns true if the parent directory of path is writeable.
bool parent_dir_is_writeable(const char * const path) {
  
  char copy[strlen(path) + 1];
  strcpy(copy, path);
  return access(dirname(copy), W_OK) == 0;
  
}

// Returns true if file does not exist.
bool does_not_exist(char *file) {
  
  struct stat s;
  return (stat(file, &s) && errno == ENOENT);
  
}

// Returns true if file is an nbd device filename
// Equivalent to the regex nbd[0-9][0-9]?
bool is_nbd(const char *file) {

  return file[0] == 'n' &&
         file[1] == 'b' &&
         file[2] == 'd' &&
         file[3] >= '0' && file[3] <= '9' &&
         (file[4] == '\0' || (file[4] >= '0' && file[4] <= '9' && file[5] == '\0'));

}

// Returns true if str starts with pre
bool starts_with(const char *str, const char *pre) {
  
  while (*pre) {
    if (*str++ != *pre++) return false;
  }
  return true;
  
}

// Loops through /dev to find each partition of nbd.
// Calls callback func with the nbd device and the target path for that partition
bool each_nbd(const char *nbd, const char *tgt, bool (*func)(const char*, const char*, int)) {
  
  DIR *d = opendir(dev);
  if (d == NULL) return NULL;
  
  int nbd_len = strlen(nbd);
  int tgt_len = strlen(tgt);
  struct dirent *e;
  char *name;
  int name_len;
  while ((e = readdir(d)) != NULL) {
    name = e->d_name;
    name_len = strlen(name);
    if (starts_with(name, nbd) && (name[nbd_len] == '\0' || name[nbd_len] == 'p')) {
      char nbd_path[name_len + 6];
      char tgt_path[tgt_len + name_len - nbd_len + 2];
      strcpy(nbd_path, dev);
      strcat(nbd_path, name);
      strcpy(tgt_path, tgt);
      if (name[nbd_len] == 'p') strcat(tgt_path, "-");
      strcat(tgt_path, name + nbd_len);
      if (!func(nbd_path, tgt_path, name_len - nbd_len)) { closedir(d); return false; }
    }
  }
  
  closedir(d);
  return true;

}
