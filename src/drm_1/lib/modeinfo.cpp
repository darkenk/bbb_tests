#include "modeinfo.hpp"

#include "dk_utils/logger.hpp"

ModeInfo::ModeInfo(drmModeModeInfoPtr modeInfo) {
    mModeInfo = modeInfo;
    if (not modeInfo) {
        throw "Mode Info cannot be created";
    }
}

ModeInfo::~ModeInfo() {

}

void ModeInfo::dump() {
    LOGVD("ModeInfo %s\n"
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
