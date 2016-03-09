#ifndef GBMBUFFER_HPP
#define GBMBUFFER_HPP

#include "buffer.hpp"
#include <stddef.h>
#include <gbm/gbm.h>

class GbmBuffer : public Buffer
{
public:
    GbmBuffer(gbm_bo* buffer, gbm_surface* surface);

    virtual void clear(uint8_t color) override;
    virtual uint8_t* map() override;
    virtual void unmap() override;

private:
    gbm_bo* mBuffer;
    gbm_surface* mSurface;
};

#endif // GBMBUFFER_HPP
