#ifndef SCHEDULER_HPP
#define SCHEDULER_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <string>

namespace tools::utils {

class Scheduler;

struct Task {
    std::string name;
    std::function<bool ()> task;
    std::chrono::nanoseconds delay_ns;

    private:
    std::chrono::steady_clock::time_point next_run;

    friend Scheduler;
};

class Scheduler {
    public:

    Scheduler();
    ~Scheduler();

    bool add_task(Task task);

    void start();
    void stop();

    bool is_running();

    void set_high_precision(bool high_precision);
    bool get_high_precision();

    private:

    void loop();

    bool _running = false;
    bool _high_precision = false;
    std::vector<Task> _tasks;
};

} // namespace tools::utils

#endif // SCHEDULER_HPP