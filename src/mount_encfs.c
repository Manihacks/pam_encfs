#include "mount_encfs.h"
// for test
#include <fcntl.h>
#include <sys/stat.h>

int unmount_encfs(const char *target_path)
{
    int ret_code = 0;
    FILE *fusermount_proc = NULL;
    char *CMD_TPL = "fusermount -zu %s";
    size_t CMD_LEN = strlen(CMD_TPL) + strlen(target_path);
    char *cmd = malloc(CMD_LEN);
    snprintf(cmd, CMD_LEN, CMD_TPL, target_path);

    fusermount_proc = popen(cmd, "r");

    if ( fusermount_proc == NULL )
    {
        fprintf(stderr, "fusermount popen failed");
    }

    if ((ret_code = pclose(fusermount_proc) != 0))
    {
        fprintf(stderr, "fusermount command failed");
    }

    free(cmd);

    if (ret_code == 0)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

int mount_encfs(const char *target_path, const char *mountpoint, const char *password, int nonempty)
{
    char *CMD_TPL;
    int ret_code = 0;
    FILE *encfs_proc = NULL;
    // Needs 'user_allow_other' to be set in /etc/fuse.conf
    if ( ! nonempty )
        CMD_TPL = "encfs -S -o allow_root %s %s";
    else
        CMD_TPL = "encfs -S -o allow_root,nonempty %s %s";
    size_t CMD_LEN = strlen(CMD_TPL) + strlen(target_path) + strlen(mountpoint) + 1;
    char *cmd = calloc(1, CMD_LEN);
    snprintf(cmd, CMD_LEN, CMD_TPL, target_path, mountpoint);

    encfs_proc = popen(cmd, "w");

    if ( encfs_proc == NULL )
    {
        fprintf(stderr, "encfs popen failed");
    }

    fwrite(password, 1, strlen(password), encfs_proc);

    if ((ret_code = pclose(encfs_proc)) != 0)
    {
        fprintf(stderr, "encfs command failed");
    }

    free(cmd);

    if (ret_code == 0)
        return EXIT_SUCCESS;
    else
        return EXIT_FAILURE;
}

int main(int argc, char * argv[])
{
    char * cmd = argv[1];
    char * target_path = argv[2];

    if ( strcmp(cmd, "mount") == 0 )
    {
        char * mountpoint = argv[3];
        char * password = argv[4];
        printf("mounting\n");
        exit(mount_encfs(target_path, mountpoint, password, 0));
    }
    else if ( strcmp(cmd, "umount") == 0 )
    {
        printf("unmounting\n");
        exit(unmount_encfs(target_path));
    }
    return 0;
}
