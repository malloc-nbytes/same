#include <stdint.h>
#include <stdio.h>
#include <dirent.h>

#define CIO_IMPL
#include "cio.h"
#define CLAP_IMPL
#include "clap.h"
#include "dyn_array.h"

#define FLAG_1HY_HELP 'h'
#define FLAG_2HY_HELP "help"

#define FLAG_1HY_RECURSIVE 'r'
#define FLAG_2HY_RECURSIVE "recursive"

DYN_ARRAY_TYPE(char *, Str_Array);

enum Flag_Type {
        FT_REC = 1 << 0,
};

struct {
        uint32_t flags;
} g_config = {
        .flags = 0x0,
};

void usage(void) {
        printf("same <dir...|file...> [options...]\n");
        printf("Options:\n");
        printf("  -%c, --%s         show this help message\n", FLAG_1HY_HELP, FLAG_2HY_HELP);
        printf("  -%c, --%s    search recursively\n", FLAG_1HY_RECURSIVE, FLAG_2HY_RECURSIVE);
        exit(0);
}

void walk(const char *path, Str_Array *arr) {
        char *abs_path = realpath(path, NULL);
        if (!abs_path) {
                perror("realpath");
                return;
        }

        struct stat st;
        if (stat(abs_path, &st) == -1) {
                perror("stat");
                free(abs_path);
                return;
        }

        if (S_ISREG(st.st_mode)) {
                dyn_array_append(*arr, strdup(abs_path));
                free(abs_path);
                return;
        }

        if (!S_ISDIR(st.st_mode)) {
                char msg[256];
                snprintf(msg, sizeof(msg), "Path %s is not a directory or a supported file format", abs_path);
                free(abs_path);
                return;
        }

        DIR *dir = opendir(abs_path);
        if (!dir) {
                perror("opendir");
                free(abs_path);
                return;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
                // Skip "." and ".."
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                        continue;
                }

                char subpath[PATH_MAX];
                snprintf(subpath, sizeof(subpath), "%s/%s", abs_path, entry->d_name);

                // Verify subpath is valid by getting its absolute path
                char *abs_subpath = realpath(subpath, NULL);
                if (!abs_subpath) {
                        perror("realpath");
                        continue;
                }

                if (stat(abs_subpath, &st) == -1) {
                        perror("stat");
                        free(abs_subpath);
                        continue;
                }

                // Check if it's a regular file and a music file
                if (S_ISREG(st.st_mode)) {
                        dyn_array_append(*arr, strdup(abs_subpath));
                } else if (S_ISDIR(st.st_mode) && (g_config.flags & FT_REC)) {
                        // Recursive call with absolute subpath
                        walk(abs_subpath, arr);
                }

                free(abs_subpath);
        }

        closedir(dir);
        free(abs_path);
}

Str_Array io_flatten_dirs(const Str_Array *dirs) {
        Str_Array ar = dyn_array_empty(Str_Array);

        for (size_t i = 0; i < dirs->len; ++i) {
                char *abs_dir = realpath(dirs->data[i], NULL);
                if (!abs_dir) {
                        perror("realpath");
                        char msg[256];
                        snprintf(msg, sizeof(msg), "Failed to resolve absolute path for %s", dirs->data[i]);
                        continue;
                }

                walk(abs_dir, &ar);
        }

        return ar;
}

int main(int argc, char **argv) {
        if (argc <= 1) { usage(); }

        --argc, ++argv;

        Str_Array filepaths = dyn_array_empty(Str_Array);
        clap_init(argc, argv);
        Clap_Arg arg = {0};
        while (clap_next(&arg)) {
                if (arg.hyphc == 1 && arg.start[0] == FLAG_1HY_HELP) {
                        usage();
                } else if (arg.hyphc == 2 && !strcmp(arg.start, FLAG_2HY_HELP)) {
                        usage();
                } else if (arg.hyphc == 1 && arg.start[0] == FLAG_1HY_RECURSIVE) {
                        g_config.flags |= FT_REC;
                } else if (arg.hyphc == 2 && !strcmp(arg.start, FLAG_2HY_RECURSIVE)) {
                        g_config.flags |= FT_REC;
                } else if (arg.hyphc > 0) {
                        fprintf(stderr, "unknown flag: %s\n", arg.start);
                        exit(1);
                } else {
                        dyn_array_append(filepaths, strdup(arg.start));
                }
        }

        Str_Array resolved_files = io_flatten_dirs(&filepaths);

        for (size_t i = 0; i < filepaths.len; ++i)      free(filepaths.data[i]);
        for (size_t i = 0; i < resolved_files.len; ++i) free(resolved_files.data[i]);
        dyn_array_free(filepaths);
        dyn_array_free(resolved_files);

        return 0;
}
