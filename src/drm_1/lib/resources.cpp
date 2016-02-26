#include "resources.hpp"

#include <algorithm>
#include "dk_utils/exceptions.hpp"

Resources::Resources(int fd) {
    mFd = fd;
    mResources = drmModeGetResources(fd);
    if (not mResources) {
        throw "Cannot get resource";
    }
}

Resources::~Resources() {
    drmModeFreeResources(mResources);
}

Connector Resources::getDefaultConnector() {
    auto vec = getConnectors();
    auto it = std::find_if(std::begin(vec), std::end(vec),
                           [](Connector& c) {return c.isConnected(); });
    if (it != std::end(vec)) {
        return std::move(*it);
    }
    throw Exception<Resources>("Cannot find default connector");
}

std::vector<Connector> Resources::getConnectors() {
    std::vector<Connector> vec;
    for (auto i = 0; i < mResources->count_connectors; i++) {
        vec.emplace_back(mFd, mResources->connectors[i]);
    }
    return vec;
}

std::vector<Crtc> Resources::getCrtcs() {
    std::vector<Crtc> vec;
    for (auto i = 0; i < mResources->count_crtcs; i++) {
        vec.emplace_back(mFd, mResources->crtcs[i]);
    }
    return vec;
}

std::vector<Encoder> Resources::getEncoders() {
    std::vector<Encoder> vec;
    for (auto i = 0; i < mResources->count_encoders; i++) {
        vec.emplace_back(mFd, mResources->encoders[i]);
    }
    return vec;
}
