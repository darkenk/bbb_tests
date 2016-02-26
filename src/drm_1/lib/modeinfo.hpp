#ifndef MODEINFO_HPP
#define MODEINFO_HPP

#define virtual myvirtual
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm.h>
#undef virtual

#include "dk_utils/noncopyable.hpp"

class ModeInfo : NonCopyable
{
public:
    ModeInfo(drmModeModeInfoPtr modeInfo);
    ~ModeInfo();

    void dump();

private:
    drmModeModeInfoPtr mModeInfo;
};

#endif // MODEINFO_HPP
