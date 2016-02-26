#include "kmsbuffer.hpp"

#include <cstring>

KmsBuffer::KmsBuffer(kms_driver* driver, uint32_t width, uint32_t height):
    Buffer(width, height) {
    unsigned attribs[] = {
        KMS_WIDTH,   mWidth,
        KMS_HEIGHT,  mHeight,
        KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
        KMS_TERMINATE_PROP_LIST
    };
    kms_bo_create(driver, attribs, &mBuffer);
    kms_bo_get_prop(mBuffer, KMS_PITCH, &mStride);
    kms_bo_get_prop(mBuffer, KMS_HANDLE, &mHandle);
    mSize = mStride * mHeight;
}

KmsBuffer::~KmsBuffer() {
    kms_bo_destroy(&mBuffer);
}

void KmsBuffer::clear(uint8_t color) {
    kms_bo_map(mBuffer, (void**)&mMappedBuffer);
    memset(mMappedBuffer, color, mSize);
    kms_bo_unmap(mBuffer);
}
