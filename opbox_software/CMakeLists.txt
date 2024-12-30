cmake_minimum_required(VERSION 3.9)
project(opbox_software)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# GoogleTest requires at least C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Misc. Requirements
#
find_package(ament_cmake REQUIRED)
find_package(serial_library REQUIRED)
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Qt5 REQUIRED COMPONENTS Widgets)

#
# Opbox Software compilation
#

set(THREADS_PREFER_PTHREAD_FLAG ON)

#
# libnotify
#
pkg_check_modules(LIBNOTIFY REQUIRED libnotify)

#
# yaml-cpp
#
include(FetchContent)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG d45c4fba3eccb318441ceeccca67345b0dd21a80 # Can be a tag (yaml-cpp-x.x.x), a commit hash, or a branch name (master)
)
FetchContent_MakeAvailable(yaml-cpp)

#
# Compile src files for opbox lib
#

add_library(opbox_lib
    src/opboxio.cpp
    src/opboxutil.cpp
    src/opboxcomms.cpp
    src/opboxlinux.cpp)

target_include_directories(opbox_lib PUBLIC include ${LIBNOTIFY_INCLUDE_DIRS})
target_link_libraries(opbox_lib ${LIBNOTIFY_LIBRARIES})

ament_target_dependencies(opbox_lib
    serial_library)

target_compile_options(opbox_lib PRIVATE ${LIBNOTIFY_CFLAGS_OTHER})

#
# opboxalert
#

add_executable(opbox_alert src/opboxalert.cpp)
target_link_libraries(opbox_alert
    PUBLIC opbox_lib Qt5::Widgets)

#
# opboxmain
#

add_executable(opbox_main src/main.cpp)
target_link_libraries(opbox_main 
    PUBLIC opbox_lib yaml-cpp::yaml-cpp
    PRIVATE Threads::Threads)

#
# testnotification
#

add_executable(test_notification src/testnotification.cpp)
target_include_directories(test_notification PUBLIC ${LIBNOTIFY_INCLUDE_DIRS})
target_link_libraries(test_notification PUBLIC ${LIBNOTIFY_LIBRARIES})
target_compile_options(test_notification PRIVATE ${LIBNOTIFY_CFLAGS_OTHER})

#
# testopboxalert
#
add_executable(test_opboxalert src/testopboxalert.cpp)
target_link_libraries(test_opboxalert
    PUBLIC opbox_lib)

#
# testopboxbrowser
#
add_executable(test_opboxbrowser src/testopboxbrowser.cpp)
target_link_libraries(test_opboxbrowser
    PUBLIC opbox_lib)

#
# installs
#
install(TARGETS opbox_alert opbox_main test_notification test_opboxalert test_opboxbrowser
    DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY test/test_files
    DESTINATION share/${PROJECT_NAME})

#
# Compile test files
#
if(BUILD_TESTING)
    # Copy test files to build dir for development testing
    file(COPY test/test_files
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    # Install test files to install dir for production testing
    install(DIRECTORY test/test_files
        DESTINATION ${PROJECT_NAME})
    file(GLOB test_src test/*.cpp)
    add_executable(opbox_test ${test_src})
    target_include_directories(opbox_test PUBLIC include)
    target_link_libraries(opbox_test
        PUBLIC gtest opbox_lib
        PRIVATE Threads::Threads)
    install(TARGETS opbox_test
        DESTINATION lib/${PROJECT_NAME})
endif()

#
# Copy/install config directory to build/install so program can access the YAML
#
file(COPY config
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
install(DIRECTORY config
    DESTINATION ${PROJECT_NAME})

ament_package()
