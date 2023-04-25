#include "files.hpp"
#include "spdlog/spdlog.h"

namespace tools::utils::files {

std::string read_text_file(const std::string &path) {
    std::string ret("");

    std::ifstream file(path);
    if (!file.is_open()) {
        SPDLOG_ERROR("Failed to open file {} : {}", path, strerror(errno));
        return ret;
    }

    uintmax_t file_size = std::filesystem::file_size(path);
    SPDLOG_DEBUG("Reading {} bytes from '{}'", file_size, path);

    char *buf = static_cast<char *>(malloc((file_size + 1) * sizeof(char)));
    file.read(buf, file_size);
    buf[file_size] = '\0';

    ret = buf;
    free(buf);

    return ret;
}

std::vector<uint8_t> read_binary_file(const std::string &path) {
    std::vector<uint8_t> result;

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SPDLOG_ERROR("Failed to open file {} : {}", path, strerror(errno));
        return result;
    }

    uintmax_t file_size = std::filesystem::file_size(path);
    SPDLOG_DEBUG("Reading {} bytes from '{}'", file_size, path);

    uint8_t tmp;
    for (size_t i = 0 ; i < file_size ; ++i) {
        file.read(reinterpret_cast<char *>(&tmp), 1);
        result.push_back(tmp);
    }

    return result;
}

bool write_binary_file(const std::vector<uint8_t> &data, const std::string &path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SPDLOG_ERROR("Failed to open file {} : {}", path, strerror(errno));
        return false;
    }

    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    return true;
}

} // namespace tools::utils::files