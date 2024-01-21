//
// Created by Thomas Brooks on 1/20/24.
//

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <chrono>
#include <iostream>
#include "ThreadPool.hpp"


BOOST_AUTO_TEST_CASE(Validate) {
    conc::FixedThreadPool thread_pool(10);

    for (int i = 0; i < 100; i++) {
        thread_pool.submit([i]() {
            std::cout << i << std::endl;
        });
    }

    thread_pool.shutdown_now();
    std::this_thread::sleep_for(std::chrono::seconds(3));
}
