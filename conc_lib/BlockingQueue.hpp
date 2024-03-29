//
// Created by Thomas Brooks on 1/21/24.
//

#ifndef CONC_DEV_BLOCKINGQUEUE_HPP
#define CONC_DEV_BLOCKINGQUEUE_HPP

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>
#include "Lock.hpp"


namespace conc {
    template<typename ElemT>
    class BlockingQueue {
    public:
        virtual bool offer(const ElemT &element, uint32_t timeout) = 0;

        virtual bool offer(const ElemT &element) = 0;

        virtual void put(const ElemT &element) = 0;

        virtual std::optional<ElemT> poll(uint32_t timeout) = 0;

        virtual std::optional<ElemT> poll() = 0;

        virtual ElemT take() = 0;
    };

    template<typename DerivedLockT> concept IsUniqueLock_ =
    std::is_base_of<std::unique_lock<std::mutex>, DerivedLockT>::value;

    template<typename ElemT, uint32_t Size, IsUniqueLock_ LockT>
    class SimpleBlockingQueue_ : public BlockingQueue<ElemT> {
    public:
        bool offer(const ElemT &element, uint32_t timeout) override;

        bool offer(const ElemT &element) override;

        void put(const ElemT &element) override;

        std::optional<ElemT> poll(uint32_t timeout) override;

        std::optional<ElemT> poll() override;

        ElemT take() override;

    protected:
        virtual bool is_full() = 0;

        virtual bool is_empty() = 0;

        virtual LockT lock_on_insert() = 0;

        virtual LockT lock_on_remove() = 0;

        std::queue<ElemT> elements;
        std::mutex queue_mutex;
    private:
        std::condition_variable not_full_cv;
        std::condition_variable not_empty_cv;
    };

    template<typename ElemT>
    class SynchronousQueue : public SimpleBlockingQueue_<ElemT, 0, LockWithHooks<std::mutex>> {
    protected:
        bool is_full() override;

        bool is_empty() override;

        LockWithHooks<std::mutex> lock_on_insert() override;

        LockWithHooks<std::mutex> lock_on_remove() override;

    private:
        uint32_t consumers_waiting;
        uint32_t producers_waiting;
    };

    template<typename ElemT, uint32_t Size>
    class ThickBlockingQueue : public SimpleBlockingQueue_<ElemT, Size, std::unique_lock<std::mutex>> {
        static_assert(Size > 0, "Size must be positive");
    protected:
        bool is_full() override;

        bool is_empty() override;

        std::unique_lock<std::mutex> lock_on_insert() override;

        std::unique_lock<std::mutex> lock_on_remove() override;
    };

    template<typename T> concept Delayable_ = requires(T t) {
        { t.get_delay() } -> std::same_as<uint64_t>;
    };

    // Wraps elements submitted to a DelayQueue in order to convert relative time to absolute time, while preserving
    // the item initially submitted to the queue
    template<Delayable_ ElemT>
    class DelayQueueElement_ {
    public:
        explicit DelayQueueElement_(const ElemT &inner);

        DelayQueueElement_(const DelayQueueElement_ &other);

        DelayQueueElement_(DelayQueueElement_ &&other) noexcept;

        uint64_t get_delay();

    private:
        uint64_t delay;
        const ElemT inner;
    };

    template<Delayable_ ElemT>
    class DelayQueue : public BlockingQueue<ElemT> {
    public:
        bool offer(const ElemT &element, uint32_t timeout) override;

        bool offer(const ElemT &element) override;

        void put(const ElemT &element) override;

        std::optional<ElemT> poll(uint32_t timeout) override;

        std::optional<ElemT> poll() override;

        ElemT take() override;

    private:
        std::priority_queue<DelayQueueElement_<ElemT>> queue;
        std::mutex queue_mutex;
    };
}

#include "BlockingQueue.cpp"

#endif //CONC_DEV_BLOCKINGQUEUE_HPP
