#ifndef CONC_THREAD_POOL_HPP
#define CONC_THREAD_POOL_HPP

#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <semaphore>
#include <thread>


namespace conc {
    class ThreadPool_ {
    public:
        virtual ~ThreadPool_();

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


    template <typename T> concept IsThreadPool_ = std::is_base_of<ThreadPool_, T>::value;
    template <IsThreadPool_ T = ThreadPool_> using ThreadPool = std::shared_ptr<T>;

    class FixedThreadPool_ : public ThreadPool_, public std::enable_shared_from_this<FixedThreadPool_> {
    public:
        friend std::shared_ptr<FixedThreadPool_> make_fixed_thread_pool(uint16_t nthreads);

        ~FixedThreadPool_() override;

        void shutdown(bool join) override;
        void shutdown_now(bool join) override;
        void submit(const std::function<void ()> &job) override;
    private:
        static void run_thread(std::shared_ptr<FixedThreadPool_> &pool);

        explicit FixedThreadPool_(uint16_t nthreads);

        uint16_t nthreads;
        std::condition_variable runner_cv;
        std::condition_variable safe_shutdown_cv;
        std::queue<std::function<void()>> task_queue;
        std::mutex tasks_mutex;
    };

    conc::ThreadPool<FixedThreadPool_> make_fixed_thread_pool(uint16_t nthreads);

    class CachedThreadPool_ : public ThreadPool_ {
    public:
        friend std::shared_ptr<CachedThreadPool_> make_cached_thread_pool(uint16_t thread_timeout);

        ~CachedThreadPool_() override;

        void shutdown(bool join) override;
        void shutdown_now(bool join) override;
        void submit(const std::function<void ()> &job) override;
    private:
        static void run_thread(std::shared_ptr<CachedThreadPool_> &pool);

        explicit CachedThreadPool_(uint16_t thread_timeout);

        uint16_t thread_timeout;
    };

    conc::ThreadPool<CachedThreadPool_> make_cached_thread_pool(uint16_t thread_timeout);
}

#endif //CONC_THREAD_POOL_HPP
