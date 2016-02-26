#include <cstdio>

#define virtual myvirtual
#include "xf86drm.h"
#include "xf86drmMode.h"
#include <drm.h>
#undef virtual

#include <vector>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <algorithm>

class ModeInfo
{
public:
    ModeInfo(drmModeModeInfoPtr modeInfo) {
        mModeInfo = modeInfo;
        if (not modeInfo) {
            throw "Mode Info cannot be created";
        }
    }

    ~ModeInfo() {

    }

    void dump() {
        fprintf(stderr, "ModeInfo %s\n"
                "\tClock %d\n"
                "\tHDisplay %d, HSyncStart, %d, HSyncEnd %d, HTotal %d, HSkew %d\n"
                "\tVDisplay %d, VSyncStart, %d, VSyncEnd %d, VTotal %d, VScan %d\n"
                "\tVRefresh %d\n"
                "\tFlags 0x%x\n"
                "\tType %d\n",
                mModeInfo->name,
                mModeInfo->clock,
                mModeInfo->hdisplay, mModeInfo->hsync_start, mModeInfo->hsync_end,
                mModeInfo->htotal, mModeInfo->hskew,
                mModeInfo->vdisplay, mModeInfo->vsync_start, mModeInfo->vsync_end,
                mModeInfo->vtotal, mModeInfo->vscan,
                mModeInfo->vrefresh,
                mModeInfo->flags,
                mModeInfo->type);
    }

private:
    drmModeModeInfoPtr mModeInfo;
};

class DumbBuffer
{
public:
    DumbBuffer(int fd, uint32_t width, uint32_t height, uint32_t bpp = 32):
               mFd(fd), mWidth(width), mHeight(height), mBpp(bpp), mDepth(24) {
        mFd = fd;
        drm_mode_create_dumb creq;
        memset(&creq, 0, sizeof(creq));
        creq.width = mWidth;
        creq.height = mHeight;
        creq.bpp = bpp;
        auto ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
        if (ret < 0) {
            throw "Can't create dumb buffer";
        }
        mHandle = creq.handle;
        mSize = creq.size;
        mStride = creq.pitch;
        map();
    }

