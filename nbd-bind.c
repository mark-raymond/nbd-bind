// nbd-bind.c
// Copyright (c) 2013 Mark Raymond
// Released under the MIT license

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>

#include "nbd-common.h"

// Tries to bind img to the nbd device nbd
bool try_nbd(const char *nbd, const char *img) {
  
  char nbd_path[strlen(nbd) + 6];
  pid_t pid;
  int status;
  
  strcpy(nbd_path, dev);
  strcat(nbd_path, nbd);
  
  pid = fork();
  
  if (pid < 0) {
    // fork failed
    return false;
  } else if (pid == 0) {
    // child process
    int devnull = open("/dev/null", O_RDWR);
    dup2(devnull, STDIN_FILENO);
    dup2(devnull, STDOUT_FILENO);
    dup2(devnull, STDERR_FILENO);
    close(devnull);
    execl(qemu_nbd_path, qemu_nbd, "-c", nbd_path, img, (char*)NULL);
    _exit(1);
  } else {
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
  }
  
}

// Binds img to an nbd device, returning the name of the nbd device
char *bind_nbd(const char *img) {
  
  DIR *d = opendir(dev);
  if (d == NULL) return NULL;
  
  struct dirent *e;
  char *name;
  bool r;
  while ((e = readdir(d)) != NULL) {
    name = e->d_name;
    if (is_nbd(name)) {
      r = try_nbd(name, img);
      if (r) {
        char *n = malloc((strlen(name) + 1) * sizeof(char));
        strcpy(n, name);
        closedir(d);
        return n;
      }
    }
  }
  
  closedir(d);
  return NULL;
  
}

// Change owner of nbd_path to the calling user,
// and create a symlink at tgt_path to nbd_path.
bool set_permissions_and_link(const char *nbd_path, const char *tgt_path, int extlen) {
  
  struct stat s;
  if (stat(nbd_path, &s)) {
    fprintf(stderr, "stat failed on file %s\n", nbd_path);
    return false;
  }
  if (chown(nbd_path, getuid(), s.st_gid)) {
    fprintf(stderr, "chown failed on file %s\n", nbd_path);
    return false;
  }
  if (symlink(nbd_path, tgt_path)) {
    fprintf(stderr, "symlink failed %s -> %s\n", tgt_path, nbd_path);
    return false;
  }
  if (lchown(tgt_path, getuid(), getgid())) {
    fprintf(stderr, "lchown failed on file %s\n", tgt_path);
    return false;
  }
  return true;
      
}

int main(int argc, char **argv) {
  
  if (argc != 3) {
    fprintf(stderr, "Usage: nbd-bind image target\n");
    return 1;
  }
  
  char *img = argv[1];
  char *tgt = argv[2];
  
  // Check permissions to read and write image file
  if (access(img, R_OK | W_OK)) {
    fprintf(stderr, "Insufficcient permissions for %s\n", img);
    return 1;
  }
  
  // Check target does not exist, and we have permission to create it
  if (!parent_dir_is_writeable(tgt)) {
    fprintf(stderr, "Insufficcient permissions to create %s\n", tgt);
    return 1;
  }
  if (!does_not_exist(tgt)) {
    fprintf(stderr, "%s already exists\n", tgt);
    return 1;
  }

  // Bind the image to an nbd device
  char *nbd = bind_nbd(img);
  if (nbd == NULL) {
    fprintf(stderr, "Unable to bind nbd\n");
    return 1;
  }
  
  // Give nbd a moment to create the devices.
  // This is a hack, I know...
  usleep(50000);
  
  // Set permissions on the nbd devices,
  // and symlink them to target
  bool r = each_nbd(nbd, tgt, &set_permissions_and_link);
  free(nbd);
  if (r) {
    return 0;
  } else {
    fprintf(stderr, "Unable to set permissions and link\n");
    return 1;
  }

}
