//
// Created by Thomas Brooks on 1/21/24.
//

#include <chrono>
#include "BlockingQueue.hpp"


/*************************************************************************************************
 ****************************************** BlockingQueue ****************************************
 *************************************************************************************************
 */

template <typename T> bool conc::BlockingQueue<T>::offer(const T &element) {
    return offer(element, 0);
}

template <typename T> std::optional<T> conc::BlockingQueue<T>::poll() {
    return poll(0);
}


/****************************************************************************************************
 ****************************************** SynchronousQueue ****************************************
 ****************************************************************************************************
 */

template <typename T> conc::SynchronousQueue<T>::SynchronousQueue() : consumer_waiting(false) {
}

template <typename T> bool conc::SynchronousQueue<T>::offer(const T &element, uint32_t timeout) {
    {
        std::unique_lock<std::mutex> queue_lock(queue_mutex);
    }
    std::unique_lock<std::mutex> consumer_lock(consumer_mutex);

    if (!consumer_waiting && (timeout == 0 || !consumer_waiting_cv.wait_for(
            consumer_lock,
            std::chrono::milliseconds(timeout),
            [this] -> bool { return consumer_waiting; }))) {
        return false;
    }

    {
        std::unique_lock<std::mutex> queue_lock(queue_mutex);
        elements.emplace_back(element);
    }
    enqueued_cv.notify_one();
    return true;
}

template <typename T> void conc::SynchronousQueue<T>::put(const T &element) {
    {
        std::unique_lock<std::mutex> queue_lock(queue_mutex);
    }
    std::unique_lock<std::mutex> consumer_lock(consumer_mutex);
    if (!consumer_waiting) {
        consumer_waiting_cv.wait(consumer_lock, [this] -> bool { return consumer_waiting; });
    }

    {
        std::unique_lock<std::mutex> queue_lock(queue_mutex);
        elements.emplace_back(element);
    }
    enqueued_cv.notify_one();
}

template <typename T> std::optional<T> conc::SynchronousQueue<T>::poll(uint32_t timeout) {
    {
        std::unique_lock<std::mutex> consumer_lock(consumer_mutex);
        consumer_waiting = true;
    }

    std::unique_lock<std::mutex> queue_lock(queue_mutex);
    consumer_waiting_cv.notify_one();
    if (elements.empty() && (timeout == 0 || !enqueued_cv.wait_for(
            queue_lock,
            std::chrono::milliseconds(timeout),
            [this] -> bool { return !elements.empty(); }))) {
        return std::nullopt;
    }

    {
        std::unique_lock<std::mutex> consumer_lock(consumer_mutex);
        consumer_waiting = false;
    }

    T element = std::move(elements[0]);
    elements.clear();
    return element;
}

template <typename T> T conc::SynchronousQueue<T>::take() {
    {
        std::unique_lock<std::mutex> consumer_lock(consumer_mutex);
        consumer_waiting = true;
    }

    std::unique_lock<std::mutex> queue_lock(queue_mutex);
    consumer_waiting_cv.notify_one();
    if (elements.empty()) {
        enqueued_cv.wait(queue_lock, [this] -> bool { return !elements.empty(); });
    }
    
    {
        std::unique_lock<std::mutex> consumer_lock(consumer_mutex);
        consumer_waiting = false;
    }

    T element = std::move(elements[0]);
    elements.clear();
    return element;
}
