#include "sdl/Window.hpp"
#include "spdlog/spdlog.h"

namespace tools::sdl {

std::atomic<bool> Window::_sdl_initialized = false;
std::atomic<uint8_t> Window::_instances_count = 0;

bool Window::sdl_init() {
    SPDLOG_INFO("SDL init.");
    bool ret = true;
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) < 0) {
        SPDLOG_ERROR("Failed to initialize SDL.");
        ret = false;
    }
    _sdl_initialized = ret;
    return ret;
}

void Window::sdl_cleanup() {
    SPDLOG_INFO("SDL cleanup.");
    SDL_Quit();
    _sdl_initialized = false;
}

Window::Window() {
    _width = 640;
    _height = 320;
    init();
}

Window::Window(const std::string &title, int width, int height) {
    _window_title = title;
    _width = width;
    _height = height;
    init();
}

Window::~Window() {
    if (_initialized)
        stop();
}

bool Window::init() {
    if (_instances_count == 0)
        sdl_init();

    if(TTF_Init() != 0) {
        SPDLOG_ERROR("Failed to initialize TTF.");
        return false;
    }

    if(!get_screen_size()) {
        SPDLOG_ERROR("Failed to get screen's size.");
        return false;
    }

    SPDLOG_INFO("Screen size = ({}, {})", _screen_width, _screen_height);

    if (_width > _screen_width)
        _width = _screen_width;
    if (_height > _screen_height)
        _height = _screen_height;

    _window = SDL_CreateWindow(
        _window_title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        _width,
        _height,
        SDL_WINDOW_RESIZABLE
    );

    if(_window == nullptr) {
        SPDLOG_ERROR("Failed to create window.");
        return false;
    }

    _renderer = SDL_CreateRenderer(
        _window,
        -1,
        SDL_RENDERER_ACCELERATED
    );

    if(_renderer == nullptr) {
        SPDLOG_ERROR("Failed to create renderer.");
        return false;
    }

    SPDLOG_INFO("Width = {}, height = {}", get_width(), get_height());

    _instances_count++;
    SPDLOG_DEBUG("{} renderer instance(s).", _instances_count);

    SPDLOG_INFO("Init done.");
    _initialized = true;
    return true;
}

void Window::stop() {
    SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    _initialized = false;

    _instances_count--;
    SPDLOG_DEBUG("{} renderer instance(s).", _instances_count);
    if (_instances_count == 0) {
        sdl_cleanup();
    }
}

void Window::refresh() {
    SDL_RenderPresent(_renderer);
}

bool Window::clear() {
    if(SDL_RenderClear(_renderer) == -1) {
        SPDLOG_ERROR("Failed to clear renderer.");
        return false;
    }
    return true;
}

void Window::set_width(int width) {
    _width = width;
}

void Window::set_height(int height) {
    _height = height;
}

void Window::set_size(int width, int height) {
    _width = width;
    _height = height;
}

void Window::set_position(int x, int y) {
    SDL_SetWindowPosition(_window, x, y);
}

