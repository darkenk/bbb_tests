#ifndef WAYLANDWINDOW_HPP
#define WAYLANDWINDOW_HPP

#include <pvr2d/pvr2d.h>
#include <dk_utils/noncopyable.hpp>
#include <wayland-client.h>
#include <cstdio>
#include "wsegl_plugin.hpp"
#include <array>

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


class WaylandDisplay
{
public:
    WaylandDisplay(wl_display* nativeDisplay) {
        mNativeDisplay = nativeDisplay;
        mEventQueue = wl_display_create_queue(nativeDisplay);
        //struct wl_egl_display *egldisplay = wl_egl_display_create((struct wl_display *) nativeDisplay);
    }
    ~WaylandDisplay() {
        wl_event_queue_destroy(mEventQueue);
    }

private:
    wl_display* mNativeDisplay;
    wl_event_queue* mEventQueue;
};

class WSWaylandBuffer : NonCopyable
{
public:
    WSWaylandBuffer(PVR2DCONTEXTHANDLE ctx, wl_egl_window* nativeWindow): mContext(ctx) {
        if (PVR2DMemAlloc(ctx, nativeWindow->width * nativeWindow->height * 4, 0, 0, &mMemInfo)
            != PVR2D_OK) {
                fprintf(stderr, "Allocation failed\n");
        }
    }

    ~WSWaylandBuffer() {
        PVR2DMemFree(mContext, mMemInfo);
    }

    PVR2DMEMINFO* getMemInfo() {
        return mMemInfo;
    }

private:
    PVR2DCONTEXTHANDLE mContext;
    PVR2DMEMINFO* mMemInfo;
};

class WSWaylandWindow : NonCopyable
{
public:
    WSWaylandWindow(PVR2DCONTEXTHANDLE ctx, NativeWindowType nativeWindow):
        mContext(ctx) {
        auto w = reinterpret_cast<struct wl_egl_window*>(nativeWindow);
        fprintf(stderr, "DK %dx%d", w->width, w->height);
        mFormat = WSEGL_PIXELFORMAT_ABGR8888;
        mHeight = w->height;
        mWidth = w->width;
        mStride = w->width * 4;
        for (unsigned int i = 0; i < BUFFER_COUNT; i++) {
            mBuffers[i] = new WSWaylandBuffer(ctx, w);
        }
        mIndex = 0;
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
};


class WaylandWindow
{
public:
    WaylandWindow();
};

#endif // WAYLANDWINDOW_HPP
