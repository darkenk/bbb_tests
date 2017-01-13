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

class WaylandDisplay;

class WaylandBuffer :public Buffer
{
public:
    WaylandBuffer(WaylandDisplay* disp, wl_egl_window* nativeWindow);

    ~WaylandBuffer() override {
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

class WaylandWindow : public Drawable
{
public:
    WaylandWindow(WaylandDisplay* display, NativeWindowType nativeWindow);

    virtual ~WaylandWindow() override {
        for (auto b : mBuffers) {
            delete b;
        }
    }

    virtual void swapBuffers() override;

private:
    wl_surface* mSurface;
    WaylandDisplay* mDisplay;
    bool mBufferConsumedByCompositor;

    static const struct wl_callback_listener sThrottleListener;

    static void throttleCallback(void *data, struct wl_callback *callback, uint32_t /*time*/);

};


class WaylandDisplay : public Display
{
public:
    WaylandDisplay(wl_display* nativeDisplay);
    ~WaylandDisplay() override;

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

    Drawable* createWindow(NativeWindowType window) override {
        return new WaylandWindow(this, window);
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

}

#endif // WAYLANDWINDOW_HPP
