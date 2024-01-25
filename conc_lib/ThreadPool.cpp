#include "ThreadPool.hpp"


/***********************************************************************************************
 ****************************************** ThreadPool_ ****************************************
 ***********************************************************************************************
 */

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
    ThreadPool<FixedThreadPool_> pool_ptr(new FixedThreadPool_(nthreads));

    for (; nthreads > 0; --nthreads) {
        pool_ptr->threads.emplace_back([pool_ptr]() mutable -> void {
            FixedThreadPool_::run_thread(pool_ptr);
        });
    }

    return pool_ptr;
}

void conc::FixedThreadPool_::shutdown(bool join) {
    std::thread shutdown_thread([pool = shared_from_this()] {
        {
            std::unique_lock<std::mutex> lk(pool->tasks_mutex);
            if (pool->is_shutdown_ || pool->is_safe_shutdown_started_) {
                return;
            }
            pool->is_safe_shutdown_started_ = true;
            if (!pool->task_queue.empty()) {
                pool->safe_shutdown_cv.wait(lk, [&pool] -> bool {
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
    for (std::thread &active_thread: threads) {
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

void conc::FixedThreadPool_::run_thread(ThreadPool<FixedThreadPool_> &pool) {
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
            job = std::move(pool->task_queue.front());
            pool->task_queue.pop();
            should_start_safe_shutdown = pool->is_safe_shutdown_started_ && pool->task_queue.empty();
        }

        try {
            job();
        } catch (...) {}

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

conc::ThreadPool<conc::CachedThreadPool_> conc::make_cached_thread_pool(uint16_t thread_idle_timeout) {
    return ThreadPool<CachedThreadPool_>(new CachedThreadPool_(thread_idle_timeout));
}

void conc::CachedThreadPool_::shutdown(bool join) {
    shutdown_now(join);
}

void conc::CachedThreadPool_::shutdown_now(bool join) {
    {
        std::lock_guard<std::mutex> lk(shutdown_or_thread_mod_mutex);
        if (is_shutdown_) {
            return;
        }
        is_safe_shutdown_started_ = is_shutdown_ = true;
    }

    for (std::thread &active_thread: threads) {
        if (join) {
            active_thread.join();
        } else {
            active_thread.detach();
        }
    }

    is_terminated_ = join;
    threads.clear();
}

void conc::CachedThreadPool_::submit(const std::function<void()> &job) {
    std::lock_guard<std::mutex> lk(shutdown_or_thread_mod_mutex);
    if (is_shutdown_) {
        return;
    }

    if (!job_queue.offer(job)) {
        threads.emplace_back([pool_ptr = shared_from_this(), initial_job = job] mutable -> void {
            CachedThreadPool_::run_thread(pool_ptr, initial_job);
        });
    }
}

void conc::CachedThreadPool_::run_thread(ThreadPool<CachedThreadPool_> &pool,
                                         std::function<void()> &initial_job) {
    std::optional<std::function<void()>> job = std::move(initial_job);
    while (true) {
        // job will actually always contain a value. See exit condition below.
        try {
            job.value_or([] {})();
        } catch (...) {}

        if ((job = pool->job_queue.poll(pool->thread_idle_timeout)) == std::nullopt) {
            // Start new thread to safely erase this running thread, if appropriate. Immediately detach new thread.
            std::thread([pool, thread_to_erase = std::this_thread::get_id()] -> void {
                std::lock_guard<std::mutex> lk(pool->shutdown_or_thread_mod_mutex);

                // If the pool is shut down, then the shutdown thread will do the erasing.
                if (!pool->is_shutdown_) {
                    auto thread_to_erase_it = std::find_if(
                            pool->threads.begin(),
                            pool->threads.end(),
                            [thread_to_erase](const std::thread &thr) {
                                return thr.get_id() == thread_to_erase;
                            });
                    if (thread_to_erase_it != pool->threads.end()) {
                        // Whether we detach or join is irrelevant since we are immediately returning after detaching
                        // this current thread (that is, the child-most thread). That said, join() more clearly
                        // expresses that the thread of execution in question has completed its computation.
                        thread_to_erase_it->join();
                        pool->threads.erase(thread_to_erase_it);
                    }
                }
            }).detach();

            return;
        }
    }
}

conc::CachedThreadPool_::CachedThreadPool_(uint16_t thread_idle_timeout) : thread_idle_timeout(thread_idle_timeout) {
}
