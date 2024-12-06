cmake_minimum_required(VERSION 3.9)
project(opbox_software)

# GoogleTest requires at least C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Compile src files
#

add_library(opbox_lib
    src/opboxio.cpp)
target_include_directories(opbox_lib PUBLIC include)

add_executable(opbox_main src/main.cpp)
target_include_directories(opbox_main PUBLIC include)
target_link_libraries(opbox_main opbox_lib)

#
# Compile test files
#

file(COPY test/test_files
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(GLOB test_src test/*.cpp)
add_executable(opbox_test ${test_src})
target_include_directories(opbox_test PUBLIC include)
target_link_libraries(opbox_test gtest opbox_lib)
