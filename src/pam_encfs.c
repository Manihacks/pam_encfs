#include "pam_encfs.h"

/* Source: https://code.google.com/p/pam-encfs/source/browse/trunk/pam_encfs.c */
/* function for correct syslog logging 
   provided by Scipio <scipio@freemail.hu> */
static void _pam_log(int err, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    openlog("pam_encfs", LOG_CONS | LOG_PID, LOG_AUTH);
    vsyslog(err, format, args);
    va_end(args);
    closelog();
}

/* Source: https://code.google.com/p/pam-encfs/source/browse/trunk/pam_encfs.c */
/* this function ripped from pam_unix/support.c */
int converse(pam_handle_t * pamh, int nargs, struct pam_message **message, struct pam_response **response)
{
    int retval;
    struct pam_conv *conv;

    retval = pam_get_item(pamh, PAM_CONV, (const void **) &conv);
    if (retval == PAM_SUCCESS)
    {
        retval = conv->conv(nargs,
                (const struct pam_message **) message,
                response, conv->appdata_ptr);
    }
    return retval;
}

int get_user_passwd(pam_handle_t * pamh, struct passwd_line **pwd)
{
    /* Source: https://code.google.com/p/pam-encfs/source/browse/trunk/pam_encfs.c */
    /* Don't know why the receipe found there didn't work, so I did my own parsing /etc/passwd */
    /* pam_get_env didn't returned anything for HOME */
    //struct passwd *pwd = NULL;    
    int rc;
    const char *user = NULL;

    rc = pam_get_user(pamh, &user, NULL);
    if ((rc != PAM_SUCCESS) || (!user))
    {
        _pam_log(LOG_ERR, "Can't get username: %s", pam_strerror(pamh, rc));
        return rc;
    }

    get_passwd_struct(user, pwd);
    return rc;

}

/* Function used to free out string data stored in pam
 * string is memset()ed to 0 before freeing to prevent password
 * leaks */
void free_string_pointer(pam_handle_t *pamh, void *ptr, int error_status)
{
    if ( ptr != NULL )
    {
        // memset the memory so that we don't leak data/password in memory
        memset(ptr, 0, strlen((char *) ptr));
        free(ptr);
    }
}

/* Function used to free out arbitrary allocated data */
void free_data_pointer(pam_handle_t *pamh, void *ptr, int error_status)
{
    if ( ptr != NULL )
    {
        free(ptr);
    }
}

/* Function amended from source: https://code.google.com/p/pam-encfs/source/browse/trunk/pam_encfs.c */
/* this function ripped from pam_unix/support.c */
int set_encfs_pwd(pam_handle_t *pamh, const char *target)
{
    int retval;
    char *p;


    const char *baseprompt = "Encfs password to decrypt %s: ";
    size_t s_message = strlen(target) + strlen(baseprompt) + 1;
    char *message = calloc(1, s_message);

    if ( message == NULL )
    {
        _pam_log(LOG_ERR, "%s", "Cannot allocate message buffer");
        return PAM_BUF_ERR;
    }

    snprintf(message, s_message, baseprompt, target);

    struct pam_message msg[1], *pmsg[1];
    struct pam_response *resp;

    /* set up conversation call */

    pmsg[0] = &msg[0];
    msg[0].msg_style = PAM_PROMPT_ECHO_OFF;
    msg[0].msg = message;
    resp = NULL;

    if ((retval = converse(pamh, 1, pmsg, &resp)) != PAM_SUCCESS)
    {
        // We go here
        _pam_log(LOG_ERR,"Error in converse: %s", pam_strerror(pamh,retval));
        free(message);
        return retval;
    }

    if (resp)
    {
        if ( resp[0].resp == NULL )
        {
            free(resp);
            free(message);
            return PAM_AUTH_ERR;
        }

        p = resp[0].resp;

        /* This could be a memory leak. If resp[0].resp 
           is malloc()ed, then it has to be free()ed! 
           -- alex 
           */

        resp[0].resp = NULL;
    }

    else
    {
        _pam_log(LOG_ERR,"%s", "resp is NULL");
        free(message);
        return PAM_CONV_ERR;
    }

    free(resp);
    free(message);
    pam_set_data(pamh, ENCFS_PWD, strdup(p), free_string_pointer);
    return PAM_SUCCESS;
}

