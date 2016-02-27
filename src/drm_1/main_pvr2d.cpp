#include <cstdio>
#include <unistd.h>
#include <algorithm>
#include <libkms/libkms.h>

#include "lib/resources.hpp"
#include "lib/dumbbuffer.hpp"

#include "pvr2d/pvr2d.h"
#include <cstring>
#include "dk_utils/logger.hpp"

class BlitEngine : NonCopyable
{
public:
    BlitEngine() {
        auto devices = PVR2DEnumerateDevices(0);
        LOGVD("Devices %d ", devices);
        pDevInfo = (PVR2DDEVICEINFO *) malloc(devices * sizeof(PVR2DDEVICEINFO));
        PVR2DEnumerateDevices(pDevInfo);
        ePVR2DStatus = PVR2DCreateDeviceContext (pDevInfo[0].ulDevID, &mContext, 0);
        auto width = 300;
        auto height = 300;
        std::vector<uint32_t> mData(width * height, 0xFFFF00FF);
        pBlt = (PVR2DBLTINFO *) calloc(1, sizeof(PVR2DBLTINFO));
        pBlt->CopyCode = PVR2DROPcopyInverted;
        pBlt->BlitFlags = PVR2D_BLIT_DISABLE_ALL;
        ePVR2DStatus = PVR2DMemWrap(mContext, mData.data(), 0, mData.size() * sizeof(mData[0]),
                                    NULL, &mSrc);

        pBlt->pSrcMemInfo = mSrc;
        pBlt->SrcSurfWidth = width;
        pBlt->SrcSurfHeight = height;
        pBlt->SrcFormat = PVR2D_ARGB8888;
        pBlt->SrcStride = width * sizeof(mData[0]);
        pBlt->SizeX = width;
        pBlt->SizeY = height;
        pBlt->CopyCode = PVR2DROPcopy;
    }

    ~BlitEngine() {
        PVR2DMemFree(mContext, mSrc);
        PVR2DDestroyDeviceContext(mContext);
    }

    void blitTo(Buffer* b) {
        static auto x = 0;
        static auto y = 0;
        PVR2DMEMINFO* mDst = 0;
        ePVR2DStatus = PVR2DMemWrap(mContext, b->map(), 0,
                            b->getWidth() * b->getHeight() * b->getBpp() / 8,
                            NULL, &mDst);
        if (mDst == NULL) {
            printf("Fatal Error in pvr2dmemwrap, exiting\n");
            exit(1);
        }
        pBlt->pDstMemInfo = mDst;
        pBlt->DstSurfWidth = b->getWidth();
        pBlt->DstSurfHeight = b->getHeight();
        pBlt->DstStride = b->getStride();
        pBlt->DSizeX = pBlt->SizeX;
        pBlt->DSizeY = pBlt->SizeY;
        pBlt->DstX = x++;
        pBlt->DstY = y++;
        pBlt->DstFormat = PVR2D_ARGB8888;

        ePVR2DStatus = PVR2DBlt(mContext, pBlt);
        //LOGVD("PVR2DBlt status %d", ePVR2DStatus);
        ePVR2DStatus = PVR2DQueryBlitsComplete(mContext, mDst, 1);
        //LOGVD("PVR2DQueryBlitsComplete status %d", ePVR2DStatus);
        //unwrap again
        PVR2DMemFree(mContext, mDst);
    }

private:
    PVR2DERROR ePVR2DStatus;
    PVR2DDEVICEINFO* pDevInfo=0;
    PVR2DCONTEXTHANDLE mContext=0;
    PPVR2DBLTINFO pBlt=0;
    PVR2DMEMINFO* mSrc = 0;
};

struct PageFlipCtx {
    Framebuffer* front;
    Framebuffer* back;
    Crtc* crtc;
    uint8_t color;
    BlitEngine* blit;
};

void pageFlipHandler(int fd, unsigned int frame, unsigned int sec, unsigned int usec, void *data)
{
    auto ctx = static_cast<PageFlipCtx*>(data);

    //ctx->back->clear(ctx->color);
    //ctx->color++;
    std::swap(ctx->front, ctx->back);
    drmModePageFlip(fd, ctx->crtc->getId(), ctx->front->getId(), DRM_MODE_PAGE_FLIP_EVENT, ctx);
    ctx->blit->blitTo(ctx->back->getBuffer());
}

int main()
{
    BlitEngine bt;
    //return 0;
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

    DumbBuffer d(fd, 800, 480);
    DumbBuffer d2(fd, 800, 480);
    Framebuffer fb(fd, &d);
    Framebuffer fb2(fd, &d);
    crtc.setup(fb, c.getId(), c.getDefaulModeInfo());

    PageFlipCtx flipCtx;
    flipCtx.back = &fb;
    flipCtx.front = &fb2;
    flipCtx.crtc = &crtc;
    flipCtx.color = 0;
    flipCtx.blit = &bt;

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
