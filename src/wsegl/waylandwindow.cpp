#include "waylandwindow.hpp"
#include <wayland-kms-client-protocol.h>

#include <cstring>
#include <libkms/libkms.h>
#include "../drm_1/lib/resources.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;
using namespace WSEGL;

const struct wl_registry_listener WaylandDisplay::sRegistryListener = {
    WaylandDisplay::registryHandler,
    nullptr
};

const struct wl_kms_listener WaylandDisplay::sKmsListener = {
    WaylandDisplay::kmsDeviceHandler,
    WaylandDisplay::kmsFormatHandler,
    WaylandDisplay::kmsAuthenticatedHandler
};

const struct wl_callback_listener WaylandDisplay::sSyncListener = {
     WaylandDisplay::syncCallback
};

const struct wl_callback_listener WaylandWindow::sThrottleListener = {
    .done = throttleCallback
};


WaylandDisplay::WaylandDisplay(wl_display* nativeDisplay):
    mKmsInterface(nullptr), mDrmFd(-1), mKmsDriver(nullptr)
{
    mNativeDisplay = nativeDisplay;
    mEventQueue = wl_display_create_queue(nativeDisplay);
    auto registry = wl_display_get_registry(nativeDisplay);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(registry), mEventQueue);
    wl_registry_add_listener(registry, &sRegistryListener, this);
    wl_display_roundtrip_queue(nativeDisplay, mEventQueue);
}

WaylandDisplay::~WaylandDisplay() {
    wl_event_queue_destroy(mEventQueue);
}

//int WaylandDisplay::roundtrip() {
//    struct wl_callback *callback;
//    int done = 0, ret = 0;

//    callback = wl_display_sync(mNativeDisplay);
//    wl_callback_add_listener(callback, &sSyncListener, &done);
//    wl_proxy_set_queue((struct wl_proxy *) callback, mEventQueue);
//    while (ret != -1 && !done)
//        ret = wl_display_dispatch_queue(mNativeDisplay, mEventQueue);

//    wl_callback_destroy(callback);

//    return ret;
//}

void WaylandDisplay::registryHandler(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    if (strcmp(interface, wl_kms_interface.name) == 0) {
        auto wd = reinterpret_cast<WaylandDisplay*>(data);
        wd->bindKms(registry, name, version);
    }
}

void WaylandDisplay::kmsDeviceHandler(void* data, wl_kms* wl_kms, const char* name)
{
    auto wd = reinterpret_cast<WaylandDisplay*>(data);
    wd->authenticateKms(name);
}

void WaylandDisplay::kmsFormatHandler(void* data, wl_kms* wl_kms, uint32_t format)
{
    auto wd = reinterpret_cast<WaylandDisplay*>(data);
    wd->authenticated();
}

void WaylandDisplay::kmsAuthenticatedHandler(void* data, wl_kms* wl_kms)
{
}

void WaylandDisplay::syncCallback(void* data, wl_callback* /*callback*/, uint32_t)
{
    int *done = reinterpret_cast<int*>(data);
    *done = 1;
}

void WaylandDisplay::bindKms(wl_registry* registry, uint32_t name, uint32_t version)
{
    mKmsInterface = reinterpret_cast<wl_kms*>(wl_registry_bind(registry, name, &wl_kms_interface, version));
    wl_kms_add_listener(mKmsInterface, &sKmsListener, this);
    wl_display_roundtrip_queue(mNativeDisplay, mEventQueue);
}

void WaylandDisplay::authenticateKms(const char* name)
{
    mDrmFd = open(name, O_RDWR | O_CLOEXEC);
    drm_magic_t magic;
    drmGetMagic(mDrmFd, &magic);
    wl_kms_authenticate(mKmsInterface, magic);
    wl_display_roundtrip_queue(mNativeDisplay, mEventQueue);
}

void WaylandDisplay::authenticated()
{
    kms_create(mDrmFd, &mKmsDriver);
}

WaylandWindow::WaylandWindow(WaylandDisplay* display, NativeWindowType nativeWindow):
    Drawable(display->getContext()), mBufferConsumedByCompositor(true)
{
    auto w = reinterpret_cast<struct wl_egl_window*>(nativeWindow);
    mSurface = w->surface;
    for (unsigned int i = 0; i < BUFFER_COUNT; i++) {
        mBuffers[i] = new WaylandBuffer(display, w);
    }
    mDisplay = display;
}

void WaylandWindow::swapBuffers() {
    if (!mBufferConsumedByCompositor) {
        wl_display_roundtrip(mDisplay->getNative());
    }
    mBufferConsumedByCompositor = false;
    auto c = wl_surface_frame(mSurface);
    wl_callback_add_listener(c, &sThrottleListener, this);
    auto b = (WaylandBuffer*)getFrontBuffer();
    wl_surface_attach(mSurface, b->getWaylandBuffer(), 0, 0);
    wl_surface_commit(mSurface);
    WSEGL::Drawable::swapBuffers();
}

void WaylandWindow::throttleCallback(void* data, wl_callback* callback, uint32_t)
{
    auto *w = reinterpret_cast<WaylandWindow*>(data);
    w->mBufferConsumedByCompositor = true;
    wl_callback_destroy(callback);
}

WaylandBuffer::WaylandBuffer(WaylandDisplay* disp, wl_egl_window* nativeWindow):
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
