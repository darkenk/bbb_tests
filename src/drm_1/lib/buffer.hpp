#ifndef BUFFER_HPP
#define BUFFER_HPP

#include <cstdint>
#include "dk_utils/noncopyable.hpp"

class Buffer : public NonCopyable
{
public:
    Buffer(uint32_t width, uint32_t height, uint8_t bpp = 32);
    virtual ~Buffer();

    uint32_t getWidth() const;
    uint32_t getHeight() const;
    uint32_t getHandle() const;
    uint32_t getStride() const;
    uint8_t getDepth() const;
    uint8_t getBpp() const;

    virtual void clear(uint8_t color) = 0;
    virtual uint8_t* map() = 0;

protected:
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mHandle;
    uint64_t mSize;
    uint32_t mStride;
    uint8_t mDepth;
    uint8_t mBpp;

};

#endif // BUFFER_HPP
