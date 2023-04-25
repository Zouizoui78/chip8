#include "sdl/Sound.hpp"
#include "spdlog/spdlog.h"

namespace tools::sdl {

// Default waveform implementations
ASound::ASound() {
    set_volume(1);
    set_frequency(440);
}

ASound::~ASound() {}

double ASound::get_volume() const {
    return _volume;
}

void ASound::set_volume(double volume) {
    _volume = volume;
    _amplitude_mult = _volume * SOUND_AMPLITUDE;
}

double ASound::get_frequency() const  {
    return _frequency;
}

void ASound::set_frequency(double frequency) {
    if (frequency <= 0)
        return;
    _frequency = frequency;
    _period = 1.0 / frequency;
    _sampling_period = SOUND_SAMPLING_RATE / _frequency;
}

double ASound::get_period() const {
    return _period;
}


Sinus::Sinus() : ASound() {
    update_freq_mult();
}

Sinus::~Sinus() {}

int16_t Sinus::synthesize(SoundSynthesisData data) const {
    return _amplitude_mult * sin(_freq_mult * data.time);
}

void Sinus::set_frequency(double frequency) {
    ASound::set_frequency(frequency);
    update_freq_mult();
}

void Sinus::update_freq_mult() {
    _freq_mult = 2.0 * 3.141592653589793116 * _frequency;
}


Square::Square() : ASound() {
    update_sampling_duty_cycle();
}

Square::~Square() {}

int16_t Square::synthesize(SoundSynthesisData data) const {
    return (data.sample_n % _sampling_period) >= _sampling_duty_cycle ? -_amplitude_mult : _amplitude_mult;
}

void Square::set_frequency(double frequency) {
    ASound::set_frequency(frequency);
    update_sampling_duty_cycle();
}

double Square::get_duty_cycle() const {
    return _duty_cycle;
}

void Square::set_duty_cycle(double duty_cycle) {
    _duty_cycle = duty_cycle;
    update_sampling_duty_cycle();
}

void Square::update_sampling_duty_cycle() {
    _sampling_duty_cycle = _sampling_period * _duty_cycle;
}

/////////////////////////////////////


// SoundPlayer

SoundPlayer::SoundPlayer() {
    init();
}

SoundPlayer::~SoundPlayer() {
    if (is_initialized()) {
        SDL_CloseAudio();
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        _is_audio_initialized = false;
        SPDLOG_INFO("SDL audio subsystem cleaned up.");
    }
}

bool SoundPlayer::init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        SPDLOG_ERROR("Failed to initialize audio subsystem : {}", SDL_GetError());
        _is_audio_initialized = false;
    }

    SDL_AudioSpec desired;
    desired.freq = SOUND_SAMPLING_RATE;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1; // mono
    desired.samples = 2048; // buffer-size
    desired.callback = sdl_callback; // called periodically by SDL to refill the buffer
    desired.userdata = this;

    SDL_AudioSpec obtained;

    if (SDL_OpenAudio(&desired, &obtained) != 0) {
        SPDLOG_ERROR("Failed to open sound device : {}", SDL_GetError());
        _is_audio_initialized = false;
    }

    SPDLOG_INFO("SDL audio subsystem initialized.");
    _is_audio_initialized = true;
    return _is_audio_initialized;
}

bool SoundPlayer::is_initialized() {
    return _is_audio_initialized;
}

bool SoundPlayer::add_sound(ASound *sound) {
    auto it = std::find(_sounds.begin(), _sounds.end(), sound);
    if (it != _sounds.end())
        return false;
    _sounds.push_back(sound);
    return true;
}

bool SoundPlayer::remove_sound(ASound *sound) {
    auto it = std::find(_sounds.begin(), _sounds.end(), sound);
    if (it == _sounds.end())
        return false;
    _sounds.erase(it);
    return true;
}

void SoundPlayer::sdl_callback(void *instance, uint8_t *raw_buffer, int bytes) {
    SoundPlayer *player = static_cast<SoundPlayer *>(instance);
    int16_t *buffer = reinterpret_cast<int16_t *>(raw_buffer);

    uint32_t len = bytes / sizeof(int16_t);
    for (uint32_t i = 0 ; i < len ; i++) {
        double time = static_cast<double>(player->_sample_n) / SOUND_SAMPLING_RATE;

        buffer[i] = 0;
        for (ASound *sound : player->_sounds) {
            buffer[i] += sound->synthesize(SoundSynthesisData{player->_sample_n, time});
        }

        player->_sample_n++;
    }
}

void SoundPlayer::play() {
    SDL_PauseAudio(0);
}

void SoundPlayer::pause() {
    SDL_PauseAudio(1);
}

} // namespace tools::sdl