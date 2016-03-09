project(drm_1)

find_package(DRM REQUIRED)
find_package(KMS REQUIRED)
find_package(PVR2D REQUIRED)
include_directories(
    ${DRM_INCLUDE_DIR}
    ${KMS_INCLUDE_DIR}
    ${PVR2D_INCLUDE_DIR}
    ${DK_UTILS_INCLUDE_DIRS}
)

add_subdirectory(lib)

add_definitions(
    -std=gnu++11
)

#add_executable(${PROJECT_NAME} main_pvr2d.cpp)
add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    ${DRM_LIBRARIES}
    ${KMS_LIBRARIES}
    dk_utils
    drm_wrapper
    ${PVR2D_LIBRARY}
)
