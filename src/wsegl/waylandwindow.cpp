#include "waylandwindow.hpp"
#include <wayland-kms-client-protocol.h>

#include <cstring>
#include <libkms/libkms.h>
#include "../drm_1/lib/resources.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

const struct wl_registry_listener WaylandDisplay::sRegistryListener = {
    WaylandDisplay::registryHandler,
    nullptr
};

const struct wl_kms_listener WaylandDisplay::sKmsListener = {
    WaylandDisplay::kmsDeviceHandler,
    WaylandDisplay::kmsFormatHandler,
    WaylandDisplay::kmsAuthenticatedHandler
};

WaylandDisplay::WaylandDisplay(wl_display* nativeDisplay) {
    mNativeDisplay = nativeDisplay;
    mEventQueue = wl_display_create_queue(nativeDisplay);
    auto registry = wl_display_get_registry(nativeDisplay);
    wl_proxy_set_queue(reinterpret_cast<wl_proxy*>(registry), mEventQueue);
    wl_display_sync(nativeDisplay);
    wl_registry_add_listener(registry, &sRegistryListener, this);
    wl_display_roundtrip_queue(nativeDisplay, mEventQueue);

}

WaylandDisplay::~WaylandDisplay() {
    wl_event_queue_destroy(mEventQueue);
}

void WaylandDisplay::registryHandler(void* data, wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    if (strcmp(interface, "wl_kms") == 0) {
        auto wd = reinterpret_cast<WaylandDisplay*>(data);
        wd->bindKms(registry, name, interface, version);
        fprintf(stderr, "wl_kms found\n");
    }
}

void WaylandDisplay::kmsDeviceHandler(void* data, wl_kms* wl_kms, const char* name)
{
    auto wd = reinterpret_cast<WaylandDisplay*>(data);
    fprintf(stderr, "kmsDeviceHandler = %s\n", name);
    wd->authenticateKms(name);
}

void WaylandDisplay::kmsFormatHandler(void* data, wl_kms* wl_kms, uint32_t format)
{
    fprintf(stderr, "Format 0x%x\n", format);
    auto wd = reinterpret_cast<WaylandDisplay*>(data);
    wd->authenticated();
}

void WaylandDisplay::kmsAuthenticatedHandler(void* data, wl_kms* wl_kms)
{
    fprintf(stderr, "kms authenticated\n");
}

void WaylandDisplay::bindKms(wl_registry* registry, uint32_t name, const char* interface, uint32_t version)
{
    mKmsInterface = reinterpret_cast<wl_kms*>(wl_registry_bind(registry, name, &wl_kms_interface, 1));
    wl_kms_add_listener(mKmsInterface, &sKmsListener, this);
    wl_display_roundtrip_queue(mNativeDisplay, mEventQueue);
}

void WaylandDisplay::authenticateKms(const char* name)
{
    mDrmFd = open(name, O_RDWR | O_CLOEXEC);
    fprintf(stderr, "FD %d\n", mDrmFd);
    drm_magic_t magic;
    drmGetMagic(mDrmFd, &magic);
    wl_kms_authenticate(mKmsInterface, magic);
    wl_display_roundtrip_queue(mNativeDisplay, mEventQueue);
}

void WaylandDisplay::authenticated()
{
    kms_create(mDrmFd, &mKmsDriver);
}
