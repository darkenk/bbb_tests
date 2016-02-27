#ifndef BUFFERKMS_HPP
#define BUFFERKMS_HPP

#include "buffer.hpp"

#include <libkms/libkms.h>

class KmsBuffer : public Buffer
{
public:
    KmsBuffer(kms_driver* driver, uint32_t width, uint32_t height);
    virtual ~KmsBuffer();

    void clear(uint8_t color);
    virtual uint8_t* map() { return nullptr; }

private:
    kms_bo* mBuffer;
    uint8_t* mMappedBuffer;
};

#endif // BUFFERKMS_HPP
