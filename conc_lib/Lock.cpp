//
// Created by Thomas Brooks on 1/24/24.
//

#include <Lock.hpp>


/*********************************************************************************************************
 ****************************************** LockWithHooks ****************************************
 *********************************************************************************************************
 */

template <typename T>
conc::LockWithHooks<T>::LockWithHooks(T &mutex,
                                      const std::function<void()> &on_lock,
                                      const std::function<void()> &on_unlock)
                                      : std::unique_lock<T>(mutex), on_unlock(on_unlock) {
    on_lock();
}

template <typename T> conc::LockWithHooks<T>::LockWithHooks(
        LockWithHooks<T> &&other) noexcept : std::unique_lock<T>(std::move(other)), on_unlock(other.on_unlock) {
}

template <typename T> conc::LockWithHooks<T>::~LockWithHooks() {
    on_unlock();
}