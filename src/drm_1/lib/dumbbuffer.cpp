#include "dumbbuffer.hpp"
#include <cstring>
#include <sys/mman.h>

DumbBuffer::DumbBuffer(int fd, uint32_t width, uint32_t height):
    mFd(fd), Buffer(width, height)
{
    drm_mode_create_dumb creq;
    memset(&creq, 0, sizeof(creq));
    creq.width = getWidth();
    creq.height = getHeight();
    creq.bpp = getBpp();
    auto ret = drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &creq);
    if (ret < 0) {
        throw "Can't create dumb buffer";
    }
    mHandle = creq.handle;
    mSize = creq.size;
    mStride = creq.pitch;
    map2();
}

DumbBuffer::~DumbBuffer()
{
    drm_mode_destroy_dumb dreq;
    memset(&dreq, 0, sizeof(dreq));
    dreq.handle = mHandle;
    drmIoctl(mFd, DRM_IOCTL_MODE_DESTROY_DUMB, &dreq);
}

void DumbBuffer::clear(uint8_t color)
{
    memset(mMappedBuffer, color, mSize);
}

uint8_t* DumbBuffer::map()
{
    return mMappedBuffer;
}

void DumbBuffer::map2()
{
    drm_mode_map_dumb mreq;
    memset(&mreq, 0, sizeof(mreq));
    mreq.handle = mHandle;
    auto ret = drmIoctl(mFd, DRM_IOCTL_MODE_MAP_DUMB, &mreq);
    if (ret < 0) {
        throw "Cannot map buffer(ioctl)";
    }
    mMappedBuffer = static_cast<uint8_t*>(mmap(0, mSize, PROT_READ | PROT_WRITE, MAP_SHARED, mFd,
                                               mreq.offset));
    clear(0xFF);
}
