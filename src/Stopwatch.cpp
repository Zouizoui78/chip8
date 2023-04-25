#include "Stopwatch.hpp"
#include "spdlog/spdlog.h"

namespace tools::utils {

Stopwatch::Stopwatch(const std::string &name) {
    _name = name;
    reset();
}

uint64_t Stopwatch::get_duration() const {
    return (std::chrono::steady_clock::now() - _start_time_point).count();
}

void Stopwatch::log_duration() const {
    double d = get_duration();
    std::string prefix = "";
    std::string unit = " ns";

    if (!_name.empty())
        prefix = _name + " : ";

    if (d > 1e9) {
        d /= 1e9;
        unit = "s";
    }
    else if (d > 1e6) {
        d /= 1e6;
        unit = "ms";
    }
    else if (d > 1e3) {
        d /= 1e3;
        unit = "us";
    }

    SPDLOG_INFO("{}{:.3f} {}", prefix, d, unit);
}

void Stopwatch::reset() {
    _start_time_point = std::chrono::steady_clock::now();
}

} // namespace tools::utils