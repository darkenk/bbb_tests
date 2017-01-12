#include "mockwayland.hpp"
#include <dk_utils/exceptions.hpp>

using namespace testing;

static MockWayland* gMock = nullptr;

MockWayland::MockWayland()
{
    if (gMock) {
        throw Exception<MockWayland>("gMock already created");
    }

    gMock = this;

    ON_CALL(*this, kms_buffer_get(_)).WillByDefault(Return(nullptr));
}

MockWayland::~MockWayland()
{
    gMock = nullptr;
}

wl_kms_buffer* wayland_kms_buffer_get(wl_resource* resource)
{
    return gMock->kms_buffer_get(resource);
}

extern "C" {

extern const struct wl_interface wl_buffer_interface;

static const struct wl_message wl_kms_requests[] = {
    { "authenticate", "u", nullptr },
    { "create_buffer", "nhiiuuu", nullptr },
    { "create_mp_buffer", "2niiuhuhuhu", nullptr },
};

static const struct wl_message wl_kms_events[] = {
    { "device", "s", nullptr },
    { "format", "u", nullptr },
    { "authenticated", "", nullptr },
};

extern const struct wl_interface wl_kms_interface = {
    "wl_kms", 2,
    3, wl_kms_requests,
    3, wl_kms_events,
};

}
