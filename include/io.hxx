#ifndef IO_HXX_INCLUDED
#define IO_HXX_INCLUDED

#include <vector>
#include <string>

std::vector<uint8_t> file_to_bytes(const std::string &filename);
std::vector<std::string> get_files_in_directory(const std::string &dir);

#endif // IO_HXX_INCLUDED
