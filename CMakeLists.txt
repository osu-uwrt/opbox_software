cmake_minimum_required(VERSION 3.9)
project(opbox_software)

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add serial library
add_subdirectory(serial_library)

# add opbox software
add_subdirectory(opbox_software)