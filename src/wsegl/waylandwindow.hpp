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
#include "drawable.hpp"
#include <unistd.h>
#include <dk_utils/logger.hpp>

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

    wl_display* getNative() {
        return mNativeDisplay;
    }

    int roundtrip();

private:
    wl_display* mNativeDisplay;
    wl_event_queue* mEventQueue;
    wl_kms* mKmsInterface;
    int mDrmFd;
    kms_driver* mKmsDriver;

    static const struct wl_registry_listener sRegistryListener;
    static const struct wl_kms_listener sKmsListener;
    static const struct wl_callback_listener sSyncListener;

    static void registryHandler(void *data, struct wl_registry *registry, uint32_t name,
        const char *interface, uint32_t version);
    static void kmsDeviceHandler(void *data, struct wl_kms *wl_kms, const char *name);
    static void kmsFormatHandler(void *data, struct wl_kms *wl_kms, uint32_t format);
    static void kmsAuthenticatedHandler(void* data, wl_kms *wl_kms);
    static void syncCallback(void *data, struct wl_callback *, uint32_t /*serial*/);


    void bindKms(struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version);
    void authenticateKms(const char* name);
    void authenticated();
};

class WSWaylandBuffer :public WSEGL::Buffer
{
public:
    WSWaylandBuffer(WaylandDisplay* disp, wl_egl_window* nativeWindow): mContext(disp->getContext()) {
        unsigned attribs[] = {
            KMS_WIDTH,   static_cast<unsigned int>(nativeWindow->width + 6),
            KMS_HEIGHT,  static_cast<unsigned int>(nativeWindow->height),
            KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
            KMS_TERMINATE_PROP_LIST
        };
        kms_bo_create(disp->getKmsDriver(), attribs, &mBuffer);

        kms_bo_get_prop(mBuffer, KMS_PITCH, &mStride);
        kms_bo_get_prop(mBuffer, KMS_HANDLE, &mHandle);
        mFormat = WL_KMS_FORMAT_ARGB8888;
        mHeight = nativeWindow->height;
        mWidth = nativeWindow->width + 6;
        int mSize = mStride * nativeWindow->height;

        LOGVD("Stride %d", mStride);
        void* out;
        kms_bo_map(mBuffer, &out);
        auto ret = PVR2DMemWrap(mContext, out, PVR2D_WRAPFLAG_CONTIGUOUS, mSize, NULL, &mMemInfo);
        fprintf(stderr, "WSWaylandBuffer 0x%x, Size: %d\n", ret, mStride);
        fprintf(stderr, "WSWaylandBuffer 0x%lx, Base: %p, PrivateData %p, PrivateMapData %p\n", mMemInfo->ui32DevAddr, mMemInfo->pBase,
                mMemInfo->hPrivateData, mMemInfo->hPrivateMapData);
        LOGVD("Range 0x%x - 0x%x", mMemInfo->ui32DevAddr, mMemInfo->ui32DevAddr + mMemInfo->ui32MemSize );
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

    wl_buffer* getWaylandBuffer() {
        return mWaylandBuffer;
    }

    void dump() {
        uint8_t* out;
        kms_bo_map(mBuffer, (void**)&out);
        FILE* f = fopen("/tmp/wayland-client.data", "wb");
        fwrite(out, sizeof(uint8_t), mStride * mHeight, f);
        fclose(f);
    }

private:
    PVR2DCONTEXTHANDLE mContext;
    kms_bo* mBuffer;
    uint32_t mHandle;
    uint32_t mStride;
    uint32_t mWidth;
    uint32_t mHeight;
    wl_kms_format mFormat;
    wl_buffer* mWaylandBuffer;
};

class WSWaylandWindow : public WSEGL::Drawable
{
public:
    WSWaylandWindow(WaylandDisplay* display, NativeWindowType nativeWindow):
        Drawable(display->getContext(),
            reinterpret_cast<struct wl_egl_window*>(nativeWindow)->width + 6,
            reinterpret_cast<struct wl_egl_window*>(nativeWindow)->height,
            reinterpret_cast<struct wl_egl_window*>(nativeWindow)->width + 6,
            WSEGL_PIXELFORMAT_ARGB8888),
        mBufferConsumedByCompositor(true)
    {
        auto w = reinterpret_cast<struct wl_egl_window*>(nativeWindow);
        LOGVD("%d x %d ---- (%d,%d) ", w->attached_width, w->attached_height, w->dx, w->dy);
        mSurface = w->surface;
        for (unsigned int i = 0; i < BUFFER_COUNT; i++) {
            mBuffers[i] = new WSWaylandBuffer(display, w);
        }
        mDisplay = display;
    }
    virtual ~WSWaylandWindow() {
        for (auto b : mBuffers) {
            delete b;
        }
    }

    static WSWaylandWindow* getFromWSEGL(WSEGLDrawableHandle handle) {
        return reinterpret_cast<WSWaylandWindow*>(handle);
    }

    virtual void swapBuffers() {
        //PVR2DQueryBlitsComplete(mContext, getBackBuffer()->getMemInfo(), 1);
        if (!mBufferConsumedByCompositor) {
            wl_display_roundtrip(mDisplay->getNative());
        }
        mBufferConsumedByCompositor = false;
        auto c = wl_surface_frame(mSurface);
        wl_callback_add_listener(c, &sThrottleListener, this);
        auto b = (WSWaylandBuffer*)getFrontBuffer();
        wl_surface_attach(mSurface, b->getWaylandBuffer(), 0, 0);
        wl_surface_commit(mSurface);
        WSEGL::Drawable::swapBuffers();
        //sleep(1);
//        wl_display_roundtrip(mDisplay->getNative());
//        for(;;) {}
    }

    WaylandDisplay* getDisplay() {
        return mDisplay;
    }


private:
    wl_surface* mSurface;
    WaylandDisplay* mDisplay;
    bool mBufferConsumedByCompositor;

    static const struct wl_callback_listener sThrottleListener;

    static void throttleCallback(void *data, struct wl_callback *callback, uint32_t /*time*/);

};


class WaylandWindow
{
public:
    WaylandWindow();
};

#endif // WAYLANDWINDOW_HPP
