#ifndef MOCKWAYLAND_HPP
#define MOCKWAYLAND_HPP

#include <gmock/gmock.h>
#include <wayland-kms.h>

class MockWayland
{
public:
    MockWayland();
    ~MockWayland();

    MOCK_METHOD1(kms_buffer_get, wl_kms_buffer*(wl_resource*));
};

#endif // MOCKWAYLAND_HPP
