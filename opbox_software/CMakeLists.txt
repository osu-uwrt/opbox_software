#
# Opbox Software compilation
#

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

#
# Compile src files
#

add_library(opbox_lib
    src/opboxio.cpp)
target_include_directories(opbox_lib PUBLIC include)

add_executable(opbox_main src/main.cpp)
target_include_directories(opbox_main PUBLIC include)
target_link_libraries(opbox_main 
    PUBLIC opbox_lib serial_library
    PRIVATE Threads::Threads)

#
# Compile test files
#

file(COPY test/test_files
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(GLOB test_src test/*.cpp)
add_executable(opbox_test ${test_src})
target_include_directories(opbox_test PUBLIC include)
target_link_libraries(opbox_test
    PUBLIC gtest opbox_lib
    PRIVATE Threads::Threads)
