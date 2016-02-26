project(drm_wrapper)

find_package(DRM REQUIRED)
find_package(KMS REQUIRED)

include_directories(
    ${DRM_INCLUDE_DIR}
    ${KMS_INCLUDE_DIR}
    ${DK_UTILS_INCLUDE_DIRS}
    ../xf86drm
)

add_definitions(
    -std=gnu++11
)

aux_source_directory(. SRC)

add_library(${PROJECT_NAME} STATIC ${SRC})

target_link_libraries(${PROJECT_NAME}
    ${DRM_LIBRARIES}
    ${KMS_LIBRARIES}
    dk_utils
)
