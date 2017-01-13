#include "drawable.hpp"
#include "mockpvr2d.hpp"
#include <gtest/gtest.h>

using namespace testing;

class FakeContext {};

class PVR2DBuffer : public WSEGL::Buffer {
public:
    PVR2DBuffer() : WSEGL::Buffer(new FakeContext) {
        init(20, 30, 32, WSEGL_PIXELFORMAT_ARGB8888, nullptr);
    }

    WSEGLPixelFormat convertWaylandFormatToWSEGLFormat(wl_kms_format format) {
        return convertWaylandFormatToPVR2DFormat(format);
    }

    int getSizeOfPixel(WSEGLPixelFormat format) {
        return Buffer::getSizeOfPixel(format);
    }

private:
};

class WSEGLBuffer : public ::testing::Test {
public:
protected:
    testing::NiceMock<MockPVR2D> pvr2d;
    PVR2DBuffer buffer;
};

TEST_F(WSEGLBuffer, is_it_compile) {
    EXPECT_EQ(20, buffer.getWidth());
}

TEST_F(WSEGLBuffer, wrap_memory) {
    EXPECT_NE(nullptr, buffer.getMemInfo());
}

TEST_F(WSEGLBuffer, expect_unamp_on_destroy) {
    EXPECT_CALL(pvr2d, MemFree(_, _));
}

TEST_F(WSEGLBuffer, convert_wayland_format_to_wsegl) {
    EXPECT_EQ(WSEGL_PIXELFORMAT_RGB565,
              buffer.convertWaylandFormatToWSEGLFormat(WL_KMS_FORMAT_RGB565));
}

TEST_F(WSEGLBuffer, throw_exception_if_no_supported_format) {
    EXPECT_ANY_THROW(buffer.convertWaylandFormatToWSEGLFormat(WL_KMS_FORMAT_ABGR1555));
}

TEST_F(WSEGLBuffer, bytes_per_pixel_for_format) {
    EXPECT_EQ(4, buffer.getSizeOfPixel(WSEGL_PIXELFORMAT_ARGB8888));
}

TEST_F(WSEGLBuffer, memory_info_contains_correct_size) {
    EXPECT_EQ(32 * 30 * 4, buffer.getMemInfo()->ui32MemSize);
}
