#include <cassert>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <cstdint>
#include <filesystem>

#include <string.h>

#include <forge/arg.h>
#include "io.hxx"
#include "flags.hxx"

typedef struct {
    std::string filepath;
    std::vector<uint8_t> bytes;
} entry;

size_t
hash_bytes(const std::vector<uint8_t> &bytes)
{
    std::string_view view(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    return std::hash<std::string_view>{}(view);
}

bool
are_bytes_equal(const std::vector<uint8_t> &a,
                const std::vector<uint8_t> &b)
{
    return a.size() == b.size()
        && std::equal(a.begin(), a.end(), b.begin());
}

void
run(const char *dir)
{
    std::unordered_map<size_t, std::vector<std::string>> hash_map;
    std::vector<std::string> files = get_files_in_directory(dir);

    // Compute hashes
    for (const auto& file : files) {
        std::vector<uint8_t> bytes = file_to_bytes(file);
        size_t hash = hash_bytes(bytes);
        hash_map[hash].push_back(file);
    }

    // Group files by content
    std::vector<std::vector<std::string>> groups;
    for (const auto& [hash, filepaths] : hash_map) {
        if (filepaths.size() <= 1) continue; // Skip singletons

        std::vector<std::vector<std::string>> unique_groups;
        for (const auto& filepath : filepaths) {
            bool added = false;

            for (auto& group : unique_groups) {
                auto bytes1 = file_to_bytes(group[0]);
                auto bytes2 = file_to_bytes(filepath);
                if (are_bytes_equal(bytes1, bytes2)) {
                    group.push_back(filepath);
                    added = true;
                    break;
                }
            }

            if (!added) {
                unique_groups.push_back({filepath});
            }
        }

        groups.insert(groups.end(), unique_groups.begin(), unique_groups.end());
    }

    for (size_t i = 0; i < groups.size(); ++i) {
        if (groups[i].size() > 1) {
            printf("Group %zu (identical files):\n", i+1);
            for (const auto& filepath : groups[i]) {
                printf("  %s\n", filepath.c_str());
            }
        }
    }

    if (groups.size() == 0) {
        printf("No duplicates found in %s\n", dir);
    }
}

int
main(int argc, char **argv)
{
    if (argc <= 1) {
        usage();
    }

    char *dir = NULL;

    forge_arg *arghd = forge_arg_alloc(argc, argv, 1);
    forge_arg *arg = arghd;
    while (arg) {
        if (!arg->h) {
            if (dir) {
                fprintf(stderr, "only one directory is supported\n");
                exit(1);
            }
            dir = strdup(arg->s);
        } else {
            fprintf(stderr, "options are unimplemented\n");
            exit(1);
        }
        arg = arg->n;
    }

    assert(dir);
    run(dir);
    free(dir);
    forge_arg_free(arghd);

    return 0;
}
