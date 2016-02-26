#ifndef CONNECTOR_HPP
#define CONNECTOR_HPP

#define virtual myvirtual
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#undef virtual

#include <cstdint>
#include "encoder.hpp"
#include "dk_utils/noncopyable.hpp"

class Connector : NonCopyable
{
public:
    Connector(int fd, uint32_t connectorId);
    Connector(Connector&& c);
    Connector& operator=(Connector&& c);
    ~Connector();

    Encoder getEncoder();
    drmModeModeInfoPtr getDefaulModeInfo();
    bool isConnected();
    uint32_t getId();

    void dump();

private:
    drmModeConnectorPtr mObject;
    int mFd;
};

#endif // CONNECTOR_HPP
