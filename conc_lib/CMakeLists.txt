project(conc)

set(HEADER_FILES
        ThreadPool.hpp
        data_struct/SynchronousQueue.hpp
)

set(SOURCE_FILES
        ThreadPool.cpp
        data_struct/SynchronousQueue.cpp
)

add_library(conc_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})