//Checked
int is_encfs_target(const char *target_path, const char *encfs_config_file)
{
    int retval = 1;
    char *encfs_config_path = NULL;
    join_path(target_path, encfs_config_file, &encfs_config_path);

    // target_path is a valid directory
    if ( isdir(target_path) == 0 )
        retval = 0;
    
    // Typical .encfsX.xml file exists under target
    if ( isfile(encfs_config_path) == 0 )
        retval = 0;

    if ( encfs_config_path != NULL )
        free(encfs_config_path);

    return retval;
}

//Checked
int is_target_mountable(const char *target, const char *mountpoint)
{
    int retval = 1;

    // Check if mountpoint already busy -> /etc/mtab
    if ( is_mountpoint_busy(mountpoint) == 1 )
    {
        retval = 0;
        _pam_log(LOG_ERR,"Mountpoint: %s is already busy", mountpoint);
    }

    // Check if target is likely to be a encfs target
    if ( 
            is_encfs_target(target, ENCFS5_CONF) == 0 &&
            is_encfs_target(target, ENCFS6_CONF) == 0
            )
    {
        retval = 0;
        _pam_log(LOG_ERR,"Target: %s is not a good encfs target", target);
    }
    return retval;
}

/* Simple wrapper around unmount_encfs */
int pam_unmount_encfs_wrapper(const char *mountpoint)
{
    if ( unmount_encfs(mountpoint) != EXIT_SUCCESS )
        return PAM_SYSTEM_ERR;
    return PAM_SUCCESS;
}

/* Wait for the child process to terminate */
int waitpid_timeout(const pid_t pid)
{
    int status;
    useconds_t time = 0;
    useconds_t step = 500000;
    useconds_t timeout = 5000000;
    while ( time < timeout )
    {   
        if (pid != waitpid(-1, &status, WNOHANG))
            continue;

        if ( WIFEXITED(status) )
            break;

        usleep(step);
        time += step;
    }
    return status;
}

/* Wrapper that mount a encfs directory according to a config_entry
 * We take care of setting appropriate user rights before doing it */
