#include <gtest/gtest.h>
#include "gbmdrawable.hpp"
#include "mockpvr2d.hpp"
#include "mockdrm.hpp"
#include "mocklibc.hpp"
#include "mockwayland.hpp"

using namespace testing;

class FakeContext {};

class GBMTest : public Test
{
public:
    virtual void SetUp() {
        ctx = new FakeContext;
        kmsBuffer = new wl_kms_buffer;
        memset(kmsBuffer, 0x0, sizeof(wl_kms_buffer));
        kmsBuffer->stride = 16;
        kmsBuffer->height = 12;
        kmsBuffer->format = WL_KMS_FORMAT_ARGB8888;
        kmsBuffer->kms = new wl_kms;
        memset(kmsBuffer->kms, 0x0, sizeof(wl_kms));
        buffer = new GbmPixmapBuffer(ctx, kmsBuffer);
        nativePixmap = reinterpret_cast<NativePixmapType>(1245687);
    }

    virtual void TearDown() {
        delete buffer;
        delete kmsBuffer->kms;
        delete kmsBuffer;
        delete ctx;
    }

protected:
    wl_kms_buffer* kmsBuffer;
    testing::NiceMock<MockPVR2D> pvr2d;
    testing::NiceMock<MockDRM> drm;
    testing::NiceMock<MockLibc> libc;
    testing::NiceMock<MockWayland> wayland;
    FakeContext* ctx;
    GbmPixmapBuffer* buffer;
    NativePixmapType nativePixmap;
};

TEST_F(GBMTest, wrap_memory) {
    EXPECT_NE(nullptr, buffer->getMemInfo());
}

TEST_F(GBMTest, unamp_on_destroy) {
    EXPECT_CALL(pvr2d, MemFree(_, _));
}

TEST_F(GBMTest, correct_format) {
    EXPECT_EQ(WSEGL_PIXELFORMAT_ARGB8888, buffer->getFormat());
}

TEST_F(GBMTest, mmap_correct_size) {
    EXPECT_CALL(libc, mmap(_, 16 * 12 * 4, _, _, _, _));
    GbmPixmapBuffer b(ctx, kmsBuffer);
}

TEST_F(GBMTest, unmap_correct_size_on_destroy) {
    EXPECT_CALL(libc, munmap(_, 16 * 12 * 4));
}

TEST_F(GBMTest, pixmap_drawable_ctor) {
    auto r = reinterpret_cast<wl_resource*>(nativePixmap);
    ON_CALL(wayland, kms_buffer_get(Eq(r))).WillByDefault(Return(kmsBuffer));
    GbmPixmapDrawable gbm(ctx, nativePixmap);
}
