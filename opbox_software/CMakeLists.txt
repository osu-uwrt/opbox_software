#
# Opbox Software compilation
#

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

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
# Compile src files
#

add_library(opbox_lib
    src/opboxio.cpp
    src/opboxutil.cpp
    src/opboxcomms.cpp)
target_include_directories(opbox_lib PUBLIC include)
target_link_libraries(opbox_lib PUBLIC serial_library)

add_executable(opbox_main src/main.cpp)
target_include_directories(opbox_main PUBLIC include)
target_link_libraries(opbox_main 
    PUBLIC opbox_lib yaml-cpp::yaml-cpp
    PRIVATE Threads::Threads)

#
# Compile test files
#

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

#
# Copy/install config directory to build/install so program can access the YAML
#
file(COPY config
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
install(DIRECTORY config
    DESTINATION ${PROJECT_NAME})
