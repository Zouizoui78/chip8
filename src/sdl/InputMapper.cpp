#include "sdl/InputMapper.hpp"

#include "files.hpp"
#include "spdlog/spdlog.h"

namespace tools::sdl {

InputMapper::InputMapper() {}
InputMapper::~InputMapper() {}

void InputMapper::set_mapping(SDL_Keycode key, uint8_t mapped_key) {
    _keymap[key] = mapped_key;
}

bool InputMapper::set_mapping(const std::string &key, uint8_t mapped_key) {
    SDL_Keycode sdl_key = SDL_GetKeyFromName(key.c_str());
    if (sdl_key == SDLK_UNKNOWN) {
        SPDLOG_ERROR("Unknown key '{}'", key);
        return false;
    }
    set_mapping(sdl_key, mapped_key);
    return true;
}

void InputMapper::remove_mapping(SDL_Keycode key) {
    _keymap.erase(key);
}

bool InputMapper::remove_mapping(const std::string &key) {
    SDL_Keycode sdl_key = SDL_GetKeyFromName(key.c_str());
    if (sdl_key == SDLK_UNKNOWN) {
        SPDLOG_ERROR("Unknown key : '{}' ; error : {}", key, SDL_GetError());
        return false;
    }
    remove_mapping(sdl_key);
    return true;
}

bool InputMapper::load_map(const json &map) {
    bool success = true;
    for (const auto &e : map.items()) {
        if (!set_mapping(e.key(), e.value()))
            success = false;
    }
    return success;
}

bool InputMapper::load_map(const std::string &json_path) {
    std::string content = tools::utils::files::read_text_file(json_path);
    if (content.empty())
        return false;

    json j;
    try {
        j = json::parse(content);
    }
    catch (const nlohmann::detail::parse_error &e) {
        SPDLOG_ERROR("Failed to parse input map json '{}' : {}", json_path, e.what());
        return false;
    }

    return load_map(j);
}

int InputMapper::map_key(SDL_Keycode key) {
    std::map<SDL_Keycode, uint8_t>::iterator value_it = _keymap.find(key);
    if (value_it == _keymap.end())
        return -1;
    return value_it->second;
}

int InputMapper::map_key(const std::string &key) {
    SDL_Keycode sdl_key = SDL_GetKeyFromName(key.c_str());
    if (sdl_key == SDLK_UNKNOWN) {
        SPDLOG_ERROR("Unknown key '{}'", key);
        return false;
    }
    return map_key(sdl_key);
}

} // namespace tools::sdl