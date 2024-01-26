project(conc)

# Header files with templated constructs #include their implementations
set(HEADER_FILES
        ThreadPool.hpp
        BlockingQueue.hpp
        Lock.hpp
)

# Only include files that don't #include their implementations
set(SOURCE_FILES
        ThreadPool.cpp
)

add_library(conc_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})
