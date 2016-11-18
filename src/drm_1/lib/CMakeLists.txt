project(drm_wrapper)

find_package(DRM REQUIRED)
find_package(KMS REQUIRED)
find_package(GBM REQUIRED)

include_directories(
    ${DRM_INCLUDE_DIRS}
    ${KMS_INCLUDE_DIRS}
    ${GBM_INCLUDE_DIRS}
    ${DK_UTILS_INCLUDE_DIRS}
    ../xf86drm
)

message(STATUS DRM:${DRM_INCLUDE_DIRS})

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
