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

namespace WSEGL {

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

//    int roundtrip();

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


    void bindKms(struct wl_registry *registry, uint32_t name, uint32_t version);
    void authenticateKms(const char* name);
    void authenticated();
};

class WSWaylandBuffer :public Buffer
{
public:
    WSWaylandBuffer(WaylandDisplay* disp, wl_egl_window* nativeWindow):
        Buffer(disp->getContext()) {
        unsigned width = nativeWindow->width;
        unsigned stride = upperPowerOfTwo(width);
        unsigned height = nativeWindow->height;
        unsigned handle = handle;
        auto format = WL_KMS_FORMAT_ARGB8888;
        mBuffer = createKmsBuffer(disp->getKmsDriver(), stride, height);

        kms_bo_get_prop(mBuffer, KMS_PITCH, &stride);
        kms_bo_get_prop(mBuffer, KMS_HANDLE, &handle);
        stride /= getSizeOfPixel(convertWaylandFormatToPVR2DFormat(format));

        void* out;
        kms_bo_map(mBuffer, &out);
        init(width, height, stride, convertWaylandFormatToPVR2DFormat(format), out);
        kms_bo_unmap(mBuffer);
        int prime;
        drmPrimeHandleToFD(disp->getDrmFd(), handle, 0, &prime);
        mWaylandBuffer = wl_kms_create_buffer(disp->getKmsInterface(), prime, width,
                                              height, stride, format, 0);
    }

    ~WSWaylandBuffer() {
        kms_bo_destroy(&mBuffer);
    }

    wl_buffer* getWaylandBuffer() {
        return mWaylandBuffer;
    }

private:
    kms_bo* mBuffer;
    wl_buffer* mWaylandBuffer;

    kms_bo* createKmsBuffer(kms_driver* driver, unsigned width, unsigned height) {
        unsigned attribs[] = {
            KMS_WIDTH,   width,
            KMS_HEIGHT,  height,
            KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
            KMS_TERMINATE_PROP_LIST
        };
        kms_bo* out;
        kms_bo_create(driver, attribs, &out);
        return out;
    }

    unsigned long upperPowerOfTwo(unsigned long v) {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

};

class WSWaylandWindow : public Drawable
{
public:
    WSWaylandWindow(WaylandDisplay* display, NativeWindowType nativeWindow):
        Drawable(display->getContext()), mBufferConsumedByCompositor(true)
    {
        auto w = reinterpret_cast<struct wl_egl_window*>(nativeWindow);
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

}

#endif // WAYLANDWINDOW_HPP
