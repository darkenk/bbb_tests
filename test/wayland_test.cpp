#include "waylandwindow.hpp"
#include <gtest/gtest.h>
#include "mockpvr2d.hpp"
#include "mockdrm.hpp"
#include "mockkms.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <wayland-kms-server-protocol.h>

using namespace std;
using namespace testing;
using namespace WSEGL;

static void kms_authenticate(struct wl_client *client, struct wl_resource *resource,
                             uint32_t magic) {
    wl_resource_post_event(resource, WL_KMS_AUTHENTICATED);
}

const static struct wl_kms_interface kms_interface = {
    .authenticate = kms_authenticate,
    .create_buffer = nullptr,
    .create_mp_buffer = nullptr,
};

// TODO: not needed?
struct wl_kms {};

class FakeWaylandServer
{
public:
    FakeWaylandServer() { }

    void start(const string& socketName) {
        mThreadServer = thread(&FakeWaylandServer::run, this, socketName);
        unique_lock<mutex> lk(mMutexServerReady);
        mThreadStarted.wait(lk);
    }

    void waitForFinish() {
        wl_display_terminate(mDisplay);
        mThreadServer.join();
    }

    ~FakeWaylandServer() {
        if (mThreadServer.joinable()) {
            mThreadServer.join();
        }
    }

private:
    wl_display* mDisplay;
    thread mThreadServer;
    mutex mMutexServerReady;
    condition_variable mThreadStarted;
    wl_kms* mWlKms;

    void run(const string& socketName) {
        mDisplay = wl_display_create();
        mWlKms = new wl_kms();
        wl_display_add_socket(mDisplay, socketName.c_str());
        wl_global_create(mDisplay, &wl_kms_interface, wl_kms_interface.version, nullptr, bindKms);
        mThreadStarted.notify_one();
        wl_display_run(mDisplay);
        wl_display_destroy(mDisplay);
        delete mWlKms;
    }

    static void bindKms(struct wl_client *client, void *data, uint32_t version, uint32_t id) {
        wl_resource* resource = wl_resource_create(client, &wl_kms_interface, version, id);
        wl_resource_set_implementation(resource, &kms_interface, data, NULL);
        wl_resource_post_event(resource, WL_KMS_DEVICE, "fake_kms");
        wl_resource_post_event(resource, WL_KMS_FORMAT, WL_KMS_FORMAT_ARGB8888);
    }
};

class WaylandFixture: public Test
{
public:
    virtual void SetUp() {
        server.start("test_socket");
        display = wl_display_connect("test_socket");
        eglWindow = new wl_egl_window;
        memset(eglWindow, sizeof(*eglWindow), 0);
        eglWindow->height = 12;
        eglWindow->width = 12;
        wd = new WaylandDisplay(display);
        buffer = new WSWaylandBuffer(wd, eglWindow);
        window = new WSWaylandWindow(wd, reinterpret_cast<NativeWindowType>(eglWindow));
    }

    virtual void TearDown() {
        delete window;
        delete buffer;
        delete wd;
        delete eglWindow;
        wl_display_disconnect(display);
        server.waitForFinish();
    }

protected:
    NiceMock<MockPVR2D> pvr2d;
    NiceMock<MockDRM> drm;
    NiceMock<MockKms> kms;
    FakeWaylandServer server;
    wl_display* display;
    wl_egl_window* eglWindow;
    WaylandDisplay* wd;
    WSWaylandBuffer* buffer;
    WSWaylandWindow* window;
};

TEST_F(WaylandFixture, is_it_compile) {
}

TEST_F(WaylandFixture, wl_kms_interface_created) {
    EXPECT_NE(nullptr, wd->getKmsInterface());
}

TEST_F(WaylandFixture, kms_driver_created) {
    EXPECT_NE(nullptr, wd->getKmsDriver());
}

TEST_F(WaylandFixture, buffer_has_stride_rounded_to_power_of_2) {
    EXPECT_EQ(16, buffer->getStride());
}

TEST_F(WaylandFixture, window_contains_front_buffer_with_correct_size) {
    EXPECT_EQ(12, window->getFrontBuffer()->getWidth());
}
