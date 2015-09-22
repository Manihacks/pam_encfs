#ifndef UTILS_H
#define UTILS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <error.h>
/* setuid and setgid */
#include <unistd.h>
/* setgroups */
#include <grp.h>
/* PATH_MAX */
#include <linux/limits.h>
#include <ctype.h>

#define MAX_STRING_LEN PATH_MAX
#define MAX_GROUP_MEMBERS 256
#define MAX_MOUNTPOINTS 32

struct member {
    char name[MAX_STRING_LEN];
};

typedef struct member member_t;

struct passwd_line {
   char user[MAX_STRING_LEN]; 
   char passwd[MAX_STRING_LEN]; 
   unsigned int uid;
   unsigned int gid;
   char uid_info[MAX_STRING_LEN];
   char home[MAX_STRING_LEN];
   char shell[MAX_STRING_LEN];
};

typedef struct passwd_line passwd_line_t;

struct group_line {
   char group[MAX_STRING_LEN]; 
   char passwd[MAX_STRING_LEN]; 
   unsigned int gid;
   size_t n_members;
   member_t members[MAX_GROUP_MEMBERS];
};

typedef struct group_line group_line_t;

struct mountpoint {
    char path[MAX_STRING_LEN];
};

typedef struct mountpoint mountpoint_t;

struct mounted_mountpoints{
    mountpoint_t mountpoints[MAX_MOUNTPOINTS];
    size_t n_mntpts;
};

typedef struct mounted_mountpoints mounted_mountpoints_t;

int isdir(const char *path);
int isfile(const char *path);
int drop_privileges(const uid_t uid, const gid_t gid, const gid_t groups[], const size_t s_groups);
int join_path(const char *path, const char *tojoin, char **joined);
int is_mountpoint_busy(const char *mountpoint);
void get_encfs_mounted(mounted_mountpoints_t *mounted_encfs);
size_t charocc(const void *data, const char nail, size_t s_data);
void replace(const void *data, const size_t s_data, const char old_char, const char new_char);
void get_passwd_struct(const char *username, struct passwd_line **passwd_data);
void get_group_struct( const char *group, group_line_t **group_data );
int is_user_in_group(const char *user, const group_line_t *group_data);
void strtrim(char *string);
#endif
