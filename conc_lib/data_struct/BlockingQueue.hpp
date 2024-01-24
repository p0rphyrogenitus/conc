//
// Created by Thomas Brooks on 1/21/24.
//

#ifndef CONC_DEV_BLOCKINGQUEUE_HPP
#define CONC_DEV_BLOCKINGQUEUE_HPP

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>


namespace conc {
    template <typename T> class BlockingQueue {
    public:
        virtual bool offer(const T &element);
        virtual bool offer(const T &element, uint32_t timeout) = 0;
        virtual void put(const T &element) = 0;
        virtual std::optional<T> poll();
        virtual std::optional<T> poll(uint32_t timeout) = 0;
        virtual T take() = 0;
    private:
    };

    template <typename T> class SynchronousQueue : public BlockingQueue<T> {
    public:
        SynchronousQueue();
        bool offer(const T &element, uint32_t timeout) override;
        void put(const T &element) override;
        std::optional<T> poll(uint32_t timeout) override;
        T take() override;
    private:
        std::vector<T> elements;
        std::condition_variable enqueued_cv;
        std::mutex queue_mutex;
        std::condition_variable consumer_waiting_cv;
        std::mutex consumer_mutex;
        bool consumer_waiting;
    };
}


#endif //CONC_DEV_BLOCKINGQUEUE_HPP
