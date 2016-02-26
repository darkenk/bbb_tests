project(drm_1)
cmake_minimum_required(VERSION 2.8)

# set build type to debug as it is in development phase
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE DEBUG CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

find_package(DRM REQUIRED)
find_package(KMS REQUIRED)
include_directories(
    ${DRM_INCLUDE_DIR}
    ${KMS_INCLUDE_DIR}
)

add_definitions(
    -std=gnu++11
)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    ${DRM_LIBRARIES}
    ${KMS_LIBRARIES}
)
