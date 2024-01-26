//
// Created by Thomas Brooks on 1/24/24.
//

#ifndef CONC_DEV_LOCK_HPP
#define CONC_DEV_LOCK_HPP

#include <mutex>


namespace conc {
    template<typename T>
    class LockWithHooks : public std::unique_lock<T> {
    public:
        LockWithHooks(
                T &mutex,
                const std::function<void()> &on_lock,
                const std::function<void()> &on_unlock);

        LockWithHooks(LockWithHooks<T> &&other) noexcept;

        ~LockWithHooks();

    private:
        const std::function<void()> on_unlock;
    };
}

#include "Lock.cpp"

#endif //CONC_DEV_LOCK_HPP
