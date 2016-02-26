#include "buffer.hpp"

Buffer::Buffer(uint32_t width, uint32_t height, uint8_t bpp):
    mWidth(width), mHeight(height), mHandle(0), mSize(0), mStride(0), mDepth(24),
    mBpp(bpp) {
}

Buffer::~Buffer() {
}

uint32_t Buffer::getWidth() const {
    return mWidth;
}

uint32_t Buffer::getHeight() const {
    return mHeight;
}

uint32_t Buffer::getHandle() const {
    return mHandle;
}

uint32_t Buffer::getStride() const {
    return mStride;
}

uint8_t Buffer::getDepth() const {
    return mDepth;
}

uint8_t Buffer::getBpp() const {
    return mBpp;
}
