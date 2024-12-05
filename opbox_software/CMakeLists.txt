project(opbox_software)
cmake_minimum_required(VERSION 3.18)

file(GLOB opbox_src src/*.cpp)
add_executable(opbox_main ${opbox_src})
target_include_directories(opbox_main PUBLIC include)