    ~DumbBuffer() {
        drm_mode_destroy_dumb dreq;
        memset(&dreq, 0, sizeof(dreq));
        dreq.handle = mHandle;
        drmIoctl(mFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
    }

//    DumbBuffer(DumbBuffer&& o) {
//        //mObject  = std::move(o.mObject);
//    }
//    DumbBuffer& operator=(DumbBuffer&& o) {
//        //mObject  = std::move(o.mObject);
//    }

    DumbBuffer(DumbBuffer&) = delete;
    DumbBuffer& operator=(const DumbBuffer&)  = delete;

    uint32_t getWidth() const {
        return mWidth;
    }

    uint32_t getHeight() const {
        return mHeight;
    }

    uint32_t getHandle() const {
        return mHandle;
    }

    uint32_t getStride() const {
        return mStride;
    }

    uint8_t getDepth() const {
        return mDepth;
    }

    uint8_t getBpp() const {
        return mBpp;
    }

    void clear(uint8_t color) {
        memset(mMappedBuffer, color, mSize);
    }

private:
    int mFd;
    uint8_t* mMappedBuffer;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mHandle;
    uint64_t mSize;
    uint32_t mStride;
    uint8_t mDepth;
    uint8_t mBpp;

    void map() {
        drm_mode_map_dumb mreq;
        memset(&mreq, 0, sizeof(mreq));
        mreq.handle = mHandle;
        auto ret = drmIoctl(mFd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
        if (ret < 0) {
            throw "Cannot map buffer(ioctl)";
        }
        mMappedBuffer = static_cast<uint8_t*>(mmap(0, mSize, PROT_READ | PROT_WRITE, MAP_SHARED, mFd,
                                                   mreq.offset));
    }
};

class Framebuffer
{
public:
    Framebuffer(int fd, DumbBuffer* buffer) {
        mFd = fd;
        mBuffer = buffer;
        auto ret = drmModeAddFB(mFd, buffer->getWidth(), buffer->getHeight(), buffer->getDepth(),
                                buffer->getBpp(), buffer->getStride(), buffer->getHandle(), &mFb);
        if (ret) {
            throw "Cannot create framebuffer";
        }

    }
    ~Framebuffer() {
        drmModeRmFB(mFd, mFb);
    }

//    Framebuffer(Framebuffer&& o) {
//        mObject  = std::move(o.mObject);
//    }
//    Framebuffer& operator=(Framebuffer&& o) {
//        mObject  = std::move(o.mObject);
//    }

    Framebuffer(Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&)  = delete;

    uint32_t getId() {
        return mFb;
    }

    void clear(uint8_t color) {
        mBuffer->clear(color);
    }

private:
    int mFd;
    DumbBuffer* mBuffer;
    uint32_t mFb;
};

class Crtc
{
public:
    Crtc(int fd, uint32_t crtcId) {
        mFd = fd;
        mObject = drmModeGetCrtc(fd, crtcId);
    }

    Crtc(Crtc&& o) {
        mObject  = std::move(o.mObject);
    }
    Crtc& operator=(Crtc&& o) {
        mObject  = std::move(o.mObject);
    }

    Crtc(Crtc&) = delete;
    Crtc& operator=(const Crtc&)  = delete;

    ~Crtc() {
        drmModeFreeCrtc(mObject);
    }

    void setup(Framebuffer& fb, uint32_t connector, drmModeModeInfoPtr mode) {
        drmModeSetCrtc(mFd, mObject->crtc_id, fb.getId(), 0, 0, &connector, 1, mode);
    }

    uint32_t getId() {
        return mObject->crtc_id;
    }

    void dump() {
        fprintf(stderr, "Crtc: %d\n"
                "\tBuffer_Id %d\n"
                "\tPos(%d, %d) Size(%d, %d)\n"
                "\tValid %d\n"
                "\tGammaSize %d\n",
                mObject->crtc_id,
                mObject->buffer_id,
                mObject->x, mObject->y, mObject->width, mObject->height,
                mObject->mode_valid,
                mObject->gamma_size);
    }

private:
    int mFd;
    drmModeCrtcPtr mObject;
};

class Encoder
{
public:
    Encoder(int fd, uint32_t encoderId) {
        fprintf(stderr, "%s: %d %p\n", __FUNCTION__, encoderId, this);
        mFd = fd;
        mObject = drmModeGetEncoder(fd, encoderId);
        if (not mObject) {
            throw "Can't find encoder";
        }
    }
    ~Encoder() {
        drmModeFreeEncoder(mObject);
    }
    Encoder(Encoder&& e) {
        mFd = e.mFd;
        mObject = e.mObject;
        e.mObject = nullptr;
    }
    Encoder& operator=(Encoder&& e) {
        mFd = e.mFd;
        mObject = e.mObject;
        e.mObject = nullptr;
    }
    Encoder(const Encoder& ) = delete;
    Encoder& operator=(const Encoder&) = delete;

    Crtc getCrtc() {
        return Crtc(mFd, mObject->crtc_id);
    }

    void dump() {
        fprintf(stderr, "Encoder %d\n"
                "\tType 0x%x, Crtc %d\n"
                "\tPossible_crtcs 0x%x\n"
                "\tPossible clones 0x%x\n",
                mObject->encoder_id,
                mObject->encoder_type, mObject->crtc_id,
                mObject->possible_crtcs, mObject->possible_clones
                );
    }

private:
    drmModeEncoderPtr mObject;
    int mFd;
};

class Connector
{
public:
    Connector(int fd, uint32_t connectorId) {
        mFd = fd;
        fprintf(stderr, "%s: %p\n", __FUNCTION__, this);
        mObject = drmModeGetConnector(fd, connectorId);
        if (not mObject) {
            throw "Connector not connected";
        }
    }
    Connector(Connector&& c) {
        mFd = c.mFd;
        mObject = std::move(c.mObject);
        c.mObject = nullptr;
    }
    Connector& operator=(Connector&& c) {
        mFd = c.mFd;
        mObject = std::move(c.mObject);
        c.mObject = nullptr;
    }
    Connector(const Connector& ) = delete;
    Connector& operator=(const Connector&) = delete;

    ~Connector() {
        fprintf(stderr, "%s: %p\n", __FUNCTION__, this);
        if (mObject) {
            drmModeFreeConnector(mObject);
        }
    }

    Encoder getEncoder() {
        return Encoder(mFd, mObject->encoder_id);
    }

    drmModeModeInfoPtr getDefaulModeInfo() {
        return mObject->modes;
    }

    bool isConnected() {
        return mObject->connection == DRM_MODE_CONNECTED;
    }

    uint32_t getId() {
        return mObject->connector_id;
    }

    void dump() {
        fprintf(stderr, "Connector Id: %d\n"
                "\tConnected encoder: %d\n"
                "\tType: %d, TypeId: %d\n"
                "\tConnection: %d\n"
                "\tWidth %d, \tHeight %d\n",
                mObject->connector_id,
                mObject->encoder_id,
                mObject->connector_type, mObject->connector_type_id,
                mObject->connection,
                mObject->mmWidth, mObject->mmHeight);
        for (auto i = 0; i < mObject->count_modes; i++) {
            ModeInfo(&mObject->modes[i]).dump();
        }
    }

private:
    drmModeConnectorPtr mObject;
    int mFd;
};

class Resources
{
public:
    Resources(int fd) {
        mFd = fd;
        mResources = drmModeGetResources(fd);
        if (not mResources) {
            throw "Cannot get resource";
        }
    }

    ~Resources() {
        drmModeFreeResources(mResources);
    }

    Connector getDefaultConnector() {
        auto vec = getConnectors();
        auto it = std::find_if(std::begin(vec), std::end(vec),
                               [](Connector& c) {return c.isConnected(); });
        if (it != std::end(vec)) {
            return std::move(*it);
        }
    }

    std::vector<Connector> getConnectors() {
        std::vector<Connector> vec;
        for (auto i = 0; i < mResources->count_connectors; i++) {
            vec.emplace_back(mFd, mResources->connectors[i]);
        }
        return vec;
    }

    std::vector<Crtc> getCrtcs() {
        std::vector<Crtc> vec;
        for (auto i = 0; i < mResources->count_crtcs; i++) {
            vec.emplace_back(mFd, mResources->crtcs[i]);
        }
        return vec;
    }

    std::vector<Encoder> getEncoders() {
        std::vector<Encoder> vec;
        for (auto i = 0; i < mResources->count_encoders; i++) {
            vec.emplace_back(mFd, mResources->encoders[i]);
        }
        return vec;
    }

private:
    drmModeResPtr mResources;
    int mFd;
};

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

    drmEventContext ctx;
    drmModePageFlip(fd, crtc.getId(), fb2.getId(), DRM_MODE_PAGE_FLIP_EVENT, &flipCtx);
    ctx.version = DRM_EVENT_CONTEXT_VERSION;
    ctx.vblank_handler = nullptr;
    ctx.page_flip_handler = pageFlipHandler;
    for (auto i = 0; i < 256; i++) {
        struct timeval timeout = { .tv_sec = 3, .tv_usec = 0 };
        fd_set fds;
        int ret;

        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(fd, &fds);
        ret = select(fd + 1, &fds, NULL, NULL, &timeout);
        drmHandleEvent(fd, &ctx);
    }

    drmClose(fd);
}
