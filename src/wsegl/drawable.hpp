#ifndef DRAWABLE_HPP
#define DRAWABLE_HPP

#include <dk_utils/noncopyable.hpp>
#include "wsegl_plugin.hpp"
#include <array>
#include <pvr2d/pvr2d.h>

namespace WSEGL {

class Buffer : NonCopyable {
public:
    Buffer() {}
    PVR2DMEMINFO* getMemInfo() {
        return mMemInfo;
    }

    virtual ~Buffer() {}

protected:
    PVR2DMEMINFO* mMemInfo;
};

class Drawable : NonCopyable
{
public:
    Drawable(PVR2DCONTEXTHANDLE ctx, int w, int h, int s, WSEGLPixelFormat f):
        mWidth(w), mHeight(h), mStride(s), mFormat(f), mIndex(0), mPVR2DCxt(ctx) {

    }

    Drawable(PVR2DCONTEXTHANDLE ctx):  mIndex(0), mPVR2DCxt(ctx) {

    }

    virtual ~Drawable() {}

    int getWidth() const { return mWidth; }
    int getHeight() const { return mHeight; }
    int getStride() const { return mStride; }
    WSEGLPixelFormat getFormat() const {return mFormat; }

    Buffer* getFrontBuffer() {
        return mBuffers[mIndex];
    }

    Buffer* getBackBuffer() {
        return mBuffers[(mIndex - 1) % BUFFER_COUNT];
    }

    virtual void swapBuffers() {
        mIndex = (mIndex + 1) % BUFFER_COUNT;
    }

protected:
    static constexpr size_t BUFFER_COUNT = 2;
    std::array<Buffer*, BUFFER_COUNT> mBuffers;

    unsigned int getIndex() {
        return mIndex;
    }

    PVR2DCONTEXTHANDLE getPvrCtx() {
        return mPVR2DCxt;
    }
    void init(int w, int h, int s, WSEGLPixelFormat f) {
        mWidth = w;
        mHeight = h;
        mStride = s;
        mFormat = f;
    }

private:
    int mWidth;
    int mHeight;
    int mStride;
    WSEGLPixelFormat mFormat;
    unsigned int mIndex;
    PVR2DCONTEXTHANDLE mPVR2DCxt;
};

}

#endif // DRAWABLE_HPP
