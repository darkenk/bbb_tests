project(wsegl)

aux_source_directory(. SRC_LIST)

find_package(PVR2D REQUIRED)
find_package(GBM REQUIRED)
find_package(KMS REQUIRED)
find_package(Wayland COMPONENTS Egl Client)

include_directories(SYSTEM
    ./sgx_include
    ${GBM_INCLUDE_DIRS}
    ${KMS_INCLUDE_DIRS}/libkms
    ${DK_UTILS_INCLUDE_DIRS}
    ${Wayland_INCLUDE_DIRS}
)

add_definitions(-std=c++11
    -fvisibility=hidden)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
target_link_libraries(${PROJECT_NAME}
    ${PVR2D_LIBRARIES}
    ${GBM_LIBRARIES}
    ${KMS_LIBRARIES}
)

aux_source_directory(wrapper SRC_LIST2)
add_library(wsegl_wrapper SHARED ${SRC_LIST2})
target_link_libraries(wsegl_wrapper ${PVR2D_LIBRARIES})

