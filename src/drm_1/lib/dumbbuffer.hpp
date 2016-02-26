#ifndef DUMBBUFFER_HPP
#define DUMBBUFFER_HPP

#include "buffer.hpp"

#define virtual myvirtual
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#undef virtual

class DumbBuffer : public Buffer
{
public:
    DumbBuffer(int fd, uint32_t width, uint32_t height);

    virtual ~DumbBuffer();

    virtual void clear(uint8_t color);

private:
    int mFd;
    uint8_t* mMappedBuffer;

    void map();
};

#endif // DUMBBUFFER_HPP
