#include "Scheduler.hpp"
#include "spdlog/spdlog.h"

#include <thread>

namespace tools::utils {

Scheduler::Scheduler() {}
Scheduler::~Scheduler() {}

bool Scheduler::add_task(Task task) {
    if (is_running() || task.name.empty() || !task.task)
        return false;

    _tasks.push_back(task);
    return true;
}

void Scheduler::start() {
    auto now = std::chrono::steady_clock::now();
    for (auto &e : _tasks)
        e.next_run = now + e.delay_ns;
    _running = true;
    loop();
}

void Scheduler::stop() {
    _running = false;
}

void Scheduler::loop() {
    while (is_running()) {
        for (auto &e : _tasks) {
            if (!is_running())
                break;

            auto now = std::chrono::steady_clock::now();
            if (now > e.next_run) {
                if (!e.task())
                    SPDLOG_ERROR("Task '{}' returned false.", e.name);
                e.next_run = now + e.delay_ns;
            }
        }
        if (!_high_precision) {
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }
}

bool Scheduler::is_running() {
    return _running;
}

bool Scheduler::get_high_precision() {
    return _high_precision;
}

void Scheduler::set_high_precision(bool high_precision) {
    _high_precision = high_precision;
}

} // namespace tools::utils