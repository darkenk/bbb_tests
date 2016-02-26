#include "encoder.hpp"

#include "dk_utils/logger.hpp"

Encoder::Encoder(int fd, uint32_t encoderId) {
    LOGVD("%s: %d %p\n", __FUNCTION__, encoderId, this);
    mFd = fd;
    mObject = drmModeGetEncoder(fd, encoderId);
    if (not mObject) {
        throw "Can't find encoder";
    }
}

Encoder::~Encoder() {
    drmModeFreeEncoder(mObject);
}

Encoder::Encoder(Encoder&& e) {
    mFd = e.mFd;
    mObject = e.mObject;
    e.mObject = nullptr;
}

Encoder& Encoder::operator=(Encoder&& e) {
    mFd = e.mFd;
    mObject = e.mObject;
    e.mObject = nullptr;
}

Crtc Encoder::getCrtc() {
    return Crtc(mFd, mObject->crtc_id);
}

void Encoder::dump() {
    LOGVD("Encoder %d\n"
          "\tType 0x%x, Crtc %d\n"
          "\tPossible_crtcs 0x%x\n"
          "\tPossible clones 0x%x\n",
          mObject->encoder_id,
          mObject->encoder_type, mObject->crtc_id,
          mObject->possible_crtcs, mObject->possible_clones
          );
}
