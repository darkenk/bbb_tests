#include "framebuffer.hpp"

Framebuffer::Framebuffer(int fd, Buffer* buffer) {
    mFd = fd;
    mBuffer = buffer;
    auto ret = drmModeAddFB(mFd, buffer->getWidth(), buffer->getHeight(), buffer->getDepth(),
                            buffer->getBpp(), buffer->getStride(), buffer->getHandle(), &mFb);
    if (ret) {
        throw "Cannot create framebuffer";
    }

}

Framebuffer::~Framebuffer() {
    drmModeRmFB(mFd, mFb);
}

uint32_t Framebuffer::getId() {
    return mFb;
}

void Framebuffer::clear(uint8_t color) {
    mBuffer->clear(color);
}
