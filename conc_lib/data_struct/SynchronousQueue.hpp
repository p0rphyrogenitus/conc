//
// Created by Thomas Brooks on 1/21/24.
//

#ifndef CONC_DEV_SYNCHRONOUSQUEUE_HPP
#define CONC_DEV_SYNCHRONOUSQUEUE_HPP

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>


namespace conc {
    template <typename T> class SynchronousQueue {
    public:
        SynchronousQueue();
        bool offer(const T &element, int32_t timeout = 0);
        void put(const T &element);
        std::optional<T> poll(int32_t timeout = 0);
        T take();
    private:
        std::vector<T> elements;
        std::condition_variable enqueued_cv;
        std::mutex queue_mutex;
        std::condition_variable consumer_waiting_cv;
        std::mutex consumer_mutex;
        bool consumer_waiting;
    };
}


#endif //CONC_DEV_SYNCHRONOUSQUEUE_HPP
