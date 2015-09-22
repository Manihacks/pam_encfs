#include "config.h"

config_entry_t *next_entry(FILE *config_stream, config_entry_t **config_entry)
{
    char *line_p = NULL;
    size_t read_s = 0;
    size_t n = 0;

    char *entry_line = NULL;

    if ( config_stream == NULL )
        return NULL;

    if ( *config_entry == NULL )
    {
        *config_entry = calloc(sizeof(config_entry_t), 1);
    }
    else
    {
        free(*config_entry);
        *config_entry = calloc(sizeof(config_entry_t), 1);
    }


    while ( (read_s = getline(&line_p, &n, config_stream)) != -1 )
    {
        strtrim(line_p);
        if ( line_p[0] == '#' )
            continue;
        entry_line = line_p;
        char * encfs_dir = strsep(&entry_line, ENTRY_SEP);
        if ( encfs_dir == NULL)
            break;
        strncpy((*config_entry)->target, encfs_dir, MAX_STRING_LEN - 1);
        char * mountpoint = strsep(&entry_line, ENTRY_SEP);
        if ( mountpoint == NULL)
            break;
        strncpy((*config_entry)->mountpoint, mountpoint, MAX_STRING_LEN - 1);
        break;
    }

    if ( line_p != NULL )
    {
        free(line_p);
    }
    
    if ( read_s == -1 )
    {
        return NULL;
    }

    return *config_entry;
}
