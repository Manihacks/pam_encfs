#include "utils.h"

int isdir(const char *path)
{
    struct stat file_stat = {0};

    if ( stat(path, &file_stat) != -1 );
    {
        if ( S_ISDIR(file_stat.st_mode) == 1 )
            return 1;
    }
    return 0;
}

int isfile(const char *path)
{
    struct stat file_stat = {0};

    if ( stat(path, &file_stat) != -1 );
    {
        if ( S_ISREG(file_stat.st_mode) == 1 )
            return 1;
    }
    return 0;
}

int drop_privileges(const uid_t uid, const gid_t gid, const gid_t groups[], const size_t s_groups)
{
    if ( setgid(gid) == -1 )
    {
        perror("guid failed");
        return EXIT_FAILURE;
    }

    // Set additional group to the process otherwise
    // we cannot use /dev/fuse on Debian for instance
    if ( s_groups > 0 && groups != NULL )
    {
        if ( setgroups(s_groups, groups) == -1 )
        {
            perror("setgroups failed");
            return EXIT_FAILURE;
        }
    }

    if ( setuid(uid) == -1 )
    {
        perror("suid failed");
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int join_path(const char *path, const char *tojoin, char **joined)
{
    const char *delim = "/";
    const size_t boundary = strlen(path) + strlen(tojoin) + 2;
    *joined = calloc(boundary, 1);
    if ( *joined == NULL )
        return EXIT_FAILURE;
    snprintf(*joined, boundary, "%s%s%s", path, delim, tojoin);
    //strcpy(*joined, path);
    //strcat(*joined, delim);
    //strcat(*joined, tojoin);
    return EXIT_SUCCESS;
}

int is_mountpoint_busy(const char *mountpoint)
{
    #define MTAB_SEP " "
    const char *mtab = "/etc/mtab";
    int isbusy = 0;
    size_t s_line = 0;
    FILE *smtab = fopen(mtab, "r");
    char *line = NULL;
    char *tmp_line = NULL;
    char dev[MAX_STRING_LEN] = {0};
    char mntp[MAX_STRING_LEN] = {0};

    if ( smtab == NULL )
        return isbusy = 1;

    while ( getline(&line, &s_line, smtab) != -1 )
    {
        tmp_line = line;
        memset(&dev, 0, MAX_STRING_LEN);
        memset(&mntp, 0, MAX_STRING_LEN);
        strncpy(dev, strsep(&tmp_line, MTAB_SEP), MAX_STRING_LEN - 1);
        strncpy(mntp, strsep(&tmp_line, MTAB_SEP), MAX_STRING_LEN - 1);

        if ( strcmp(mntp, mountpoint) == 0 )
        {
            isbusy = 1;
            break;
        }
    }

    if ( line != NULL )
        free(line);
    fclose(smtab);

    return isbusy;
}

void get_encfs_mounted(mounted_mountpoints_t *mounted_encfs)
{
    #define MTAB_SEP " "
    #define ENCFS_FSTYPE "fuse.encfs"
    const char *mtab = "/etc/mtab";
    size_t s_line = 0;
    FILE *smtab = fopen(mtab, "r");
    char *line = NULL;
    char *tmp_line = NULL;
    char dev[MAX_STRING_LEN], mntp[MAX_STRING_LEN], fstype[MAX_STRING_LEN];

    memset(mounted_encfs, 0, sizeof(mounted_mountpoints_t));

    if ( smtab == NULL )
        return;

    while ( getline(&line, &s_line, smtab) != -1 )
    {
        tmp_line = line;
        strncpy(dev, strsep(&tmp_line, MTAB_SEP), MAX_STRING_LEN - 1);
        strncpy(mntp, strsep(&tmp_line, MTAB_SEP), MAX_STRING_LEN - 1);
        strncpy(fstype, strsep(&tmp_line, MTAB_SEP), MAX_STRING_LEN - 1);

        if ( strcmp(fstype, ENCFS_FSTYPE) == 0 )
        {
            strncpy(mounted_encfs->mountpoints[mounted_encfs->n_mntpts].path, mntp, MAX_STRING_LEN - 1);
            mounted_encfs->n_mntpts++;
        }
    }

    if ( line != NULL )
        free(line);
    fclose(smtab);
}

void get_passwd_struct( const char *username, passwd_line_t **passwd_data )
{
    #define ETC_PASSWD "/etc/passwd"
    #define PASSWD_SEP_COUNT 6
    #define PASSWD_SEP ':'
    #define S_PASSWD_SEP ":"

    char *line = NULL;
    char *tmp_line = NULL;

    *passwd_data = calloc(1, sizeof(passwd_line_t));
    FILE *passwd_stream = fopen(ETC_PASSWD, "r");
    size_t s_line = 0;

    if ( passwd_stream == NULL )
    {
        return;
    }

    while (getline(&line, &s_line, passwd_stream) != -1)
    {
        // control that we cannot overflow the line
        if ( strlen(line) > sizeof(struct passwd_line) )
            continue;

        // control that we have the good number of separators in the line
        if ( charocc(line, PASSWD_SEP, s_line) != PASSWD_SEP_COUNT )
            continue;

        tmp_line = line;
        memset(*passwd_data, 0, sizeof(passwd_line_t));
        // -1 avoid overwritting the tailing end of string
        strncpy((*passwd_data)->user, strsep(&tmp_line, S_PASSWD_SEP),MAX_STRING_LEN - 1);
        strncpy((*passwd_data)->passwd, strsep(&tmp_line, S_PASSWD_SEP),MAX_STRING_LEN - 1);
        sscanf(strsep(&tmp_line, S_PASSWD_SEP), "%u", &(*passwd_data)->uid);
        sscanf(strsep(&tmp_line, S_PASSWD_SEP), "%u", &(*passwd_data)->gid);
        strncpy((*passwd_data)->uid_info, strsep(&tmp_line, S_PASSWD_SEP),MAX_STRING_LEN - 1);
        strncpy((*passwd_data)->home, strsep(&tmp_line, S_PASSWD_SEP),MAX_STRING_LEN - 1 );
        strncpy((*passwd_data)->shell, strsep(&tmp_line, S_PASSWD_SEP),MAX_STRING_LEN - 1);

        if ( strcmp(username, (*passwd_data)->user) == 0 )
            break;
    }

    if ( line != NULL )
        free(line);
}

void get_group_struct( const char *group, group_line_t **group_data )
{
    #define ETC_GROUP "/etc/group"
    #define GROUP_SEP_COUNT 3
    #define GROUP_SEP ':'
    #define S_GROUP_SEP ":"
    #define S_MEMBER_SEP ","

    char *line = NULL;
    char *tmp_line = NULL;

    FILE *group_stream = fopen(ETC_GROUP, "r");
    size_t s_line = 0;
    *group_data = calloc(1, sizeof(group_line_t));

    if ( group_stream == NULL )
    {
        return;
    }

    while (getline(&line, &s_line, group_stream) != -1)
    {
        //Strip line
        strtrim(line);
        memset(*group_data, 0, sizeof(group_line_t));
        // control that we cannot overflow the line
        if ( strlen(line) > sizeof(group_line_t) )
            continue;

        // control that we have the good number of separators in the line
        if ( charocc(line, GROUP_SEP, s_line) != GROUP_SEP_COUNT )
            continue;

        tmp_line = line;
        // -1 avoid overwritting the tailing end of string
        // ToDo: Use strncpy here
        strncpy((*group_data)->group, strsep(&tmp_line, S_GROUP_SEP),MAX_STRING_LEN - 1);
        strncpy((*group_data)->passwd, strsep(&tmp_line, S_GROUP_SEP),MAX_STRING_LEN - 1);
        sscanf(strsep(&tmp_line, S_GROUP_SEP), "%u", &(*group_data)->gid);
        while ( tmp_line != NULL )
        {
            strncpy(((*group_data)->members)[(*group_data)->n_members].name, strsep(&tmp_line, S_MEMBER_SEP), MAX_STRING_LEN - 1);
            (*group_data)->n_members++;
        }

        if ( strcmp(group, (*group_data)->group) == 0 )
            break;
    }

    if ( line != NULL )
        free(line);
}

int is_user_in_group(const char *user, const group_line_t *group_data)
{
    if ( group_data == NULL )
        return 0;

    for ( size_t i = 0; i < group_data->n_members; i++)
    {
        if ( strcmp(group_data->members[i].name, user) == 0 )
            return 1;
    }
    return 0;
}

/********************************************************************/
/************************** STRING UTILS ****************************/
/********************************************************************/

// Trim string in place
void strtrim(char *string)
{
    if ( string == NULL )
        return;
    
    size_t i = 0;
    size_t orig_len = strlen(string);
    char *begin = string;
    char *end = string + orig_len - 1;

    while ( isspace(*begin) )
        begin += 1;

    while ( isspace(*end) )
        end -= 1;

    for ( char *cp = begin; cp != end + 1; cp += 1 )
    {
        string[i] = *cp;
        i++;
    }

    while ( i < orig_len )
    {
        string[i] = '\0';
        i++;
    }
}

size_t charocc(const void *data, const char nail, size_t s_data)
{
    size_t counter = 0;
    const unsigned char *iterator = (unsigned char*) data;
    for ( size_t i = 0; i < s_data; i++ )
    {
        if ( iterator[i] == nail )
            counter++;
    }
    return counter;
}

void replace(const void *data, const size_t s_data, const char old_char, const char new_char)
{
    unsigned char *iterator = (unsigned char*) data;
    for ( size_t i = 0; i < s_data; i++ )
    {
        if ( iterator[i] == old_char )
            iterator[i] = new_char;
    }
}
