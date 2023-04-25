#ifndef FILES_HPP
#define FILES_HPP

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace tools::utils::files {

/**
 * @brief Read a file and return its content as a string.
 * @param path Path of the file to read.
 * @return String filled with the content of the file.
 */
std::string read_text_file(const std::string &path);

/**
 * @brief Read a binary file into the returned vector.
 * @param path Path of the file to read.
 * @return vector containing the read bytes.
 */
std::vector<uint8_t> read_binary_file(const std::string &path);

/**
 * @brief Write the content of the given vector into a binary file.
 * @param vector Vector to dump.
 * @param path Path of the file to write.
 * @return true => ok ; false => error.
 */
bool write_binary_file(const std::vector<uint8_t> &data, const std::string &path);

} // namespace tools::utils

#endif // FILES_HPP