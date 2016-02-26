#ifndef ENCODER_HPP
#define ENCODER_HPP

#define virtual myvirtual
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#undef virtual

#include <cstdint>
#include "crtc.hpp"

class Encoder : NonCopyable
{
public:
    Encoder(int fd, uint32_t encoderId);
    ~Encoder();
    Encoder(Encoder&& e);
    Encoder& operator=(Encoder&& e);

    Crtc getCrtc();

    void dump();

private:
    drmModeEncoderPtr mObject;
    int mFd;
};

#endif // ENCODER_HPP
