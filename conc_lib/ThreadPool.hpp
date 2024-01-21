#ifndef CONC_THREAD_POOL_HPP
#define CONC_THREAD_POOL_HPP

#include <cstdint>
#include <functional>
#include <list>
#include <mutex>
#include <queue>
#include <thread>


namespace conc {
    class ThreadPool {
    public:
        virtual ~ThreadPool();
        [[nodiscard]] bool is_shutdown() const;
        [[nodiscard]] bool is_terminated() const;
        virtual bool await_termination() = 0;
        virtual void shutdown() = 0;
        virtual void shutdown_now() = 0;
        virtual void submit(const std::function<void()> &job) = 0;
    protected:
        bool shutdown_;
        bool terminated;
        std::list<std::thread> threads;
        std::queue<std::function<void()>> jobs;
        std::mutex jobs_mutex;
    };


class FixedThreadPool : public ThreadPool {
    public:
        explicit FixedThreadPool(uint16_t nthreads);
        ~FixedThreadPool() override;
        bool await_termination() override;
        void shutdown() override;
        void shutdown_now() override;
        void submit(const std::function<void ()> &job) override;
    private:
        static void run_thread(std::shared_ptr<FixedThreadPool> &pool);

        std::condition_variable runner_cv;
    };
}

#endif //CONC_THREAD_POOL_HPP
