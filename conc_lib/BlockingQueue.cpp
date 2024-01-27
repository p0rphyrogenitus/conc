//
// Created by ElemThomas Brooks on 1/21/24.
//

#include <chrono>
#include "BlockingQueue.hpp"


/********************************************************************************************************
 ****************************************** SimpleBlockingQueue_ ****************************************
 ********************************************************************************************************
 */

template<typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
bool conc::SimpleBlockingQueue_<ElemT, Size, LockT>::offer(const ElemT &element, uint32_t timeout) {
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

template<typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
bool conc::SimpleBlockingQueue_<ElemT, Size, LockT>::offer(const ElemT &element) {
    return offer(element, 0);
}

template<typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
void conc::SimpleBlockingQueue_<ElemT, Size, LockT>::put(const ElemT &element) {
    {
        LockT lk(lock_on_insert());

        if (is_full()) {
            not_full_cv.wait(lk, [this] -> bool { return !is_full(); });
        }
        elements.emplace(element);
    }
    not_empty_cv.notify_one();
}

template<typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
std::optional<ElemT> conc::SimpleBlockingQueue_<ElemT, Size, LockT>::poll(uint32_t timeout) {
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

template<typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
std::optional<ElemT> conc::SimpleBlockingQueue_<ElemT, Size, LockT>::poll() {
    return poll(0);
}

template<typename ElemT, uint32_t Size, conc::IsUniqueLock_ LockT>
ElemT conc::SimpleBlockingQueue_<ElemT, Size, LockT>::take() {
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

template<typename ElemT>
bool conc::SynchronousQueue<ElemT>::is_full() {
    return consumers_waiting == 0;
}

template<typename ElemT>
bool conc::SynchronousQueue<ElemT>::is_empty() {
    return producers_waiting == 0;
}

template<typename ElemT>
conc::LockWithHooks<std::mutex> conc::SynchronousQueue<ElemT>::lock_on_insert() {
    return LockWithHooks<std::mutex>(
            this->queue_mutex,
            [this] -> void { producers_waiting++; },
            [this] -> void { producers_waiting--; });
}

template<typename ElemT>
conc::LockWithHooks<std::mutex> conc::SynchronousQueue<ElemT>::lock_on_remove() {
    return LockWithHooks<std::mutex>(
            this->queue_mutex,
            [this] -> void { consumers_waiting++; },
            [this] -> void { consumers_waiting--; });
}


/******************************************************************************************************
 ****************************************** ThickBlockingQueue ****************************************
 ******************************************************************************************************
 */

template<typename ElemT, uint32_t Size>
bool conc::ThickBlockingQueue<ElemT, Size>::is_full() {
    return this->elements.size() >= Size;
}

template<typename ElemT, uint32_t Size>
bool conc::ThickBlockingQueue<ElemT, Size>::is_empty() {
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


/******************************************************************************************************
 ****************************************** DelayQueueElement_ ****************************************
 ******************************************************************************************************
 */

template <conc::Delayable_ ElemT>
conc::DelayQueueElement_<ElemT>::DelayQueueElement_(const ElemT &inner) : inner(inner) {
    delay = inner.get_delay() + std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    // Handle uint64_t overflow
    if (delay < inner.get_delay()) {
        throw std::invalid_argument("exceeded maximum delay");
    }
}

template <conc::Delayable_ ElemT>
conc::DelayQueueElement_<ElemT>::DelayQueueElement_(const DelayQueueElement_<ElemT> &other)
    : inner(other.inner), delay(other.delay) {
}

template <conc::Delayable_ ElemT>
conc::DelayQueueElement_<ElemT>::DelayQueueElement_(DelayQueueElement_<ElemT> &&other) noexcept
    : inner(std::move(other.inner)), delay(other.delay) {
}

template <conc::Delayable_ ElemT>
uint64_t conc::DelayQueueElement_<ElemT>::get_delay() {
    return delay;
}


/**********************************************************************************************
 ****************************************** DelayQueue ****************************************
 **********************************************************************************************
 */



