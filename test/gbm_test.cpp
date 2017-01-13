#include <gtest/gtest.h>
#include "gbmdrawable.hpp"
#include "mockpvr2d.hpp"
#include "mockdrm.hpp"
#include "mocklibc.hpp"
#include "mockwayland.hpp"
#include "mockkms.hpp"

using namespace testing;
using namespace WSEGL;

class FakeContext {};

class GBMTest : public Test
{
public:
    virtual void SetUp() {
        ctx = new FakeContext;
        wlKmsBuffer = new wl_kms_buffer;
        memset(wlKmsBuffer, 0x0, sizeof(wl_kms_buffer));
        wlKmsBuffer->stride = 16;
        wlKmsBuffer->height = 12;
        wlKmsBuffer->format = WL_KMS_FORMAT_ARGB8888;
        wlKmsBuffer->kms = new wl_kms;
        memset(wlKmsBuffer->kms, 0x0, sizeof(wl_kms));
        buffer = new GbmPixmapBuffer(ctx, wlKmsBuffer);
        nativePixmap = reinterpret_cast<NativePixmapType>(1245687);
        gbmKmsBuffer = new gbm_kms_bo;
        memset(gbmKmsBuffer, 0x0, sizeof(gbm_kms_bo));
        gbmKmsBuffer->base.height = 12;
        gbmKmsBuffer->base.stride = 16;
        gbmKmsBuffer->base.width = 14;
        gbmKmsSurface = new struct gbm_kms_surface;
        memset(gbmKmsSurface, 0x0, sizeof(*gbmKmsSurface));
        gbmKmsSurface->bo[0] = gbmKmsBuffer;
        gbmKmsSurface->bo[1] = gbmKmsBuffer;
        display = new GBMDisplay();
    }

    virtual void TearDown() {
        delete display;
        delete gbmKmsSurface;
        delete gbmKmsBuffer;
        delete buffer;
        delete wlKmsBuffer->kms;
        delete wlKmsBuffer;
        delete ctx;
    }

protected:
    wl_kms_buffer* wlKmsBuffer;
    NiceMock<MockPVR2D> pvr2d;
    NiceMock<MockDRM> drm;
    NiceMock<MockLibc> libc;
    NiceMock<MockWayland> wayland;
    NiceMock<MockKms> kms;
    FakeContext* ctx;
    GbmPixmapBuffer* buffer;
    NativePixmapType nativePixmap;
    gbm_kms_bo* gbmKmsBuffer;
    struct gbm_kms_surface* gbmKmsSurface;
    GBMDisplay* display;
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
    GbmPixmapBuffer b(ctx, wlKmsBuffer);
}

TEST_F(GBMTest, unmap_correct_size_on_destroy) {
    EXPECT_CALL(libc, munmap(_, 16 * 12 * 4));
}

TEST_F(GBMTest, pixmap_drawable_ctor) {
    auto r = reinterpret_cast<wl_resource*>(nativePixmap);
    ON_CALL(wayland, kms_buffer_get(Eq(r))).WillByDefault(Return(wlKmsBuffer));
    GbmPixmapDrawable gbm(ctx, nativePixmap);
}

TEST_F(GBMTest, gbm_buffer_returns_correct_width) {
    GBMBuffer g(ctx, gbmKmsBuffer);
    EXPECT_EQ(14, g.getWidth());
}

TEST_F(GBMTest, gbm_window_has_correct_buffer_size) {
    GBMWindow w(ctx, gbmKmsSurface);
    EXPECT_EQ(14, w.getFrontBuffer()->getWidth());
}

TEST_F(GBMTest, create_gbm_window_from_display) {
    auto w = display->createWindow(gbmKmsSurface);
    EXPECT_NE(nullptr, dynamic_cast<GBMWindow*>(w));
    delete w;
}
