#include "crtc.hpp"

#include "dk_utils/logger.hpp"

Crtc::Crtc(int fd, uint32_t crtcId) {
    mFd = fd;
    mObject = drmModeGetCrtc(fd, crtcId);
}

Crtc::Crtc(Crtc&& o) {
    mFd = o.mFd;
    mObject  = std::move(o.mObject);
}

Crtc& Crtc::operator=(Crtc&& o) {
    mFd = o.mFd;
    mObject  = std::move(o.mObject);
    return *this;
}

Crtc::~Crtc() {
    drmModeFreeCrtc(mObject);
}

void Crtc::setup(Framebuffer& fb, uint32_t connector, drmModeModeInfoPtr mode) {
    drmModeSetCrtc(mFd, mObject->crtc_id, fb.getId(), 0, 0, &connector, 1, mode);
}

uint32_t Crtc::getId() {
    return mObject->crtc_id;
}

void Crtc::dump() {
    LOGVD("Crtc: %d\n"
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
