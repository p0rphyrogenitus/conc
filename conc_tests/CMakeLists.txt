set(Boost_USE_STATIC_LIBS OFF)
find_package(Boost 1.76 REQUIRED COMPONENTS unit_test_framework)
include_directories(${Boost_INCLUDE_DIRS})

add_executable(Boost_Tests_run thread_pool_test.cpp)
target_link_libraries(Boost_Tests_run ${Boost_LIBRARIES})
target_link_libraries(Boost_Tests_run conc_lib)
