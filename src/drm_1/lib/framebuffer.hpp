#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#define virtual myvirtual
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#undef virtual

#include "buffer.hpp"
#include "dk_utils/noncopyable.hpp"

class Framebuffer : NonCopyable
{
public:
    Framebuffer(int fd, Buffer* buffer);
    ~Framebuffer();

    uint32_t getId();

    void clear(uint8_t color);
    Buffer* getBuffer();

private:
    int mFd;
    Buffer* mBuffer;
    uint32_t mFb;
};

#endif // FRAMEBUFFER_HPP