void Window::center() {
    SDL_SetWindowPosition(_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

SDL_Texture* Window::get_render_target() {
    return SDL_GetRenderTarget(_renderer);
}

bool Window::set_render_target(SDL_Texture* dst) {
    if(SDL_SetRenderTarget(_renderer, dst) == -1) {
        SPDLOG_ERROR("Failed to set rendering target.");
        return false;
    }
    return true;
}

bool Window::set_viewport(SDL_Rect* rect) {
    if(rect == nullptr) {
        SPDLOG_ERROR("Cannot set viewport, nullptr.");
        return false;
    }

    if(SDL_RenderSetViewport(_renderer, rect) == -1) {
        SPDLOG_ERROR("Failed to set viewport.");
        return false;
    }

    SPDLOG_INFO("Set viewport to h = {}, w = {}, x = {}, y = {}", rect->h, rect->w, rect->x, rect->y);
    return true;
}

bool Window::set_draw_color(SDL_Color& color) {
    return set_draw_color(color.r, color.g, color.b, color.a);
}

bool Window::set_draw_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    if(SDL_SetRenderDrawColor(
        _renderer,
        r,
        g,
        b,
        a
    ) == -1) {
        SPDLOG_ERROR("Failed to set draw color.");
        return false;
    }

    return true;
}

TTF_Font* Window::load_font(std::string font_path, int size) {
    if(font_path.empty()) {
        SPDLOG_ERROR("Cannot load font, empty path.");
        return nullptr;
    }

    TTF_Font* font = TTF_OpenFont(font_path.c_str(), size);
    if(font == nullptr)
        SPDLOG_ERROR("Failed to load font '{}'", font_path);
    else
        SPDLOG_INFO("Loaded font '{}'", font_path);
    return font;
}

SDL_Texture* Window::load_image(std::string img_path) {
    if(img_path.empty()) {
        SPDLOG_ERROR("Cannot load image, empty path.");
        return nullptr;
    }

    SDL_Surface* surface = SDL_LoadBMP(img_path.c_str());
    if(surface == nullptr) {
        SPDLOG_ERROR("Failed to create surface from image '{}'", img_path);
        return nullptr;
    }

    SDL_Texture* texture = surface_to_texture(surface);
    if(texture == nullptr) {
        SPDLOG_ERROR("Failed to create texture from image '{}'", img_path);
        return nullptr;
    }

    SPDLOG_INFO("Loaded image '{}'", img_path);
    return texture;
}

SDL_Texture* Window::load_text(
    std::string text,
    SDL_Color color,
    TTF_Font* font
) {
    SDL_Surface* surface = nullptr;
    if(font == nullptr) {
        if(_default_font == nullptr)
        {
            SPDLOG_ERROR("Cannot load text, no default font and no font provided.");
            return nullptr;
        }
        surface = TTF_RenderText_Solid(_default_font, text.c_str(), color);
    }
    else {
        surface = TTF_RenderText_Solid(font, text.c_str(), color);
    }

    if(surface == nullptr) {
        SPDLOG_ERROR("Failed to create surface from text.");
        return nullptr;
    }

    SDL_Texture* texture = surface_to_texture(surface);
    if(texture == nullptr) {
        SPDLOG_ERROR("Failed to create texture from image.");
        return nullptr;
    }

    SPDLOG_INFO("Loaded text '{}'", text);
    return texture;
}

bool Window::render_texture(SDL_Texture* texture, SDL_Rect* dst, SDL_Rect* portion) {
    if(texture == nullptr) {
        SPDLOG_ERROR("Cannot render texture, nullptr.");
        return false;
    }

    if(SDL_RenderCopy(_renderer, texture, portion, dst) == -1) {
        SPDLOG_ERROR("Failed to render texture.");
        return false;
    }
    return true;
}

bool Window::crop_texture(SDL_Texture* src, SDL_Texture*& dst, SDL_Rect* rect) {
    if(src == nullptr) {
        SPDLOG_ERROR("Cannot crop texture, source = nullptr.");
        return false;
    }

    if(rect == nullptr) {
        SPDLOG_ERROR("Cannot crop texture, rectangle = nullptr.");
        return false;
    }

    if(rect->h == 0 || rect->w == 0) {
        SPDLOG_ERROR("Cannot crop texture, rectangle has either height or width at 0.");
        return false;
    }

    dst = this->create_blank_render_target(rect->w, rect->h);

    if(dst == nullptr) {
        SPDLOG_ERROR("Failed to create target texture.");
        return false;
    }

    if(!this->render_to_texture(src, dst, rect)) {
        SPDLOG_ERROR("Failed to crop texture.");
        return false;
    }

    return true;
}

bool Window::get_screen_size() {
    SDL_DisplayMode mode = SDL_DisplayMode();
    if(SDL_GetDesktopDisplayMode(0, &mode) == -1) {
        SPDLOG_ERROR("Failed to get display information.");
        return false;
    }
    _screen_width = mode.w;
    _screen_height = mode.h;
    return true;
}

SDL_Texture* Window::surface_to_texture(SDL_Surface* surface) {
    if(surface == nullptr) {
        SPDLOG_ERROR("Cannot create texture from surface, nullptr.");
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(_renderer, surface);
    if(texture == nullptr) {
        SPDLOG_ERROR("Failed to create texture from surface.");
        return nullptr;
    }

    SDL_FreeSurface(surface);
    return texture;
}

bool Window::render_to_texture(SDL_Texture* src, SDL_Texture* dst, SDL_Rect* dstRect) {
    // Backup rendering target.
    SDL_Texture* target = this->get_render_target();

    // Set render target to destination texture.
    if(!this->set_render_target(dst)) {
        SPDLOG_ERROR("Failed to set rendering target prior to render to texture.");
        return false;
    }

    if(this->render_texture(src, nullptr, dstRect) == false) {
        SPDLOG_ERROR("Failed to render texture to target texture.");
        return false;
    }

    // Restore rendering target.
    if(!this->set_render_target(target)) {
        SPDLOG_ERROR("Failed to set rendering target back to default.");
        return false;
    }

    return true;
}

SDL_Texture* Window::create_blank_render_target(int width, int height) {
    return SDL_CreateTexture(_renderer, SDL_GetWindowPixelFormat(_window), SDL_TEXTUREACCESS_TARGET, width, height);
}

bool Window::draw_rectangle(SDL_Rect* rect, bool fill) {
    if(rect == nullptr) {
        SPDLOG_ERROR("draw_rectangle : rect = nullptr.");
        return false;
    }

    if(SDL_RenderDrawRect(_renderer, rect) == -1) {
        SPDLOG_ERROR("draw_rectangle : Failed to render rectangle.");
        return false;
    }

    if (fill && SDL_RenderFillRect(_renderer, rect) == -1) {
        SPDLOG_ERROR("Failed to fill rectangle.");
        return false;
    }

    return true;
}

} // namespace tools::sdl