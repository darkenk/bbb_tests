#include <cstdio>
#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <libkms/libkms.h>

#include "../drm_1/lib/resources.hpp"
#include "../drm_1/lib/dumbbuffer.hpp"
#include "../drm_1/lib/gbmbuffer.hpp"
#include "../drm_1/lib/kmsbuffer.hpp"

#include <cstring>
#include "dk_utils/logger.hpp"
#include <gbm/gbm.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <dk_opengl/eglwrapper.hpp>

struct PageFlipCtx {
    Framebuffer* front;
    Framebuffer* back;
    Crtc* crtc;
    gbm_surface* surface;
};

void vblankHandler(int fd, unsigned int sequence,  unsigned int sec, unsigned int usec, void *data)
{
    LOGVE("%d,%d", sec, usec);
    auto ctx = static_cast<PageFlipCtx*>(data);
    //ctx->back->getBuffer()->unmap();
}

void pageFlipHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
{
//    LOGVE("%d,%d", sec, usec);
    auto ctx = static_cast<PageFlipCtx*>(data);

    // does it really needed?
    ctx->front->getBuffer()->unmap();
    if (not ctx->back) {
        auto buf = gbm_surface_lock_front_buffer(ctx->surface);
        ctx->back = new Framebuffer(fd, new GbmBuffer(buf, ctx->surface));
    }
    std::swap(ctx->front, ctx->back);
    drmModePageFlip(fd, ctx->crtc->getId(), ctx->front->getId(), DRM_MODE_PAGE_FLIP_EVENT, ctx);
}

int main()
{
    int fd = drmOpen("tilcdc", nullptr);
    if (fd < 0) {
        fprintf(stderr, "Cannot open tilcdc\n");
        return 0;
    }
    uint64_t has_dumb;
    if (drmGetCap(fd, DRM_CAP_DUMB_BUFFER, &has_dumb) < 0 || !has_dumb) {
        fprintf(stderr, "drm device '%s' does not support dumb buffers\n", "tilcdc");
    }

    gbm_device* gbmDevice = gbm_create_device(fd);
    auto surface = gbm_surface_create(gbmDevice, 800, 480, GBM_BO_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT |
                                      GBM_BO_USE_RENDERING);
    assert(surface);
    //EGLWrapper w((EGLNativeDisplayType)gbmDevice, (EGLNativeWindowType)surface);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    //w.swap();

    Resources r(fd);
    auto c = r.getDefaultConnector();
    auto e = c.getEncoder();
    auto crtc = e.getCrtc();

    auto buf1 = gbm_surface_lock_front_buffer(surface);
    GbmBuffer b(buf1, surface);
    Framebuffer fb(fd, &b);
    crtc.setup(fb, c.getId(), c.getDefaulModeInfo());

    PageFlipCtx flipCtx;
    flipCtx.surface = surface;
    flipCtx.back = nullptr;
    flipCtx.front = &fb;
    flipCtx.crtc = &crtc;

    drmEventContext ctx;
    drmModePageFlip(fd, crtc.getId(), fb.getId(), DRM_MODE_PAGE_FLIP_EVENT, &flipCtx);
    ctx.version = DRM_EVENT_CONTEXT_VERSION;
    ctx.vblank_handler = vblankHandler;
    ctx.page_flip_handler = pageFlipHandler;

    for (auto i = 0; i < 10256; i++) {
        glClear(GL_COLOR_BUFFER_BIT);
      //  w.swap();

        struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
        fd_set fds;

        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fd, &fds);
        select(fd + 1, &fds, NULL, NULL, &timeout);
        drmHandleEvent(fd, &ctx);
    }

    gbm_device_destroy(gbmDevice);
    drmClose(fd);
}
