project(conc)

set(HEADER_FILES
        ThreadPool.hpp
)

set(SOURCE_FILES
        ThreadPool.cpp
)

add_library(conc_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})
