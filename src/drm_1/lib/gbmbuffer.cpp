#include "gbmbuffer.hpp"

GbmBuffer::GbmBuffer(gbm_bo* buffer, gbm_surface* surface) :
    Buffer(gbm_bo_get_width(buffer), gbm_bo_get_height(buffer)),
    mBuffer(buffer), mSurface(surface)
{
    mStride = gbm_bo_get_stride(buffer);
    mHandle = gbm_bo_get_handle(buffer).u32;
}


void GbmBuffer::clear(uint8_t color)
{
}

uint8_t* GbmBuffer::map()
{
}

void GbmBuffer::unmap()
{
    gbm_surface_release_buffer(mSurface, mBuffer);
}
