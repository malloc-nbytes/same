#include <vector>
#include <fstream>
#include <cstdint>
#include <filesystem>

#include "io.hxx"

std::vector<uint8_t>
file_to_bytes(const std::string &filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read file: " + filename);
    }

    return buffer;
}

std::vector<std::string>
get_files_in_directory(const std::string &dir) {
    std::vector<std::string> paths;
    try {
        for (const auto &entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                paths.push_back(entry.path().string());
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        throw std::runtime_error("Error accessing directory: " + std::string(e.what()));
    }
    return paths;
}
