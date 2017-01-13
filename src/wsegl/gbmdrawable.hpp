#ifndef GBMDRAWABLE_HPP
#define GBMDRAWABLE_HPP

#include "drawable.hpp"
#include <wayland-egl.h>
#include <wayland-kms.h>
#include <wayland-kms-client-protocol.h>
#include <sys/mman.h>
#include <cassert>
#include "xf86drm.h"
#include <unistd.h>
#include <dk_utils/logger.hpp>
#include <gbm/gbm_kmsint.h>

struct wl_kms {
    struct wl_display *display;
    int fd; /* FD for DRM */
    char *device_name;

    struct kms_auth *auth; /* for nested authentication */
};

namespace WSEGL {

class GbmPixmapBuffer : public WSEGL::Buffer
{
public:
    GbmPixmapBuffer(PVR2DCONTEXTHANDLE ctx, wl_kms_buffer* kmsBuffer):
        Buffer(ctx) {
        auto format = convertWaylandFormatToPVR2DFormat(kmsBuffer->format);
        mSize = kmsBuffer->stride * kmsBuffer->height * getSizeOfPixel(format);

        struct drm_mode_map_dumb mmap_arg = {0};
        mmap_arg.handle = kmsBuffer->handle;
        int ret = drmIoctl(kmsBuffer->kms->fd, DRM_IOCTL_MODE_MAP_DUMB, &mmap_arg);
        mAddr = mmap(nullptr, mSize, (PROT_READ | PROT_WRITE), MAP_SHARED, kmsBuffer->kms->fd,
                    mmap_arg.offset);

        init(kmsBuffer->width, kmsBuffer->height, kmsBuffer->stride, format, mAddr);
    }

    virtual ~GbmPixmapBuffer() override {
        munmap(mAddr, mSize);
    }

private:
    void* mAddr;
    size_t mSize;
};

class GbmPixmapDrawable : public WSEGL::Drawable
{
public:
    GbmPixmapDrawable(PVR2DCONTEXTHANDLE ctx, NativePixmapType pixmap):
        Drawable(ctx) {
        auto* resource = reinterpret_cast<wl_resource*>(pixmap);
        auto* kmsBuffer = wayland_kms_buffer_get(resource);
        mBuffers[0] = mBuffers[1] = new GbmPixmapBuffer(ctx, kmsBuffer);
    }

    virtual ~GbmPixmapDrawable() override {
        delete mBuffers[0];
    }

};

class GBMBuffer : public WSEGL::Buffer
{
public:
    GBMBuffer(PVR2DCONTEXTHANDLE ctx, gbm_kms_bo* buffer): Buffer(ctx) {
        void* out;
        kms_bo_map(buffer->bo, &out);
        auto format = WSEGL_PIXELFORMAT_ARGB8888;
        auto stride = buffer->base.stride / getSizeOfPixel(format);
        init(buffer->base.width, buffer->base.height, stride, format, out);
        kms_bo_unmap(buffer->bo);
    }

};

class GBMWindow : public WSEGL::Drawable
{
public:
    GBMWindow(PVR2DCONTEXTHANDLE ctx, NativeWindowType window):
        Drawable(ctx), mSurface(reinterpret_cast<struct gbm_kms_surface*>(window)) {
        for (unsigned int i = 0; i < BUFFER_COUNT; i++) {
            mBuffers[i] = new GBMBuffer(ctx, mSurface->bo[i]);
        }
        mSurface->front = 0;
    }
    virtual ~GBMWindow() override {
        for (auto b : mBuffers) {
            delete b;
        }
    }

    static GBMWindow* getFromWSEGL(WSEGLDrawableHandle handle) {
        return reinterpret_cast<GBMWindow*>(handle);
    }

    void swapBuffers() override {
        WSEGL::Drawable::swapBuffers();
        mSurface->front = getIndex();
    }

private:
    struct gbm_kms_surface* mSurface;
};

class GBMDisplay : public Display
{
public:
    Drawable* createWindow(NativeWindowType window) override {
        return new GBMWindow(getContext(), window);
    }

};

}

#endif // GBMDRAWABLE_HPP