int pam_mount_encfs_wrapper(pam_handle_t *pamh, const config_entry_t *config_entry_p, 
        const passwd_line_t *user_passwd,
        const group_line_t *fuse_group,
        int nonempty)
{
    pid_t pid;
    int status, rc;
    gid_t groups[1] = {1024};

    // Do not free because used to pass data over PAM
    char *encfs_pwd = NULL;

    // Check if target and mountpoint are valid
    if ( is_target_mountable(config_entry_p->target, config_entry_p->mountpoint) == 0 )
    {
        _pam_log(LOG_ERR,"Not mountable target: %s mountpoint: %s", config_entry_p->target, config_entry_p->mountpoint);

        return EXIT_FAILURE;
    }

    // Set the encfs password required to mount the target
    if ( (rc = set_encfs_pwd(pamh, config_entry_p->target)) != PAM_SUCCESS )
    {
        _pam_log(LOG_ERR, "Unable to set encfs password: %s", pam_strerror(pamh, rc));

        return EXIT_FAILURE;
    }

    // Retrieves the ENCFS_PWD previously set
    if ( pam_get_data(pamh, ENCFS_PWD, (const void **) &encfs_pwd) != PAM_SUCCESS )
    {
        _pam_log(LOG_ERR, "%s", "Unable to get encfs password");

        return EXIT_FAILURE;
    }

    // Need to fork to drop privileges
    switch (pid = fork())
    {
        case -1:
            //Fork failed
            _pam_log(LOG_ERR, "%s", "Failed to fork");

            return EXIT_FAILURE;

        case 0:
            // If the current user is member of fuse group
            if ( is_user_in_group(user_passwd->user, fuse_group) )
            {
                // Drops privileges and add fuse group capabilities
                groups[0] = fuse_group->gid;
                rc = drop_privileges(user_passwd->uid, user_passwd->gid, groups, 1);
            }

            else
            {
                // Drops privilege without additional group rights
                rc = drop_privileges(user_passwd->uid, user_passwd->gid, NULL, 0);
            }

            if ( rc != EXIT_SUCCESS )
            {
                _pam_log(LOG_ERR, "%s", "Cannot drop child privileges");
                exit(1);
            }

            if ( mount_encfs(config_entry_p->target, config_entry_p->mountpoint, encfs_pwd, nonempty) != EXIT_SUCCESS )
            {
                _pam_log(LOG_ERR, "Error while mounting encfs target: %s mountpoint: %s", config_entry_p->target, config_entry_p->mountpoint);
                exit(1);
            }

            exit(0);
    }

    status = waitpid_timeout(pid);

    if ( ! WIFEXITED(status) )
    {
        _pam_log(LOG_ERR, "%s", "Mounting process failed");
        kill(pid, SIGTERM); 

        return EXIT_FAILURE;
    }

    // Can safely check status since child exited
    if ( WEXITSTATUS(status) != 0 )
    {
        _pam_log(LOG_ERR, "%s", "Mounting child exited with error(s)");

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

/* Create a default config_entry that holds data to mount user HOME 
 * config_entry should be freed even if the function fails */
int default_config_entry(const char *home, config_entry_t **config_entry)
{
    char *tmp_home_dname = NULL;
    char *tmp_home_bname = NULL;
    char *dname = NULL;
    char *bname = NULL;

    if ( *config_entry == NULL )
    {
        *config_entry = calloc(sizeof(config_entry_t), 1);
    }
    else
    {
        free(*config_entry);
        *config_entry = calloc(sizeof(config_entry_t), 1);
    }

    tmp_home_dname = strdup(home);
    tmp_home_bname = strdup(home);
    if ( tmp_home_dname == NULL || tmp_home_bname == NULL )
    {
        return EXIT_FAILURE;
    }

    dname = dirname(tmp_home_dname);
    bname = basename(tmp_home_bname);
    
    snprintf((*config_entry)->target, MAX_STRING_LEN, "%s/.%s", dname, bname);
    strncpy((*config_entry)->mountpoint, home, MAX_STRING_LEN - 1);

    free(tmp_home_dname);
    free(tmp_home_bname);
    return EXIT_SUCCESS;
}

/* PAM entry point for session creation */
int pam_sm_open_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    int rc;

    char home[MAX_STRING_LEN] = {0};

    // Variables to free out
    FILE *config_stream = NULL;
    group_line_t *fuse_group = NULL;
    char *configfile_path = NULL;
    config_entry_t *config_entry_p = NULL;

    // Do not free because used to pass data over PAM
    passwd_line_t *user_passwd = NULL;
    mounted_mountpoints_t *encfs_mounted = calloc(1, sizeof(mounted_mountpoints_t));

    if ( get_user_passwd(pamh, &user_passwd) != PAM_SUCCESS )
    {
        goto clean_after_failure;
    }

    if ( user_passwd != NULL )
    {
        get_group_struct(FUSE_GRP, &fuse_group);
        // Do like this so that we do not bother in freeing mountpoint
        // in any if since on the stack
        strncpy(home, user_passwd->home, MAX_STRING_LEN - 1);
        if( pam_set_data(pamh, USER_PASSWD, user_passwd, free_data_pointer) != PAM_SUCCESS )
        {
            _pam_log(LOG_ERR, "%s", "Unable to set user passwd data");

            goto clean_after_failure;
        }
    }

    if ( strlen(home) == 0 )
    {
        _pam_log(LOG_ERR, "%s", "Unable to find HOME environment variable");

        goto clean_after_failure;
    }

    if ( default_config_entry(home, &config_entry_p) == EXIT_FAILURE )
    {
        _pam_log(LOG_ERR, "%s", "default_config_entry failed");
        
        goto clean_after_failure;
    }
    
    // Attempt to mount into $HOME with nonempty option
    if ( pam_mount_encfs_wrapper(pamh, config_entry_p, user_passwd, fuse_group, 1) == EXIT_SUCCESS )
    {
        strncpy(encfs_mounted->mountpoints[encfs_mounted->n_mntpts].path, config_entry_p->mountpoint , MAX_STRING_LEN - 1);
        encfs_mounted->n_mntpts++;
    
    }
    else {
        // Fallback to the mountpoints found in $HOME/.pam_encfs
        rc = join_path(home, PAM_ENCFS_LOCAL_CONF, &configfile_path);
        if ( rc == EXIT_FAILURE )
        {
            _pam_log(LOG_ERR, "%s", "Cannot allocate memory for configfile_path");

            goto clean_after_failure;
        }

        if ( ! isfile(configfile_path) )
        {
            _pam_log(LOG_ERR, "Configuration file: %s does not exist", configfile_path);

            goto clean_after_failure;
        }

        config_stream = fopen(configfile_path, "r");
        if ( config_stream == NULL )
        {
            _pam_log(LOG_ERR, "Error opening configuration file: %s", configfile_path);

            goto clean_after_failure;
        }

        // Read the config file to find the next entry
        while ( next_entry(config_stream, &config_entry_p) != NULL )
        {
            if ( config_entry_p == NULL )
            {
                _pam_log(LOG_ERR, "No entry found in configuration file: %s", configfile_path);
                break;
            }

            // Attempts to mount config_entry_p->target on config_entry_p->mountpoint
            if ( pam_mount_encfs_wrapper(pamh, config_entry_p, user_passwd, fuse_group, 0) == EXIT_SUCCESS )
            {
                strncpy(encfs_mounted->mountpoints[encfs_mounted->n_mntpts].path, config_entry_p->mountpoint , MAX_STRING_LEN - 1);
                encfs_mounted->n_mntpts++;
            }
        }
    }

    // Save the structure use to store mounted mountpoint into pam data
    if ( pam_set_data(pamh, ENCFS_MNTP, encfs_mounted, free_data_pointer) != PAM_SUCCESS )
    {
        _pam_log(LOG_ERR, "%s", "Cannot set pam mountpoints data");
        for ( size_t i = 0; i < encfs_mounted->n_mntpts; i++ )
        {
            if ( pam_unmount_encfs_wrapper(encfs_mounted->mountpoints[i].path) != PAM_SUCCESS )
                _pam_log(LOG_ERR, "Unable to unmount mountpoint: %s", "pam_sm_open_session failure");
        }
        goto clean_after_failure;
    }

    // Cleans up everything dynamically allocated
    if ( config_stream != NULL )
        fclose(config_stream);
    if ( fuse_group != NULL )
        free(fuse_group);
    if ( config_entry_p != NULL )
        free(config_entry_p);
    if ( configfile_path != NULL )
        free(configfile_path);
    // Frees out the last password stored in pam memory
    pam_set_data(pamh, ENCFS_PWD, strdup(""), free_string_pointer);

    return PAM_SUCCESS;

// Cleans up everything dynamically allocated
clean_after_failure:
    if ( config_stream != NULL )
        fclose(config_stream);
    if ( fuse_group != NULL )
        free(fuse_group);
    if ( config_entry_p != NULL )
        free(config_entry_p);
    if ( configfile_path != NULL )
        free(configfile_path);
    // Frees out the last password stored in pam memory
    pam_set_data(pamh, ENCFS_PWD, strdup(""), free_string_pointer);
    return PAM_SESSION_ERR;
}

/* PAM entry point for session cleanup */
int pam_sm_close_session(pam_handle_t *pamh, int flags, int argc, const char **argv) {

    pid_t pid;
    int status;
    
    // PAM Data
    struct passwd_line *user_passwd = NULL;
    mounted_mountpoints_t *encfs_mounted = NULL;
    
    if ( pam_get_data(pamh, ENCFS_MNTP, (const void **) &encfs_mounted) != PAM_SUCCESS )
    {
        _pam_log(LOG_ERR, "%s", "Unable to get pam mountpoint data");
        return PAM_SESSION_ERR;
    }

    if ( pam_get_data(pamh, USER_PASSWD, (const void **) &user_passwd) != PAM_SUCCESS )
    {
        _pam_log(LOG_ERR, "%s", "Unable to get user_passwd data");
        return PAM_SESSION_ERR;
    }

    // Need to fork to drop privileges
    switch (pid = fork())
    {
        case -1:
            _pam_log(LOG_ERR, "%s", "Failed to fork");
            return PAM_SESSION_ERR;

        case 0:
            if ( drop_privileges(user_passwd->uid, user_passwd->gid, NULL, 0) != EXIT_SUCCESS )
            {
                _pam_log(LOG_ERR, "%s", "Cannot drop child privileges");
                exit(1);
            }

            for ( size_t i = 0; i < encfs_mounted->n_mntpts; i++ )
            {
                if ( pam_unmount_encfs_wrapper(encfs_mounted->mountpoints[i].path) != PAM_SUCCESS )
                    _pam_log(LOG_ERR, "Unable to unmount mountpoint: %s", "pam_sm_close_session failure");
            }
            exit(0);
    }

    status = waitpid_timeout(pid);

    if ( ! WIFEXITED(status) )
    {
        _pam_log(LOG_ERR, "%s", "Unmounting process failed");
        kill(pid, SIGTERM); 
        return PAM_SESSION_ERR;
    }

    // Don't check exit status if child didn't exit
    if ( WEXITSTATUS(status) != 0 )
    {
        _pam_log(LOG_ERR, "%s", "Unmounting child exited with error(s)");
        return PAM_SESSION_ERR;
    }

    return PAM_SUCCESS;
}
