project(bbb_tests)
cmake_minimum_required(VERSION 2.8.12)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE DEBUG CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_SOURCE_DIR}/cmake
    ${ECM_FIND_MODULE_DIR}
)

#set(ENABLE_TESTS ONb CACHE BOOL "Disable dk_utils tests" FORCE)
add_definitions(-std=c++11)
find_package(dk_utils REQUIRED)

if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    find_package(DRM REQUIRED)
    include_directories(
        ${DRM_INCLUDE_DIRS}
    )
    add_subdirectory(src)
endif()

option(ENABLE_UNIT_TESTS "Enable unit tests" OFF)
if(ENABLE_UNIT_TESTS)
    add_subdirectory(test)
endif()
