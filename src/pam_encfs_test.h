#ifndef PAM_ENCFS_H
#define PAM_ENCFS_H

/* Define which PAM interfaces we provide */
#define PAM_SM_ACCOUNT
#define PAM_SM_AUTH
#define PAM_SM_PASSWORD
#define PAM_SM_SESSION

/* Include PAM headers */
#include <security/pam_appl.h>
#include <security/pam_modules.h>

/* For struct passwd */
#include <pwd.h>

/* Syslog header */
#include <sys/syslog.h>

/* va_list */
#include <stdarg.h>

/*sleep*/
#include <unistd.h>

/*dirname and basename*/
#include <libgen.h>

/* wait */
#include <sys/types.h>
#include <sys/wait.h>

/* kill */
#include <signal.h>

#include "mount_encfs.h"
#include "utils.h"
#include "config.h"

#define ENCFS_PWD "encfs_password"
#define ENCFS_MNTP "encfs_mountpoint"
#define USER_PASSWD "user_password"
#define FUSE_GRP "fuse"

#define ENCFS5_CONF ".encfs5.xml"
#define ENCFS6_CONF ".encfs6.xml"
#define PAM_ENCFS_LOCAL_CONF ".pam_encfs"


static void _pam_log(int err, const char *format, ...);
int converse(pam_handle_t * pamh, int nargs, struct pam_message **message, struct pam_response **response);
int get_user_data(pam_handle_t *pamh, struct passwd_line **pwd);
void free_data_pointer(pam_handle_t *pamh, void *ptr, int error_status);
int set_encfs_pwd(pam_handle_t *pamh, const char *target);
int is_encfs_target(const char *target_path, const char *encfs_config_file);
int is_target_mountable(const char *target, const char *mountpoint);

int pam_umount_encfs_wrapper(const char *mountpoint);
int pam_mount_encfs_wrapper(pam_handle_t *pamh, const config_entry_t *config_entry_p,
        const passwd_line_t *user_passwd, const group_line_t *fuse_group, int nonempty);

#endif
