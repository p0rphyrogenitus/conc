project(conc)

set(HEADER_FILES
        ThreadPool.hpp
        BlockingQueue.hpp
        Lock.hpp
)

set(SOURCE_FILES
        ThreadPool.cpp
        BlockingQueue.cpp
        Lock.cpp
)

add_library(conc_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})
