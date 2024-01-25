//
// Created by ElemThomas Brooks on 1/21/24.
//

#include <chrono>
#include "BlockingQueue.hpp"


/*************************************************************************************************
 ****************************************** BlockingQueue ****************************************
 *************************************************************************************************
 */

template <typename ElemT> bool conc::BlockingQueue<ElemT>::offer(const ElemT &element) {
    return offer(element, 0);
}

template <typename ElemT> std::optional<ElemT> conc::BlockingQueue<ElemT>::poll() {
    return poll(0);
}


/**************************************************************************************************
 ****************************************** BlockingQueue_ ****************************************
 **************************************************************************************************
 */

template <typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
bool conc::BlockingQueue_<ElemT, Size, LockT>::offer(const ElemT &element, uint32_t timeout) {
    {
        LockT lk(lock_on_insert());

        if (is_full() && (timeout == 0 || !not_full_cv.wait_for(
                lk,
                std::chrono::milliseconds(timeout),
                [this] -> bool { return !is_full(); }))) {
            return false;
        }
        elements.emplace(element);
    }
    not_empty_cv.notify_one();
    return true;
}

template <typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
void conc::BlockingQueue_<ElemT, Size, LockT>::put(const ElemT &element) {
    {
        LockT lk(lock_on_insert());

        if (is_full()) {
            not_full_cv.wait(lk, [this] -> bool { return !is_full(); });
        }
        elements.emplace(element);
    }
    not_empty_cv.notify_one();
}

template <typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
std::optional<ElemT> conc::BlockingQueue_<ElemT, Size, LockT>::poll(uint32_t timeout) {
    ElemT element;
    {
        LockT lk(lock_on_remove());

        if (is_empty() && (timeout == 0 || !not_empty_cv.wait_for(
                lk,
                std::chrono::milliseconds(timeout),
                [this] -> bool { return !is_empty(); }))) {
            return std::nullopt;
        }
        element = std::move(elements.front());
        elements.pop();
    }
    not_full_cv.notify_one();
    return element;
}

template <typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
ElemT conc::BlockingQueue_<ElemT, Size, LockT>::take() {
    ElemT element;
    {
        LockT lk(lock_on_remove());

        if (is_empty()) {
            not_empty_cv.wait(lk, [this] -> bool { return !is_empty(); });
        }
        element = std::move(elements.front());
        elements.pop();
    }
    not_full_cv.notify_one();
    return element;
}


/****************************************************************************************************
 ****************************************** SynchronousQueue ****************************************
 ****************************************************************************************************
 */

template <typename ElemT> bool conc::SynchronousQueue<ElemT>::is_full() {
    return consumers_waiting == 0;
}

template<typename ElemT> bool conc::SynchronousQueue<ElemT>::is_empty() {
    return producers_waiting == 0;
}

template<typename ElemT> conc::LockWithHooks<std::mutex> conc::SynchronousQueue<ElemT>::lock_on_insert() {
    return LockWithHooks<std::mutex>(
            this->queue_mutex,
            [this] -> void { producers_waiting++; },
            [this] -> void { producers_waiting--; });
}

template<typename ElemT> conc::LockWithHooks<std::mutex> conc::SynchronousQueue<ElemT>::lock_on_remove() {
    return LockWithHooks<std::mutex>(
            this->queue_mutex,
            [this] -> void { consumers_waiting++; },
            [this] -> void { consumers_waiting--; });
}


/******************************************************************************************************
 ****************************************** ThickBlockingQueue ****************************************
 ******************************************************************************************************
 */

template <typename ElemT, uint32_t Size> bool conc::ThickBlockingQueue<ElemT, Size>::is_full() {
    return this->elements.size() >= Size;
}

template<typename ElemT, uint32_t Size> bool conc::ThickBlockingQueue<ElemT, Size>::is_empty() {
    return this->elements.size() == 0;
}

template<typename ElemT, uint32_t Size>
std::unique_lock<std::mutex> conc::ThickBlockingQueue<ElemT, Size>::lock_on_insert() {
    return std::unique_lock<std::mutex>(this->queue_mutex);
}

template<typename ElemT, uint32_t Size>
std::unique_lock<std::mutex> conc::ThickBlockingQueue<ElemT, Size>::lock_on_remove() {
    return lock_on_insert();
}
