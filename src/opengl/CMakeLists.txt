project(opengl)

find_package(ECM 1.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${ECM_MODULE_PATH}")

find_package(DRM REQUIRED)
find_package(KMS REQUIRED)
find_package(GBM REQUIRED)
find_package(UDEV REQUIRED)
find_package(EGL REQUIRED)
find_package(GLESv2 REQUIRED)
include_directories(
    ${DRM_INCLUDE_DIR}
    ${KMS_INCLUDE_DIR}
    ${GBM_INCLUDE_DIRS}
    ${DK_UTILS_INCLUDE_DIRS}
    ${EGL_INCLUDE_DIRS}
)

add_definitions(
    -std=gnu++11
)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME}
    ${GBM_LIBRARIES}
    ${UDEV_LIBRARIES}
    ${DRM_LIBRARIES}
    ${KMS_LIBRARIES}
    dk_utils
    drm_wrapper
    ${CMAKE_DL_LIBS}
    ${EGL_LIBRARIES}
    ${GLESv2_LIBRARIES}
)
