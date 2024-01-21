cmake_minimum_required(VERSION 3.27)
project(conc_dev)

set(CMAKE_CXX_STANDARD 23)

include_directories(conc_lib)
add_subdirectory(conc_lib)
add_subdirectory(conc_tests)
