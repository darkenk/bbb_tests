project(wsegl)

aux_source_directory(. SRC_LIST)

find_package(PVR2D REQUIRED)
find_package(GBM REQUIRED)
find_package(KMS REQUIRED)

include_directories(SYSTEM
    ./sgx_include
    ${GBM_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../thayama/libgbm
    ${KMS_INCLUDE_DIR}/libkms
    ${DK_UTILS_INCLUDE_DIRS}
)

add_definitions(-std=c++11)

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
target_link_libraries(${PROJECT_NAME}
    ${PVR2D_LIBRARY}
    ${GBM_LIBRARY}
)

aux_source_directory(wrapper SRC_LIST2)
add_library(wsegl_wrapper SHARED ${SRC_LIST2})
target_link_libraries(wsegl_wrapper ${PVR2D_LIBRARY})

