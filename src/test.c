#include "utils.h"
#include "config.h"

int test_get_group_struct(char *group)
{
    //char *group = "fuse";
    group_line_t *group_data = NULL;

    get_group_struct(group, &group_data);

    if (group_data != NULL)
    {
        printf("group: %s\n", group_data->group);
        printf("passwd: %s\n", group_data->passwd);
        printf("gid: %u\n", group_data->gid);
        printf("n_members: %u\n", group_data->n_members);
        printf("members:");
        for ( size_t i = 0; i < group_data->n_members; i++)
        {
            printf("%s,", group_data->members[i].name);
        }
        printf("\n");
    }
    printf("is user in group: %d\n", is_user_in_group("dev", group_data));
    if ( group_data )
        free(group_data);
}

int test_get_encfs_mounted()
{
   mounted_mountpoints_t encfs_mounted = {0};

   get_encfs_mounted(&encfs_mounted);

   for ( size_t i = 0; i < encfs_mounted.n_mntpts; i++ )
   {
       printf("encfs mountpoint mounted: %s\n", encfs_mounted.mountpoints[i].path);
   }
}

int main(int argc, const char **argv)
{
    const char *test_config_file = "/tmp/config";
    const char *test_file_path = "/home/test";
    const char *test_dir_path = "/usr/bin";
    const char *test_rel_path = "./usr/bin";
    const char *test_junk = "u";
    const char *to_trim = "              to trim entry               ";
    char *dyn_to_trim = calloc(1, strlen(to_trim) + 1);
    memcpy(dyn_to_trim, to_trim, strlen(to_trim));
    char *joined_path, *dirnamep = NULL;

    if ( isdir(test_dir_path) )
        printf("%s is a directory \n", test_dir_path);
    if ( isfile(test_file_path) )
        printf("%s is a file \n", test_file_path);
    
    /*
    printf("%s basename is : %s\n", test_file_path, basename(test_file_path));
    printf("%s basename is : %s\n", test_dir_path, basename(test_dir_path));

    if ( ! dname(test_file_path, &dirnamep) )
    {
        printf("%s dirname is : %s\n", test_file_path, dirnamep);
        free(dirnamep);
    }

    if ( ! dname(test_rel_path, &dirnamep) )
    {
        printf("%s dirname is : %s\n", test_rel_path, dirnamep);
        free(dirnamep);
    }

    if ( ! dname(test_junk, &dirnamep) )
    {
        printf("%s dirname is : %s\n", test_junk, dirnamep);
        free(dirnamep);
    }*/
    
    if ( ! join_path(test_rel_path, test_junk, &joined_path) )
    {
        printf("%s joined with %s = %s\n", test_rel_path, test_junk, joined_path);
        free(joined_path);
    }
    if ( ! is_mountpoint_busy(test_file_path) )
    {
        printf("mountpoint: %s is not busy\n", test_file_path);
    }

    //search_home( "quentin" );

    printf("len not trimmed string: %d\n", strlen(dyn_to_trim));
    strtrim(dyn_to_trim);
    printf("trimmed string: %s and size: %d\n", dyn_to_trim, strlen(dyn_to_trim));

    FILE *config_stream = fopen(test_config_file, "r");
    if ( config_stream != NULL )
    {
        config_entry_t *config_entry_p = NULL;
        while ( next_entry(config_stream, &config_entry_p) != NULL )
        {
            printf("encfs_dir: %s\n", config_entry_p->target);
            printf("mountpoint: %s\n", config_entry_p->mountpoint);
        }
    }

    test_get_group_struct("libvirt");
    test_get_group_struct("fcron");
    test_get_group_struct("fuse");

    test_get_encfs_mounted();

    return EXIT_SUCCESS;
}
