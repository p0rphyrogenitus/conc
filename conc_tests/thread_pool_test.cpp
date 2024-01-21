//
// Created by Thomas Brooks on 1/20/24.
//

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <atomic>
#include <boost/test/unit_test.hpp>
#include "ThreadPool.hpp"


BOOST_AUTO_TEST_CASE(FixedThreadPool_shutdown) {
    int ntasks = 100;
    std::atomic<int> counter(0);
    conc::ThreadPool<> thread_pool = conc::make_fixed_thread_pool(10);

    for (int i = 0; i < ntasks; i++) {
        thread_pool->submit([&counter]() {
            counter.fetch_add(1);
        });
    }

    thread_pool->shutdown(true);
    BOOST_CHECK_EQUAL(counter.load(), ntasks);
    BOOST_CHECK_EQUAL(thread_pool->is_safe_shutdown_started(), true);
    BOOST_CHECK_EQUAL(thread_pool->is_shutdown(), true);
    BOOST_CHECK_EQUAL(thread_pool->is_terminated(), true);
}
