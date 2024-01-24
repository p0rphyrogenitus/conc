project(conc)

set(HEADER_FILES
        ThreadPool.hpp
        data_struct/BlockingQueue.hpp
)

set(SOURCE_FILES
        ThreadPool.cpp
        data_struct/BlockingQueue.cpp
)

add_library(conc_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})
