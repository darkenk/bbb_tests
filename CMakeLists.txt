project(bbb_tests)
cmake_minimum_required(VERSION 2.8.12)
aux_source_directory(. SRC_LIST)

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

set(ENABLE_TESTS OFF CACHE BOOL "Disable dk_utils tests" FORCE)
find_package(dk_utils REQUIRED)
find_package(DRM REQUIRED)

include_directories(
    ${DRM_INCLUDE_DIRS}
)

#add_executable(${PROJECT_NAME} main.c)
add_definitions(-std=c++11)
add_subdirectory(src)
#target_link_libraries(${PROJECT_NAME} ${DRM_LIBRARIES})

