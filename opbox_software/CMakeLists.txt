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
find_package(ament_cmake)
if(ament_cmake_FOUND)
    find_package(serial_library REQUIRED)
else()
    add_subdirectory("../serial_library" serial_library)
endif()
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
add_subdirectory(yaml-cpp)

#
# Compile src files for opbox lib
#

add_library(opbox_lib
    src/opboxio.cpp
    src/opboxutil.cpp
    src/opboxcomms.cpp
    src/opboxlinux.cpp)

target_include_directories(opbox_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    ${LIBNOTIFY_INCLUDE_DIRS})

set(opbox_lib_libraries ${LIBNOTIFY_LIBRARIES} Threads::Threads)

if(NOT ament_cmake_FOUND)
    target_include_directories(opbox_lib PUBLIC
        "../serial_library/include")
    set(opbox_lib_libraries ${opbox_lib_libraries} serial_library)
else()
    target_compile_definitions(opbox_lib PUBLIC USE_AMENT_FILESYSTEM)
endif()

target_link_libraries(opbox_lib ${opbox_lib_libraries})

if(ament_cmake_FOUND)
    ament_target_dependencies(opbox_lib
        serial_library)
endif()

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

add_executable(opbox_main src/main.cpp src/opboxsettings.cpp)
target_link_libraries(opbox_main 
    PUBLIC opbox_lib yaml-cpp::yaml-cpp)

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

#export opbox lib properly
install(
    TARGETS opbox_lib
    EXPORT opbox_libTargets
    DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY include/
    DESTINATION include)

if(ament_cmake_FOUND)
    ament_export_targets(opbox_libTargets HAS_LIBRARY_TARGET)
    ament_export_dependencies(serial_library)
endif()

install(TARGETS opbox_alert opbox_main test_notification test_opboxalert test_opboxbrowser
    DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY test/test_files
    DESTINATION share/${PROJECT_NAME})

install(DIRECTORY config
    DESTINATION share/${PROJECT_NAME})

#
# Compile test files
#
if(BUILD_TESTING)
    file(GLOB test_src test/*.cpp)

    add_executable(opbox_test ${test_src})

    target_include_directories(opbox_test PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

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

if(ament_cmake_FOUND)
    ament_package()
endif()
