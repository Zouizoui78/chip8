#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <atomic>
#include <string>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

namespace tools::sdl {

/**
 * This class handles everything directly related to rendering on screen.
 */
class Window
{
    public:

    Window();
    Window(const std::string &title, int width = 640, int height = 320);
    ~Window();

    /**
     * @brief Render current back buffer.
     */
    void refresh();

    /**
     * @brief Fill the screen with the current drawing color.
     * Ignore viewports.
     *
     * @return Ok or not.
     */
    bool clear();

    void set_width(int width);
    void set_height(int height);
    void set_size(int width, int height);

    void set_position(int x, int y);
    void center();

    int get_width() { return _width; }

    int get_height() { return _height; }

    TTF_Font* get_default_font() { return _default_font; }

    SDL_Texture* get_render_target();
    bool set_render_target(SDL_Texture* dst);

    /**
     * @brief Set current rendering viewport to the rectangle.
     *
     * @param rect Rectangle to use as viewport. If nullptr the entire screen is taken as a viewport.
     * @return Ok or not.
     */
    bool set_viewport(SDL_Rect* rect = nullptr);

    /**
     * @brief Set default font for text rendering.
     *
     * @param font Font to set as default.
     */
    void set_default_font(TTF_Font* font) { _default_font = font; }

    /**
     * @brief Set the current rendering color used for rectangles, lines and clear.
     *
     * @param color Color to be used.
     * @return Ok or not.
     */
    bool set_draw_color(SDL_Color& color);

    /**
     * @brief Set the current rendering color used for rectangles, lines and clear.
     *
     * @param r
     * @param g
     * @param b
     * @param a
     * @return true success
     * @return false failure
     */
    bool set_draw_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0);

    /**
     * @brief Load a font.
     *
     * @param fontPath Path to the font to load.
     * @param size Point size to load the font fe.
     * @return TTF_Font* nullptr if error.
     */
    TTF_Font* load_font(std::string fontPath, int size);

    /**
     * @brief Load an image into a texture.
     *
     * @param imgPath Path to the image to load.
     * @return texture or nullptr on error.
     */
    SDL_Texture* load_image(std::string imgPath);

    /**
     * @brief Create a texture from a text.
     *
     * @param text Text to render.
     * @param color Text color, default is white.
     * @param font Font to use. Default font is used if nullptr.
     * @return texture or nullptr on error.
     */
    SDL_Texture* load_text(
        std::string text,
        SDL_Color color = { 255, 255, 255, 255 },
        TTF_Font* font = nullptr
    );

    /**
     * @brief Render a texture.
     *
     * @param texture The texture to render.
     * @param dst Part of the rendering target in which the texture is rendered. Whole rendering target if nullptr.
     * @param portion The portion of the texture to copy to the destination. Entire destination is used if nullptr.
     * @return Ok or not.
     */
    bool render_texture(SDL_Texture* texture, SDL_Rect* dst = nullptr, SDL_Rect* portion = nullptr);

    /**
     * @brief Extract a part of a texture.
     *
     * @param source Texture to extract a part of.
     * @param dst Texture to render to. Will be erased.
     * @param rect Part to extract.
     * @return Ok or not.
     */
    bool crop_texture(SDL_Texture* src, SDL_Texture*& dst, SDL_Rect* rect);

    /**
     * @brief Fill _width and _height with values taken from SDL.
     *
     * @return Ok or not.
     */
    bool get_screen_size();

    /**
     * @brief Create a texture from a surface. The surface is freed.
     *
     * @param surface
     * @return texture or nullptr on error.
     */
    SDL_Texture* surface_to_texture(SDL_Surface* surface);

    /**
     * @brief Render a texture into another texture.
     *
     * @param src Texture to render.
     * @param dst Texture to render into.
     * @param dstRect Destination rectangle.
     * @return true Success
     * @return false Failure
     */
    bool render_to_texture(SDL_Texture* src, SDL_Texture* dst, SDL_Rect* dstRect);

    /**
     * @brief Create a Blank Render Target object.
     *
     * @param width
     * @param height
     * @return SDL_Texture* nullptr if failure.
     */
    SDL_Texture* create_blank_render_target(int width, int height);

    /**
     * @brief Draw the passed rectangle.
     *
     * @param rect Rectangle to draw.
     * @return Ok or not.
     */
    bool draw_rectangle(SDL_Rect* rect, bool fill = false);


    private:

    /**
     * @brief Renderer initialization.
     *
     * @return Ok or not.
     */
    bool init();

    /**
     * @brief Renderer stop.
     */
    void stop();

    static bool sdl_init();
    static void sdl_cleanup();
    static std::atomic<bool> _sdl_initialized;
    static std::atomic<uint8_t> _instances_count;

    bool _initialized = false;

    int _width;
    int _height;
    int _screen_width = 0;
    int _screen_height = 0;

    std::string _window_title = "";

    SDL_Window* _window = nullptr;
    SDL_Renderer* _renderer = nullptr;

    /**
     * @brief Font used to render text, if not => nullptr.
     */
    TTF_Font* _default_font = nullptr;
};

} // namespace tools::sdl

#endif // WINDOW_HPP