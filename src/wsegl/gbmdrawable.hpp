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
    int fd;				/* FD for DRM */
    char *device_name;

    struct kms_auth *auth;		/* for nested authentication */
};

class GbmPixmapBuffer : public WSEGL::Buffer
{
public:
    GbmPixmapBuffer(PVR2DCONTEXTHANDLE ctx, wl_kms_buffer* kmsBuffer):
        mPvrCtx(ctx) {
        struct drm_mode_map_dumb mmap_arg = {0};
        mmap_arg.handle = kmsBuffer->handle;
        LOGVP("Buffer = %dx%d stride: %d", kmsBuffer->width, kmsBuffer->height, kmsBuffer->stride);
        mSize = kmsBuffer->stride * kmsBuffer->height;
        int ret = drmIoctl(kmsBuffer->kms->fd, DRM_IOCTL_MODE_MAP_DUMB, &mmap_arg);
        fprintf(stderr, "ret %d, offset %llu\n", ret, mmap_arg.offset);

//        int pagesize_mask = getpagesize() - 1;

//        mSize = ((mSize +pagesize_mask) & ~pagesize_mask);
        mAddr = mmap(nullptr, mSize, (PROT_READ | PROT_WRITE), MAP_SHARED, kmsBuffer->kms->fd,
                    mmap_arg.offset);
        LOGVD("SIZE %d", mSize);

        assert(PVR2DMemWrap(mPvrCtx, mAddr, PVR2D_WRAPFLAG_CONTIGUOUS, mSize, nullptr, &mMemInfo) == PVR2D_OK);
        LOGVD("Range 0x%x - 0x%x", mMemInfo->ui32DevAddr, mMemInfo->ui32DevAddr + mMemInfo->ui32MemSize );
    }

    virtual ~GbmPixmapBuffer() {
        PVR2DMemFree(mPvrCtx, mMemInfo);
        munmap(mAddr, mSize);
    }

private:
    PVR2DCONTEXTHANDLE mPvrCtx;
    void* mAddr;
    size_t mSize;
};

class GbmPixmapDrawable : public WSEGL::Drawable
{
public:
    GbmPixmapDrawable(PVR2DCONTEXTHANDLE ctx, NativePixmapType pixmap):
        Drawable(ctx) {
        fprintf(stderr, "Start %p this\n", this);
        auto* resource = reinterpret_cast<wl_resource*>(pixmap);
        auto* kmsBuffer = wayland_kms_buffer_get(resource);
        mBuffers[0] = mBuffers[1] = new GbmPixmapBuffer(ctx, kmsBuffer);
        init(kmsBuffer->width, kmsBuffer->height, kmsBuffer->stride / 4, WSEGL_PIXELFORMAT_ARGB8888);
    }

    virtual ~GbmPixmapDrawable() {
        fprintf(stderr, "Delete %p this\n", this);
        delete mBuffers[0];
    }

};

#endif // GBMDRAWABLE_HPP
