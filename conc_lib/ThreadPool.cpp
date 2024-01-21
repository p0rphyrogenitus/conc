#include "ThreadPool.hpp"

conc::ThreadPool::~ThreadPool() = default;

bool conc::ThreadPool::is_shutdown() const {
    return shutdown_;
}

bool conc::ThreadPool::is_terminated() const {
    return terminated;
}


void conc::FixedThreadPool::run_thread(std::shared_ptr<FixedThreadPool> &pool) {
    while (true) {
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lk(pool->jobs_mutex);
            pool->runner_cv.wait(lk, [&pool] -> bool {
                    return !pool->jobs.empty() || pool->shutdown_;
            });
            if (pool->shutdown_) {
                return;
            }
            job = pool->jobs.front();
            pool->jobs.pop();
        }
        job();
    }
}

conc::FixedThreadPool::FixedThreadPool(uint16_t nthreads) {
    for (uint16_t j = nthreads; j > 0; --j) {
        threads.emplace_back([this]() mutable -> void {
            FixedThreadPool::run_thread(this);
        });
    }
}

conc::FixedThreadPool::~FixedThreadPool() = default;

bool conc::FixedThreadPool::await_termination() {
    // TODO
    return false;
}

void conc::FixedThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lk(jobs_mutex);
        shutdown_ = true;
    }
    runner_cv.notify_all();
    for (std::thread &active_thread : threads) {
        active_thread.join();
    }
    terminated = true;
    threads.clear();
}

void conc::FixedThreadPool::shutdown_now() {
    {
        std::unique_lock<std::mutex> lk(jobs_mutex);
        shutdown_ = true;
    }
    runner_cv.notify_all();
    for (std::thread &active_thread : threads) {
        // Safe since shared pointer remains alive in Lambda capture until all threads complete. However, this will
        // cause a memory leak if some thread fails to enter a terminal state.
        active_thread.detach();
    }
    terminated = true;
    threads.clear();
}

void conc::FixedThreadPool::submit(const std::function<void()> &job) {
    {
        std::unique_lock<std::mutex> lk(jobs_mutex);
        if (!shutdown_) {
            jobs.emplace(job);
        }
    }
    runner_cv.notify_one();
}
