#ifdef DEBUG
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#endif

#include "spdlog/spdlog.h"

#include "Chip8.hpp"

#include "tools/utils/fs.hpp"
#include "tools/utils/Scheduler.hpp"
#include "tools/utils/Stopwatch.hpp"

#include "tools/sdl/InputMapper.hpp"
#include "tools/sdl/WaveformPlayer.hpp"
#include "tools/sdl/Window.hpp"

#include "tools/waveform/Waveforms.hpp"

void log_init() {
    #ifdef DEBUG
    spdlog::set_level(spdlog::level::debug);
    #endif
}

int main(int argc, char **argv) {
    log_init();

    if (argc < 2 || argc > 5) {
        SPDLOG_INFO("Usage : chip8 [rom name] [cpu freq] [background color hex] [foreground color hex]");
        return 0;
    }

    std::string rom = argv[1];

    uint32_t cpu_freq = 1000;
    if (argc >= 3)
        cpu_freq = std::stoi(argv[2]);

    // Color
    uint32_t back_color = 0;
    uint32_t front_color = 0xffffff;

    if (argc >= 4)
        back_color = std::strtol(argv[3], NULL, 16);

    if (argc >= 5)
        front_color = std::strtol(argv[4], NULL, 16);

    uint8_t back_red = back_color >> 16;
    uint8_t back_green = (back_color >> 8) & 0xff;
    uint8_t back_blue = back_color & 0xff;

    uint8_t front_red = front_color >> 16;
    uint8_t front_green = (front_color >> 8) & 0xff;
    uint8_t front_blue = front_color & 0xff;

    uint16_t timer_freq = 60;
    uint16_t display_freq = 30;

    uint64_t cpu_count = 0, timer_count = 0, display_count = 0;

    tools::chip8::Chip8 cpu;
    if (!cpu.load_rom(rom)) {
        SPDLOG_ERROR("Failed to load rom '{}'.", rom);
        exit(1);
    }

    uint8_t pixel_width = 16;
    uint8_t pixel_height = 20;

    tools::sdl::InputMapper mapper;
    mapper.set_mapping("1", 0x1);
    mapper.set_mapping("2", 0x2);
    mapper.set_mapping("3", 0x3);
    mapper.set_mapping("4", 0xc);
    mapper.set_mapping("A", 0x4);
    mapper.set_mapping("Z", 0x5);
    mapper.set_mapping("E", 0x6);
    mapper.set_mapping("R", 0xd);
    mapper.set_mapping("Q", 0x7);
    mapper.set_mapping("S", 0x8);
    mapper.set_mapping("D", 0x9);
    mapper.set_mapping("F", 0xe);
    mapper.set_mapping("W", 0xa);
    mapper.set_mapping("X", 0x0);
    mapper.set_mapping("C", 0xb);
    mapper.set_mapping("V", 0xf);

    // Audio handling.
    auto generator = std::make_shared<tools::waveform::WaveformGenerator>();
    tools::sdl::WaveformPlayer player(generator);
    auto square_wave = std::make_shared<tools::waveform::Square>();
    square_wave->set_frequency(440);
    generator->add_waveform(square_wave);
    if (!player.is_initialized())
        SPDLOG_ERROR("Failed to initialize audio.");
    ///////////////////////

    tools::sdl::Window w("Chip8", pixel_width * WIDTH, pixel_height * HEIGHT);

    tools::utils::Scheduler scheduler;

    tools::utils::Stopwatch loop_stopwatch("loop");
    uint64_t previous = 0;
    double n_inst_remainder = 0;

    tools::utils::Task emulation_task;
    emulation_task.name = "Emulation task";
    emulation_task.delay_ns = std::chrono::nanoseconds(1000000000 / timer_freq);
    spdlog::info("emulation delay {}", emulation_task.delay_ns.count());
    emulation_task.task = [&]() {
        // Get duration since last loop in seconds.
        uint64_t duration = loop_stopwatch.get_duration();
        double seconds_since_last_loop = (duration - previous) / 1e9;
        previous = duration;

        // Decrease timers.
        cpu.decrease_timers();
        if (cpu.get_sound_timer() > 0)
            player.play();
        else
            player.pause();
        ++timer_count;

        // Compute how many instructions we
        // should have done since last loop.
        double n_inst = cpu_freq * seconds_since_last_loop;

        // n_inst only makes sense as a whole number
        // since we cannot do a fractionnal number
        // of instructions.
        // So every time we skip a piece of instruction,
        // which makes us running late in the simulation.
        // To compensate this we sum the fractionnal part
        // of n_inst and add an instruction when
        // n_inst_remainder > 1.
        // It's kind of like leap years.
        n_inst_remainder += ::modf(n_inst, &n_inst);
        if (n_inst_remainder > 1) {
            n_inst += 1;
            n_inst_remainder -= 1;
        }

        // Execute n_inst instructions.
        for (uint32_t i = 0 ; i < n_inst ; ++i) {
            cpu.next_instruction();
            ++cpu_count;
        }

        return true;
    };

    SDL_Rect rect { 0, 0, pixel_width, pixel_height };

    SDL_Event event;
    tools::utils::Task sdl_task;
    sdl_task.name = "SDL task";
    sdl_task.delay_ns = std::chrono::nanoseconds(1000000000 / display_freq);
    spdlog::info("emulation delay {}", sdl_task.delay_ns.count());
    sdl_task.task = [&]() {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                scheduler.stop();
            }
            else if (event.type == SDL_KEYDOWN) {
                int mapped = mapper.map_key(event.key.keysym.sym);
                if (mapped != -1)
                    cpu.key_pressed(mapped);
            }
            else if (event.type == SDL_KEYUP) {
                int mapped = mapper.map_key(event.key.keysym.sym);
                if (mapped != -1)
                    cpu.key_released(mapped);
            }
            else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    pixel_width = event.window.data1 / WIDTH;
                    pixel_height = event.window.data2 / HEIGHT;
                    rect.w = pixel_width;
                    rect.h = pixel_height;
                }
            }
        }

        w.set_draw_color(back_red, back_green, back_blue);
        w.clear();
        w.set_draw_color(front_red, front_green, front_blue);

        auto screen = cpu.get_screen_buffer();
        for (int i = 0 ; i < SCREEN_SIZE ; ++i) {
            if (screen[i]) {
                uint8_t y = (i / WIDTH);
                uint8_t x = i - (y * WIDTH);
                rect.x = x * pixel_width;
                rect.y = y * pixel_height;
                w.draw_rectangle(&rect, true);
            }
        }

        w.refresh();
        ++display_count;
        return true;
    };

    scheduler.add_task(emulation_task);
    scheduler.add_task(sdl_task);

    tools::utils::Stopwatch stopwatch("chip8");
    loop_stopwatch.reset();
    scheduler.start();

    uint64_t duration = stopwatch.get_duration();
    SPDLOG_INFO("cpu = {}/s", 1e9 * cpu_count / duration);
    SPDLOG_INFO("timer = {}/s", 1e9 * timer_count / duration);
    SPDLOG_INFO("display = {}/s", 1e9 * display_count / duration);

    return 0;
}