#ifndef DRAWABLE_HPP
#define DRAWABLE_HPP

#include <dk_utils/noncopyable.hpp>
#include <dk_utils/exceptions.hpp>
#include "wsegl_plugin.hpp"
#include <array>
#include <pvr2d/pvr2d.h>
#include <wayland-kms-client-protocol.h>

namespace WSEGL {

class Buffer : NonCopyable {
public:
    Buffer(PVR2DCONTEXTHANDLE ctx): mWidth(-1), mHeight(-1), mStride(-1), mFormat(WSEGL_PIXELFORMAT_8888),
        mMemInfo(nullptr), mContext(ctx) {}

    virtual ~Buffer() {
        if (mMemInfo) {
            PVR2DMemFree(mContext, mMemInfo);
        }
    }

    PVR2DMEMINFO* getMemInfo() {
        return mMemInfo;
    }

    int getWidth() const {
        return mWidth;
    }

    int getHeight() const {
        return mHeight;
    }

    int getStride() const {
        return mStride;
    }

    WSEGLPixelFormat getFormat() const {
        return mFormat;
    }

protected:
    void init(int w, int h, int s, WSEGLPixelFormat f, void* mem) {
        mWidth = w;
        mHeight = h;
        mStride = s;
        mFormat = f;
        PVR2DMemWrap(mContext, mem, 0, mStride * mHeight, nullptr, &mMemInfo);
    }

    WSEGLPixelFormat convertWaylandFormatToPVR2DFormat(uint32_t format) {
        return convertWaylandFormatToPVR2DFormat(static_cast<wl_kms_format>(format));
    }

    WSEGLPixelFormat convertWaylandFormatToPVR2DFormat(wl_kms_format format) {
        switch(format) {
        case WL_KMS_FORMAT_RGB565: return WSEGL_PIXELFORMAT_RGB565;
        case WL_KMS_FORMAT_ARGB8888: return WSEGL_PIXELFORMAT_ARGB8888;
        default:
            throw Exception<Buffer>("Not supported format " + std::to_string(format));
        }
    }

    int getSizeOfPixel(WSEGLPixelFormat format) {
        switch(format) {
        case WSEGL_PIXELFORMAT_ARGB8888: return 4;
        case WSEGL_PIXELFORMAT_RGB565: return 2;
        default:
            throw Exception<Buffer>("Not supported format " + std::to_string(format));
        }
    }

private:
    int mWidth;
    int mHeight;
    int mStride;
    WSEGLPixelFormat mFormat;
    PVR2DMEMINFO* mMemInfo;
    PVR2DCONTEXTHANDLE mContext;
};

class Drawable : NonCopyable
{
public:
    Drawable(PVR2DCONTEXTHANDLE ctx):  mIndex(0), mPVR2DCxt(ctx) {

    }

    virtual ~Drawable() {}

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

private:
    unsigned int mIndex;
    PVR2DCONTEXTHANDLE mPVR2DCxt;
};

}

#endif // DRAWABLE_HPP
