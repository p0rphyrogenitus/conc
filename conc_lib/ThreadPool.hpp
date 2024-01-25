#ifndef CONC_THREAD_POOL_HPP
#define CONC_THREAD_POOL_HPP

#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <semaphore>
#include <thread>
#include "BlockingQueue.hpp"


namespace conc {
    class ThreadPool_ {
    public:
        [[nodiscard]] bool is_safe_shutdown_started() const;

        [[nodiscard]] bool is_shutdown() const;

        [[nodiscard]] bool is_terminated() const;

        virtual void shutdown(bool join) = 0;

        virtual void shutdown_now(bool join) = 0;

        virtual void submit(const std::function<void()> &job) = 0;

    protected:
        bool is_safe_shutdown_started_;
        bool is_shutdown_;
        bool is_terminated_;
        std::list<std::thread> threads;
    };


    template<typename DerivedPoolT> concept IsThreadPool_ = std::is_base_of<ThreadPool_, DerivedPoolT>::value;
    template<IsThreadPool_ DerivedPoolT = ThreadPool_> using ThreadPool = std::shared_ptr<DerivedPoolT>;

    class FixedThreadPool_ : public ThreadPool_, public std::enable_shared_from_this<FixedThreadPool_> {
    public:
        friend std::shared_ptr<FixedThreadPool_> make_fixed_thread_pool(uint16_t nthreads);

        void shutdown(bool join) override;

        void shutdown_now(bool join) override;

        void submit(const std::function<void()> &job) override;

    private:
        static void run_thread(std::shared_ptr<FixedThreadPool_> &pool);

        explicit FixedThreadPool_(uint16_t nthreads);

        uint16_t nthreads;
        std::condition_variable runner_cv;
        std::condition_variable safe_shutdown_cv;
        std::queue<std::function<void()>> task_queue;
        std::mutex tasks_mutex;
    };

    ThreadPool<FixedThreadPool_> make_fixed_thread_pool(uint16_t nthreads);

    class CachedThreadPool_ : public ThreadPool_, public std::enable_shared_from_this<CachedThreadPool_> {
    public:
        friend std::shared_ptr<CachedThreadPool_> make_cached_thread_pool(uint16_t thread_idle_timeout);

        void shutdown(bool join) override;

        void shutdown_now(bool join) override;

        void submit(const std::function<void()> &job) override;

    private:
        static void run_thread(std::shared_ptr<CachedThreadPool_> &pool, std::function<void()> &initial_job);

        explicit CachedThreadPool_(uint16_t thread_idle_timeout);

        uint16_t thread_idle_timeout;
        SynchronousQueue<std::function<void()>> job_queue;
        std::mutex shutdown_or_thread_mod_mutex;
    };

    ThreadPool<CachedThreadPool_> make_cached_thread_pool(uint16_t thread_idle_timeout);
}

#endif //CONC_THREAD_POOL_HPP
