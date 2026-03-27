#include "thread_pool.hpp"

namespace wfc {

ThreadPool::ThreadPool(size_t num_threads)
: stop(false), active_tasks(0)
{
    if (num_threads == 0) {
        num_threads = std::thread::hardware_concurrency();
        if (num_threads == 0) num_threads = 4;
    }

    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this]() {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);

                    condition.wait(lock, [this]() {
                        return stop || !tasks.empty();
                    });

                    if (stop && tasks.empty())
                        return;

                    task = std::move(tasks.front());
                    tasks.pop();

                    active_tasks++;
                }

                task();

                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    active_tasks--;

                    if (tasks.empty() && active_tasks == 0) {
                        finished_condition.notify_all();
                    }
                }
            }
        });
    }
}


void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queue_mutex);

    finished_condition.wait(lock, [this]() {
        return tasks.empty() && active_tasks == 0;
    });
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    condition.notify_all();

    for (std::thread &worker : workers) {
        if (worker.joinable())
            worker.join();
    }
}

}

