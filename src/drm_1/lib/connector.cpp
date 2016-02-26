#include "connector.hpp"

#include "modeinfo.hpp"
#include "dk_utils/logger.hpp"

Connector::Connector(int fd, uint32_t connectorId) {
    mFd = fd;
    mObject = drmModeGetConnector(fd, connectorId);
    if (not mObject) {
        throw "Connector not connected";
    }
}

Connector::Connector(Connector&& c) {
    mFd = c.mFd;
    mObject = std::move(c.mObject);
    c.mObject = nullptr;
}

Connector& Connector::operator=(Connector&& c) {
    mFd = c.mFd;
    mObject = std::move(c.mObject);
    c.mObject = nullptr;
    return *this;
}

Connector::~Connector() {
    if (mObject) {
        drmModeFreeConnector(mObject);
    }
}

Encoder Connector::getEncoder() {
    return Encoder(mFd, mObject->encoder_id);
}

drmModeModeInfoPtr Connector::getDefaulModeInfo() {
    return mObject->modes;
}

bool Connector::isConnected() {
    return mObject->connection == DRM_MODE_CONNECTED;
}

uint32_t Connector::getId() {
    return mObject->connector_id;
}

void Connector::dump() {
    LOGVD("Connector Id: %d\n"
          "\tConnected encoder: %d\n"
          "\tType: %d, TypeId: %d\n"
          "\tConnection: %d\n"
          "\tWidth %d, \tHeight %d\n",
          mObject->connector_id,
          mObject->encoder_id,
          mObject->connector_type, mObject->connector_type_id,
          mObject->connection,
          mObject->mmWidth, mObject->mmHeight);
    for (auto i = 0; i < mObject->count_modes; i++) {
        ModeInfo(&mObject->modes[i]).dump();
    }
}
