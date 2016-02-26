#ifndef CRTC_HPP
#define CRTC_HPP

#define virtual myvirtual
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#undef virtual

#include <memory>
#include "dk_utils/noncopyable.hpp"
#include "framebuffer.hpp"

class Crtc : NonCopyable
{
public:
    Crtc(int fd, uint32_t crtcId);
    Crtc(Crtc&& o);
    Crtc& operator=(Crtc&& o);
    ~Crtc();

    void setup(Framebuffer& fb, uint32_t connector, drmModeModeInfoPtr mode);
    uint32_t getId();
    void dump();

private:
    int mFd;
    drmModeCrtcPtr mObject;
};

#endif // CRTC_HPP
