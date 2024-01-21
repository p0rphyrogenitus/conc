#include "ThreadPool.hpp"


/***********************************************************************************************
 ****************************************** ThreadPool_ ****************************************
 ***********************************************************************************************
 */

conc::ThreadPool_::~ThreadPool_() = default;

bool conc::ThreadPool_::is_safe_shutdown_started() const {
    return is_safe_shutdown_started_;
}

bool conc::ThreadPool_::is_shutdown() const {
    return is_shutdown_;
}

bool conc::ThreadPool_::is_terminated() const {
    return is_terminated_;
}


/****************************************************************************************************
 ****************************************** FixedThreadPool_ ****************************************
 ****************************************************************************************************
 */

conc::ThreadPool<conc::FixedThreadPool_> conc::make_fixed_thread_pool(uint16_t nthreads) {
    conc::ThreadPool<conc::FixedThreadPool_> pool_ptr(new FixedThreadPool_(nthreads));

    for (; nthreads > 0; --nthreads) {
        pool_ptr->threads.emplace_back([pool_ptr]() mutable -> void {
            FixedThreadPool_::run_thread(pool_ptr);
        });
    }

    return pool_ptr;
}

conc::FixedThreadPool_::~FixedThreadPool_() = default;

void conc::FixedThreadPool_::shutdown(bool join) {
    std::thread shutdown_thread([pool = shared_from_this()] {
        {
            std::unique_lock<std::mutex> lk(pool->tasks_mutex);
            if (pool->is_shutdown_ || pool->is_safe_shutdown_started_) {
                return;
            }
            pool->is_safe_shutdown_started_ = true;
            if (!pool->task_queue.empty()) {
                pool->safe_shutdown_cv.wait(lk,[&pool] -> bool {
                        return pool->task_queue.empty() && pool->is_safe_shutdown_started_;
                });
            }
        }
        pool->shutdown_now(true);
    });

    if (join) {
        shutdown_thread.join();
    } else {
        shutdown_thread.detach();
    }
}

void conc::FixedThreadPool_::shutdown_now(bool join) {
    {
        std::unique_lock<std::mutex> lk(tasks_mutex);
        if (is_shutdown_) {
            return;
        }
        is_shutdown_ = true;
    }
    runner_cv.notify_all();
    for (std::thread &active_thread : threads) {
        if (join) {
            active_thread.join();
        } else {
            // Safe since shared pointer remains alive in Lambda capture until all threads complete. However, this will
            // cause a memory leak if some thread fails to enter a terminal state.
            active_thread.detach();
        }
    }
    // All tasks known to be terminated only if they were joined
    is_terminated_ = join;
    threads.clear();
}

void conc::FixedThreadPool_::submit(const std::function<void()> &job) {
    {
        std::unique_lock<std::mutex> lk(tasks_mutex);
        if (!is_safe_shutdown_started_ && !is_shutdown_) {
            task_queue.emplace(job);
        }
    }
    runner_cv.notify_one();
}

void conc::FixedThreadPool_::run_thread(conc::ThreadPool<conc::FixedThreadPool_> &pool) {
    while (true) {
        bool should_start_safe_shutdown;
        std::function<void()> job;
        {
            std::unique_lock<std::mutex> lk(pool->tasks_mutex);
            pool->runner_cv.wait(lk, [&pool] -> bool {
                return !pool->task_queue.empty() || pool->is_shutdown_;
            });
            if (pool->is_shutdown_) {
                return;
            }
            job = pool->task_queue.front();
            pool->task_queue.pop();
            should_start_safe_shutdown = pool->is_safe_shutdown_started_ && pool->task_queue.empty();
        }
        job();

        if (should_start_safe_shutdown) {
            pool->safe_shutdown_cv.notify_all();
        }
    }
}

conc::FixedThreadPool_::FixedThreadPool_(uint16_t nthreads) : nthreads(nthreads) {
}


/*****************************************************************************************************
 ****************************************** CachedThreadPool_ ****************************************
 *****************************************************************************************************
 */

conc::ThreadPool<conc::CachedThreadPool_> conc::make_cached_thread_pool(uint16_t thread_timeout) {
    return conc::ThreadPool<conc::CachedThreadPool_>(new CachedThreadPool_(thread_timeout));
}

conc::CachedThreadPool_::~CachedThreadPool_() = default;

void conc::CachedThreadPool_::shutdown(bool join) {
    // TODO
}

void conc::CachedThreadPool_::shutdown_now(bool join) {
    // TODO
}

void conc::CachedThreadPool_::submit(const std::function<void ()> &job) {
    // TODO
}

void conc::CachedThreadPool_::run_thread(std::shared_ptr<CachedThreadPool_> &pool) {

}

conc::CachedThreadPool_::CachedThreadPool_(uint16_t thread_timeout) : thread_timeout(thread_timeout) {
}
