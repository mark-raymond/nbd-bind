// nbd-unbind.c
// Copyright (c) 2013 Mark Raymond
// Released under the MIT license

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "nbd-common.h"

// Check tgt_path is a symlink to nbd_path, then remove tgt_path.
// If any part fails, return true so that we still try to remove the remaining symlinks.
bool unsymlink(const char *nbd_path, const char *tgt_path, int extlen) {
  
  struct stat s;
  if (lstat(tgt_path, &s)) {
    fprintf(stderr, "lstat failed on file %s\n", tgt_path);
    return true;
  }
  if (!S_ISLNK(s.st_mode)) {
    fprintf(stderr, "%s is not a symbolic link\n", tgt_path);
    return true;
  }
  char link_path[11 + extlen]; // "/dev/nbd??" is 10 chars long
  ssize_t chars = readlink(tgt_path, link_path, 11 + extlen);
  if (chars < 0) {
    fprintf(stderr, "Could not read link %s\n", tgt_path);
    return true;
  }
  if (chars > 10 + extlen || (link_path[chars] = '\0', strcmp(link_path, nbd_path))) {
    fprintf(stderr, "%s does not point to %s\n", tgt_path, nbd_path);
    return true;
  }
  unlink(tgt_path);
  return true;
  
}

int main(int argc, char **argv) {
  
  if (argc != 2) {
    fprintf(stderr, "Usage: nbd-unbind target\n");
    return 1;
  }
  
  char *tgt = argv[1];
  
  // Check for read, write and delete permissions on target
  if (access(tgt, R_OK | W_OK) || !parent_dir_is_writeable(tgt)) {
    fprintf(stderr, "Insufficcient permissions for %s\n", tgt);
    return 1;
  }
  
  // Check target is a symbolic link to an nbd device
  struct stat s;
  if (lstat(tgt, &s)) {
    fprintf(stderr, "lstat failed on %s\n", tgt);
    return 1;
  }
  if (!S_ISLNK(s.st_mode)) {
    fprintf(stderr, "%s is not a symbolic link\n", tgt);
    return 1;
  }
  char nbd_path[11]; // "/dev/nbd??" is 10 chars long
  ssize_t chars = readlink(tgt, nbd_path, 11);
  if (chars < 0) {
    fprintf(stderr, "Could not read link %s\n", tgt);
    return 1;
  }
  if (chars > 10 || (nbd_path[chars] = '\0', !(starts_with(nbd_path, dev) && is_nbd(nbd_path + 5)))) {
    fprintf(stderr, "%s does not point to an nbd device\n", tgt);
    return 1;
  }
  
  // Check nbd device is owned by the calling user
  if (stat(nbd_path, &s)) {
    fprintf(stderr, "stat failed on file %s\n", nbd_path);
    return 1;
  }
  if (getuid() != 0 && s.st_uid != getuid()) {
    fprintf(stderr, "%s not owned by you\n", nbd_path);
    return 1;
  }
  
  char *nbd = nbd_path + 5; // strip /dev/ from front of nbd_path
  each_nbd(nbd, tgt, &unsymlink);
  int devnull = open("/dev/null", O_RDWR);
  dup2(devnull, STDIN_FILENO);
  dup2(devnull, STDOUT_FILENO);
  dup2(devnull, STDERR_FILENO);
  close(devnull);
  execl(qemu_nbd_path, qemu_nbd, "-d", nbd_path, (char*)NULL);
  return 1;
  
}
