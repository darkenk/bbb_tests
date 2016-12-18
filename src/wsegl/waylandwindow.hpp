#ifndef WAYLANDWINDOW_HPP
#define WAYLANDWINDOW_HPP

#include <pvr2d/pvr2d.h>
#include <dk_utils/noncopyable.hpp>
#include <wayland-client.h>
#include <cstdio>
#include "wsegl_plugin.hpp"
#include <array>
extern "C" {
#include <wayland-kms.h>
#include <wayland-kms-client-protocol.h>
}
#include <libkms/libkms.h>
#include "wsdisplay.hpp"
#include "../drm_1/lib/resources.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


using namespace std;

struct wl_egl_window {
    struct wl_surface *surface;

    int width;
    int height;
    int dx;
    int dy;

    int attached_width;
    int attached_height;

    void *dprivate;
    void (*resize_callback)(struct wl_egl_window *, void *);
};

class WaylandDisplay : public WSDisplay
{
public:
    WaylandDisplay(wl_display* nativeDisplay);
    ~WaylandDisplay();

    static WaylandDisplay* getFromWSEGL(WSEGLDisplayHandle handle) {
        return reinterpret_cast<WaylandDisplay*>(handle);
    }

    kms_driver* getKmsDriver() {
        return mKmsDriver;
    }

    wl_kms* getKmsInterface() {
        return mKmsInterface;
    }

    int getDrmFd() {
        return mDrmFd;
    }

private:
    wl_display* mNativeDisplay;
    wl_event_queue* mEventQueue;
    wl_kms* mKmsInterface;
    int mDrmFd;
    kms_driver* mKmsDriver;

    static const struct wl_registry_listener sRegistryListener;
    static const struct wl_kms_listener sKmsListener;

    static void registryHandler(void *data, struct wl_registry *registry, uint32_t name,
        const char *interface, uint32_t version);
    static void kmsDeviceHandler(void *data, struct wl_kms *wl_kms, const char *name);
    static void kmsFormatHandler(void *data, struct wl_kms *wl_kms, uint32_t format);
    static void kmsAuthenticatedHandler(void* data, wl_kms *wl_kms);

    void bindKms(struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    void authenticateKms(const char* name);
    void authenticated();
};

class WSWaylandBuffer : NonCopyable
{
public:
    WSWaylandBuffer(WaylandDisplay* disp, wl_egl_window* nativeWindow): mContext(disp->getContext()) {
        unsigned attribs[] = {
            KMS_WIDTH,   static_cast<unsigned int>(nativeWindow->width),
            KMS_HEIGHT,  static_cast<unsigned int>(nativeWindow->height),
            KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
            KMS_TERMINATE_PROP_LIST
        };
        kms_bo_create(disp->getKmsDriver(), attribs, &mBuffer);

        kms_bo_get_prop(mBuffer, KMS_PITCH, &mStride);
        kms_bo_get_prop(mBuffer, KMS_HANDLE, &mHandle);
        mFormat = WL_KMS_FORMAT_ABGR8888;
        mHeight = nativeWindow->height;
        mWidth = nativeWindow->width;
//        mSize = mStride * mHeight;
        void* out;
        kms_bo_map(mBuffer, &out);
        auto ret = PVR2DMemWrap(mContext, out, 0, mStride * nativeWindow->height, NULL, &mMemInfo);
        fprintf(stderr, "WSWaylandBuffer 0x%x, Size: %d\n", ret, mStride);
        fprintf(stderr, "WSWaylandBuffer 0x%lx, Base: %p, PrivateData %p, PrivateMapData %p\n", mMemInfo->ui32DevAddr, mMemInfo->pBase,
                mMemInfo->hPrivateData, mMemInfo->hPrivateMapData);
        if (ret != PVR2D_OK) {
            throw std::exception();
        }
        int prime;
        int err = drmPrimeHandleToFD(disp->getDrmFd(), mHandle, 0, &prime);
        kms_bo_unmap(mBuffer);
        fprintf(stderr, "Err %d", err);
        mWaylandBuffer = wl_kms_create_buffer(disp->getKmsInterface(), prime, mWidth, mHeight, mStride, mFormat, 0);
        //kms_bo_map(mBuffer, &out);
    }

    ~WSWaylandBuffer() {
        //kms_bo_unmap(mBuffer);
        kms_bo_destroy(&mBuffer);
        PVR2DMemFree(mContext, mMemInfo);
    }

    PVR2DMEMINFO* getMemInfo() {
        return mMemInfo;
    }

    wl_buffer* getWaylandBuffer() {
        return mWaylandBuffer;
    }

private:
    PVR2DCONTEXTHANDLE mContext;
    PVR2DMEMINFO* mMemInfo;
    kms_bo* mBuffer;
    uint32_t mHandle;
    uint32_t mStride;
    uint32_t mWidth;
    uint32_t mHeight;
    wl_kms_format mFormat;
    wl_buffer* mWaylandBuffer;
};

class WSWaylandWindow : NonCopyable
{
public:
    WSWaylandWindow(WaylandDisplay* display, NativeWindowType nativeWindow):
        mContext(display->getContext()) {
        auto w = reinterpret_cast<struct wl_egl_window*>(nativeWindow);
        fprintf(stderr, "DK %dx%d", w->width, w->height);
        mFormat = WSEGL_PIXELFORMAT_ABGR8888;
        mHeight = w->height;
        mWidth = w->width;
        mStride = w->width;
        mIndex = 0;
        mSurface = w->surface;
        for (unsigned int i = 0; i < BUFFER_COUNT; i++) {
            mBuffers[i] = new WSWaylandBuffer(display, w);
        }
    }
    ~WSWaylandWindow() {
        for (auto b : mBuffers) {
            delete b;
        }
    }

    static WSWaylandWindow* getFromWSEGL(WSEGLDrawableHandle handle) {
        return reinterpret_cast<WSWaylandWindow*>(handle);
    }

    unsigned long getWidth() const { return mWidth; }
    unsigned long getHeight() const { return mHeight; }
    unsigned long getStride() const { return mStride; }
    WSEGLPixelFormat getFormat() const {return mFormat; }

    WSWaylandBuffer* getFrontBuffer() {
        return mBuffers[mIndex];
    }

    WSWaylandBuffer* getBackBuffer() {
        return mBuffers[(mIndex - 1) % BUFFER_COUNT];
    }

    void swapBuffers() {
        //PVR2DQueryBlitsComplete(mContext, getBackBuffer()->getMemInfo(), 1);
        wl_surface_attach(mSurface, getFrontBuffer()->getWaylandBuffer(), 0, 0);
        wl_surface_commit(mSurface);
        mIndex = (mIndex + 1) % BUFFER_COUNT;
        fprintf(stderr, "Current buffer %d\n", mIndex);
    }

private:
    static constexpr size_t BUFFER_COUNT = 2;
    unsigned int mIndex;
    PVR2DCONTEXTHANDLE mContext;
    unsigned long mWidth;
    unsigned long mHeight;
    unsigned long mStride;
    WSEGLPixelFormat mFormat;
    std::array<WSWaylandBuffer*, BUFFER_COUNT> mBuffers;
    wl_surface* mSurface;
};


class WaylandWindow
{
public:
    WaylandWindow();
};

#endif // WAYLANDWINDOW_HPP
