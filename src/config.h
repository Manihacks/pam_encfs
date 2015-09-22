#ifndef CONFIG_H
#define CONFIG_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils.h"

#define ENTRY_SEP ":"

struct config_entry {
    char target[MAX_STRING_LEN];
    char mountpoint[MAX_STRING_LEN];
};

typedef struct config_entry config_entry_t; 

config_entry_t *next_entry(FILE *config_stream, config_entry_t **config_entry);
#endif
