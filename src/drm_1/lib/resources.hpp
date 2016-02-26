#ifndef RESOURCES_HPP
#define RESOURCES_HPP

#define virtual myvirtual
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#undef virtual

#include "connector.hpp"
#include "encoder.hpp"
#include "crtc.hpp"

#include <vector>

class Resources : NonCopyable
{
public:
    Resources(int fd);
    ~Resources();

    Connector getDefaultConnector();

    std::vector<Connector> getConnectors();
    std::vector<Crtc> getCrtcs();
    std::vector<Encoder> getEncoders();

private:
    drmModeResPtr mResources;
    int mFd;
};

#endif // RESOURCES_HPP
