#include <cstdio>
#include <unistd.h>
#include <algorithm>
#include <libkms/libkms.h>

#include "lib/resources.hpp"
#include "lib/kmsbuffer.hpp"

struct PageFlipCtx {
    Framebuffer* front;
    Framebuffer* back;
    Crtc* crtc;
    uint8_t color;
};

void pageFlipHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
{
    auto ctx = static_cast<PageFlipCtx*>(data);
    ctx->back->clear(ctx->color);
    ctx->color++;
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

    Resources r(fd);
    auto c = r.getDefaultConnector();
    auto e = c.getEncoder();
    auto crtc = e.getCrtc();

    struct kms_driver* driver;
    kms_create(fd, &driver);



    KmsBuffer d(driver, 800, 480);
    KmsBuffer d2(driver, 800, 480);
    Framebuffer fb(fd, &d);
    Framebuffer fb2(fd, &d);
    crtc.setup(fb, c.getId(), c.getDefaulModeInfo());

    PageFlipCtx flipCtx;
    flipCtx.back = &fb;
    flipCtx.front = &fb2;
    flipCtx.crtc = &crtc;
    flipCtx.color = 0;

    drmEventContext ctx;
    drmModePageFlip(fd, crtc.getId(), fb2.getId(), DRM_MODE_PAGE_FLIP_EVENT, &flipCtx);
    ctx.version = DRM_EVENT_CONTEXT_VERSION;
    ctx.vblank_handler = nullptr;
    ctx.page_flip_handler = pageFlipHandler;
    for (auto i = 0; i < 10256; i++) {
        struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
        fd_set fds;
        int ret;

        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fd, &fds);
        ret = select(fd + 1, &fds, NULL, NULL, &timeout);
        drmHandleEvent(fd, &ctx);
    }
    kms_destroy(&driver);
    drmClose(fd);
}
