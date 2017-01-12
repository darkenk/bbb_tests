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

struct wl_kms {
    struct wl_display *display;
    int fd; /* FD for DRM */
    char *device_name;

    struct kms_auth *auth; /* for nested authentication */
};

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

    virtual ~GbmPixmapBuffer() {
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

    virtual ~GbmPixmapDrawable() {
        delete mBuffers[0];
    }

};

#endif // GBMDRAWABLE_HPP
