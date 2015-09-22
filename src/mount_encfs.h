#ifndef MOUNT_ENCFS_H
#define MOUNT_ENCFS_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <error.h>

int unmount_encfs(const char *target_path);
int mount_encfs(const char *target_path, const char *mountpoint, const char *password, int nonempty);

#endif
