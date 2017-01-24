find_package(GTest REQUIRED)

find_package(Wayland COMPONENTS Egl Client Server)
find_package(DRM)

include_directories(
    ${DK_UTILS_INCLUDE_DIRS}
    SYSTEM ${GTEST_INCLUDE_DIRS}
    SYSTEM ${GMOCK_INCLUDE_DIRS}
)

set(SRC "${CMAKE_CURRENT_SOURCE_DIR}/../src/wsegl")
set(INC "inc/;inc/libkms;${SRC};${SRC}/sgx_include;${Wayland_INCLUDE_DIRS};${KMS_INCLUDE_DIRS};${DRM_INCLUDE_DIRS}")
set(WSEGL_SRC "${SRC}/wsegl_plugin.cpp;${SRC}/drawable.cpp;${SRC}/gbmdrawable.cpp")
set(MOCK_SRC "mockdrm.cpp;mockpvr2d.cpp;mockdrm.cpp;mockwayland.cpp;mockkms.cpp")

add_gtest(buffer_test "buffer_test.cpp;${SRC}/drawable.cpp;mockpvr2d.cpp" "${INC}" "")
add_gtest(gbm_test "gbm_test.cpp;${SRC}/drawable.cpp;${SRC}/gbmdrawable.cpp;${MOCK_SRC};mocklibc.cpp" "${INC}" "")
add_gtest(drawable_test "drawable_test.cpp;${SRC}/drawable.cpp;mockwayland.cpp" "${INC}" "")
#add_gtest(wsegl_test "wsegl_test.cpp;${WSEGL_SRC};${MOCK_SRC}" "${INC}" "")
add_gtest(wayland_test "wayland_test.cpp;${SRC}/drawable.cpp;${SRC}/waylandwindow.cpp;${MOCK_SRC}"
    "${INC}" "${Wayland_LIBRARIES}")
