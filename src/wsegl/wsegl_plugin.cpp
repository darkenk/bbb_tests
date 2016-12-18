#include "wsegl_plugin.hpp"
#include <cstdlib>
#include <cstdio>
#include <pvr2d/pvr2d.h>
#include <array>
#include <vector>
#include <gbm/gbm.h>
#include <cassert>
#include <dk_utils/noncopyable.hpp>

#include "gbm_kmsint.h"
#include <cstring>

#include <wayland-egl.h>
#include "waylandwindow.hpp"
#include <sys/mman.h>

using namespace std;

static WSEGLCaps const wseglDisplayCaps[] = {
    {WSEGL_CAP_WINDOWS_USE_HW_SYNC, 1},
    {WSEGL_CAP_PIXMAPS_USE_HW_SYNC, 1},
    {WSEGL_NO_CAPS, 0}
};

static bool sIsWayland = false;

class WSDisplayGBM : NonCopyable
{
public:
    WSDisplayGBM() {
        int deviceCount = PVR2DEnumerateDevices(0);
        vector<PVR2DDEVICEINFO> devices(deviceCount);
        PVR2DEnumerateDevices(&devices[0]);
        assert(devices.size());

        for (auto dev : devices) {
            fprintf(stderr, "DK_%s ulDevId: %ld, name: %s\n", __FUNCTION__, dev.ulDevID, dev.szDeviceName);
        }

        auto ret = PVR2DCreateDeviceContext(devices[0].ulDevID, &mContext, 0);
        if (ret != PVR2D_OK) {
            fprintf(stderr, "Cannot create device context\n");
        }
        fprintf(stderr, "DK_%s Context created 0x%x\n", __FUNCTION__, mContext);

        mConfigs[0].ui32DrawableType = WSEGL_DRAWABLE_WINDOW;
        mConfigs[0].ePixelFormat = WSEGL_PIXELFORMAT_8888;
        mConfigs[0].ulNativeRenderable = WSEGL_FALSE;
        mConfigs[0].ulFrameBufferLevel = 0;
        mConfigs[0].ulNativeVisualID = 0;
        mConfigs[0].ulNativeVisualType = 0;
        mConfigs[0].eTransparentType = WSEGL_OPAQUE;
        mConfigs[0].ulTransparentColor = 0;
        mConfigs[0].ulFramebufferTarget = WSEGL_TRUE;
    }

    ~WSDisplayGBM() {
        auto ret = PVR2DDestroyDeviceContext(mContext);
        if (ret != PVR2D_OK) {
            fprintf(stderr, "Cannot destroy device context\n");
        }
    }

    static WSDisplayGBM* getFromWSEGL(WSEGLDisplayHandle handle) {
        return reinterpret_cast<WSDisplayGBM*>(handle);
    }

    WSEGLConfig* getConfigs() {
        return mConfigs;
    }

    PVR2DCONTEXTHANDLE getContext() {
        return mContext;
    }

private:
    PVR2DCONTEXTHANDLE mContext;
    WSEGLConfig mConfigs[1];
};

class WSBuffer : NonCopyable
{
public:
    WSBuffer(PVR2DCONTEXTHANDLE ctx, gbm_kms_bo* buffer): mContext(ctx), mBuffer(buffer) {
        if (buffer->addr) {
            if (kms_bo_map(buffer->bo, &buffer->addr)) {
                fprintf(stderr, "Mapping failed\n");
            }
        }
        PVR2DMemWrap(mContext, buffer->addr, 0, buffer->size, NULL, &mMemInfo);
    }

    ~WSBuffer() {
        PVR2DMemFree(mContext, mMemInfo);
    }

    PVR2DMEMINFO* getMemInfo() {
        return mMemInfo;
    }

private:
    PVR2DCONTEXTHANDLE mContext;
    gbm_kms_bo* mBuffer;
    PVR2DMEMINFO* mMemInfo;
};

class WSWindow : NonCopyable
{
public:
    WSWindow(PVR2DCONTEXTHANDLE ctx, struct gbm_kms_surface* surface):
        mContext(ctx), mSurface(surface) {
        mFormat = WSEGL_PIXELFORMAT_ABGR8888;
        auto& buffer = surface->bo[0]->base;
        mHeight = buffer.height;
        mWidth = buffer.width;
        mStride = buffer.stride / 4;
        for (unsigned int i = 0; i < BUFFER_COUNT; i++) {
            mBuffers[i] = new WSBuffer(ctx, surface->bo[i]);
        }
        surface->front = 0;
    }
    ~WSWindow() {
        for (auto b : mBuffers) {
            delete b;
        }
    }

    static WSWindow* getFromWSEGL(WSEGLDrawableHandle handle) {
        return reinterpret_cast<WSWindow*>(handle);
    }

    unsigned long getWidth() const;
    unsigned long getHeight() const;
    unsigned long getStride() const;
    WSEGLPixelFormat getFormat() const;

    WSBuffer* getFrontBuffer() {
        return mBuffers[mSurface->front];
    }

    WSBuffer* getBackBuffer() {
        return mBuffers[(mSurface->front - 1) % BUFFER_COUNT];
    }

    void swapBuffers() {
        mSurface->front = (mSurface->front + 1) % BUFFER_COUNT;
        fprintf(stderr, "Current buffer %d\n", mSurface->front);
    }

private:
    static constexpr size_t BUFFER_COUNT = 2;
    PVR2DCONTEXTHANDLE mContext;
    struct gbm_kms_surface* mSurface;
    unsigned long mWidth;
    unsigned long mHeight;
    unsigned long mStride;
    WSEGLPixelFormat mFormat;
    std::array<WSBuffer*, BUFFER_COUNT> mBuffers;
};

WSEGLError IsDisplayValid(NativeDisplayType disp)
{
    fprintf(stderr, "DK_%s 0x%x\n", __FUNCTION__, disp);
    void* head = *reinterpret_cast<void**>(disp);
    sIsWayland = (head == reinterpret_cast<const void*>(&wl_display_interface));
    if (sIsWayland || head == reinterpret_cast<void*>(gbm_create_device)) {
        fprintf(stderr, "Valid display\n");
        return WSEGL_SUCCESS;
    } else {
        fprintf(stderr, "InValid display\n");
        return WSEGL_BAD_NATIVE_DISPLAY;
    }
}

WSEGLError InitialiseDisplay(NativeDisplayType disp, WSEGLDisplayHandle* display, const WSEGLCaps** caps,
                             WSEGLConfig** config)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    if (true && sIsWayland) {
        auto d = new WaylandDisplay(reinterpret_cast<wl_display*>(disp));
        *display = reinterpret_cast<WSEGLDisplayHandle>(d);
        *caps = wseglDisplayCaps;
        *config = d->getConfigs();
    } else {
        auto d = new WSDisplayGBM;
        *display = reinterpret_cast<WSEGLDisplayHandle>(d);
        *caps = wseglDisplayCaps;
        *config = d->getConfigs();
    }
    return WSEGL_SUCCESS;
}

WSEGLError CloseDisplay(WSEGLDisplayHandle display)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    if (sIsWayland) {
        delete WaylandDisplay::getFromWSEGL(display);
    } else {
        delete WSDisplayGBM::getFromWSEGL(display);
    }
    return WSEGL_SUCCESS;
}

WSEGLError CreateWindowDrawable(WSEGLDisplayHandle display, WSEGLConfig * config,
                                WSEGLDrawableHandle* drawable, NativeWindowType window,
                                WSEGLRotationAngle* rotation)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);

    if (sIsWayland) {
        if (config == NULL || !(config->ui32DrawableType & WSEGL_DRAWABLE_WINDOW)) {
            fprintf(stderr, "selected config does not support window drawables");
            return WSEGL_BAD_CONFIG;
        }
        auto d = WaylandDisplay::getFromWSEGL(display);
        fprintf(stderr, "Pixel Format 0x%x", config->ePixelFormat);
        auto w = new WSWaylandWindow(d, window);
        *drawable = w;
    } else {
        auto d = WSDisplayGBM::getFromWSEGL(display);
        auto s = reinterpret_cast<struct gbm_kms_surface*>(window);
        fprintf(stderr, "DK %dx%d", s->base.width, s->base.height);

        auto w = new WSWindow(d->getContext(), s);
        *drawable = w;
    }
    *rotation = WSEGL_ROTATE_0;
    return WSEGL_SUCCESS;
}

struct wl_kms {
	struct wl_display *display;
	int fd;				/* FD for DRM */
	char *device_name;

	struct kms_auth *auth;		/* for nested authentication */
};

PVR2DMEMINFO* memInfo;

WSEGLError CreatePixmapDrawable(WSEGLDisplayHandle display, WSEGLConfig *, WSEGLDrawableHandle *drawable,
                                NativePixmapType nativePixmap, WSEGLRotationAngle *rotation)
{
    if (sIsWayland) {
        return WSEGL_BAD_NATIVE_ENGINE;
    }
    auto d = WSDisplayGBM::getFromWSEGL(display);
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    auto *buffer = (struct wl_buffer*)nativePixmap;
    void* head = *reinterpret_cast<void**>(nativePixmap);
    auto *kms = wayland_kms_buffer_get(&buffer->resource);
    fprintf(stderr, "Format %d, Height %d, Width %d\n", kms->format, kms->height, kms->width);
    void* addr;
    fprintf(stderr, "kms->fd %d", kms->kms->fd);
    struct drm_mode_map_dumb mmap_arg = {0};
    mmap_arg.handle = kms->handle;
    size_t map_size = kms->stride * kms->height;
    int ret = drmIoctl(kms->kms->fd, DRM_IOCTL_MODE_MAP_DUMB, &mmap_arg);
    fprintf(stderr, "ret %d, offset %p\n", ret, mmap_arg.offset);
    addr = mmap(nullptr, map_size, (PROT_READ | PROT_WRITE), MAP_SHARED, kms->kms->fd, mmap_arg.offset);
    //ret = drmMap(kms->kms->fd, kms->fd, kms->stride * kms->height, &addr);
    fprintf(stderr, "ret %d, addr %p\n", ret, addr);
//    unsigned attribs[] = {
//        KMS_WIDTH,   static_cast<unsigned int>(kms->width),
//        KMS_HEIGHT,  static_cast<unsigned int>(kms->height),
//        KMS_BO_TYPE, KMS_BO_TYPE_SCANOUT_X8R8G8B8,
//        KMS_HANDLE,  kms->handle,
//        KMS_TERMINATE_PROP_LIST
//    };
//    kms_bo_create(kms->, attribs, &mBuffer);
//    if (kms->addr) {
//        if (kms_bo_map(kms->bo, &kms->addr)) {
//            fprintf(stderr, "Mapping failed\n");
//        }
//    }
//    assert(PVR2DMemMap(d->getContext(), 0, (PVR2D_HANDLE)4, &memInfo) == PVR2D_OK);
//    assert(PVR2DMemWrap(d->getContext(), 0, (PVR2D_HANDLE)kms->fd, &memInfo) == PVR2D_OK);
    assert(PVR2DMemWrap(d->getContext(), addr, 0, kms->stride * kms->height, NULL, &memInfo) == PVR2D_OK);
    fprintf(stderr, "dziala!\n");
    *drawable = (WSEGLDrawableHandle) 2;
    *rotation = WSEGL_ROTATE_0;
    return WSEGL_SUCCESS;
}

WSEGLError DeleteDrawable(WSEGLDrawableHandle drawable)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    if (drawable == (void*)2) {
        return WSEGL_SUCCESS;
    }
    if (sIsWayland) {
        delete WSWaylandWindow::getFromWSEGL(drawable);
    } else {
        delete WSWindow::getFromWSEGL(drawable);
    }
    return WSEGL_SUCCESS;
}

WSEGLError SwapDrawable(WSEGLDrawableHandle window, unsigned long)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    if (sIsWayland) {
        WSWaylandWindow::getFromWSEGL(window)->swapBuffers();
    } else {
        WSWindow::getFromWSEGL(window)->swapBuffers();
    }
    return WSEGL_SUCCESS;
}

WSEGLError SwapControlInterval(WSEGLDrawableHandle, unsigned long)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError WaitNative(WSEGLDrawableHandle, unsigned long)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError CopyFromDrawable(WSEGLDrawableHandle, NativePixmapType)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError CopyFromPBuffer(void *, unsigned long, unsigned long, unsigned long, WSEGLPixelFormat, NativePixmapType)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError GetDrawableParameters(WSEGLDrawableHandle drawable, WSEGLDrawableParams* sourceParams,
                                 WSEGLDrawableParams* renderParams, unsigned long flags)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    memset(sourceParams, 0, sizeof (WSEGLDrawableParams));
    memset(renderParams, 0, sizeof (WSEGLDrawableParams));
    if (drawable == (void*)0x2) {
        renderParams->ui32Width = sourceParams->ui32Width = 250;
        renderParams->ui32Height = sourceParams->ui32Height = 250;
        renderParams->ui32Stride = sourceParams->ui32Stride = 250;
        renderParams->ePixelFormat = sourceParams->ePixelFormat = WSEGL_PIXELFORMAT_ABGR8888;
        sourceParams->pvLinearAddress = memInfo->pBase;
        sourceParams->ui32HWAddress = memInfo->ui32DevAddr;
        sourceParams->ulFlags = memInfo->ulFlags;

        renderParams->pvLinearAddress = memInfo->pBase;
        renderParams->ui32HWAddress = memInfo->ui32DevAddr;
        renderParams->ulFlags = memInfo->ulFlags;
    } else
    if (sIsWayland) {
        auto w = WSWaylandWindow::getFromWSEGL(drawable);
        renderParams->ui32Width = sourceParams->ui32Width = w->getWidth();
        renderParams->ui32Height = sourceParams->ui32Height = w->getHeight();
        renderParams->ui32Stride = sourceParams->ui32Stride = w->getStride();
        renderParams->ePixelFormat = sourceParams->ePixelFormat = w->getFormat();
        auto memInfo = w->getFrontBuffer()->getMemInfo();
        sourceParams->pvLinearAddress = memInfo->pBase;
        sourceParams->ui32HWAddress = memInfo->ui32DevAddr;
        sourceParams->ulFlags = memInfo->ulFlags;

        memInfo = w->getBackBuffer()->getMemInfo();
        renderParams->pvLinearAddress = memInfo->pBase;
        renderParams->ui32HWAddress = memInfo->ui32DevAddr;
        renderParams->ulFlags = memInfo->ulFlags;
    } else {
        auto w = WSWindow::getFromWSEGL(drawable);
        renderParams->ui32Width = sourceParams->ui32Width = w->getWidth();
        renderParams->ui32Height = sourceParams->ui32Height = w->getHeight();
        renderParams->ui32Stride = sourceParams->ui32Stride = w->getStride();
        renderParams->ePixelFormat = sourceParams->ePixelFormat = w->getFormat();
        auto memInfo = w->getFrontBuffer()->getMemInfo();
        sourceParams->pvLinearAddress = memInfo->pBase;
        sourceParams->ui32HWAddress = memInfo->ui32DevAddr;
        sourceParams->ulFlags = memInfo->ulFlags;

        memInfo = w->getBackBuffer()->getMemInfo();
        renderParams->pvLinearAddress = memInfo->pBase;
        renderParams->ui32HWAddress = memInfo->ui32DevAddr;
        renderParams->ulFlags = memInfo->ulFlags;
    }
    fprintf(stderr, "Render 0x%x, 0x%lx, Source 0x%x 0x%lx", renderParams->pvLinearAddress,
            renderParams->ui32HWAddress, sourceParams->pvLinearAddress,
            sourceParams->ui32HWAddress);
    return WSEGL_SUCCESS;
}

WSEGLError ConnectDrawable(WSEGLDrawableHandle)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    //dummy what should I do here?
    return WSEGL_SUCCESS;
}

WSEGLError DisconnectDrawable(WSEGLDrawableHandle)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    //dummy what should I do here?
    return WSEGL_SUCCESS;
}

WSEGLError FlagStartFrame(void)
{
    fprintf(stderr, "DK_%s=====================================================\n", __FUNCTION__);
    return WSEGL_SUCCESS;
}


static WSEGL_FunctionTable gFunctionTable =
{
    .ui32WSEGLVersion = WSEGL_VERSION,
    .pfnWSEGL_IsDisplayValid = IsDisplayValid,
    .pfnWSEGL_InitialiseDisplay = InitialiseDisplay,
    .pfnWSEGL_CloseDisplay = CloseDisplay,
    .pfnWSEGL_CreateWindowDrawable = CreateWindowDrawable,
    .pfnWSEGL_CreatePixmapDrawable = CreatePixmapDrawable,
    .pfnWSEGL_DeleteDrawable = DeleteDrawable,
    .pfnWSEGL_SwapDrawable = SwapDrawable,
    .pfnWSEGL_SwapControlInterval = SwapControlInterval,
    .pfnWSEGL_WaitNative = WaitNative,
    .pfnWSEGL_CopyFromDrawable = CopyFromDrawable,
    .pfnWSEGL_CopyFromPBuffer = CopyFromPBuffer,
    .pfnWSEGL_GetDrawableParameters = GetDrawableParameters,
    .pfnWSEGL_ConnectDrawable = ConnectDrawable,
    .pfnWSEGL_DisconnectDrawable = DisconnectDrawable,
    .pfnWSEGL_FlagStartFrame = FlagStartFrame
};

PVR2D_EXPORT const WSEGL_FunctionTable* WSEGL_GetFunctionTablePointer(void)
{
    return &gFunctionTable;
}

unsigned long WSWindow::getWidth() const
{
    return mWidth;
}

unsigned long WSWindow::getHeight() const
{
    return mHeight;
}

unsigned long WSWindow::getStride() const
{
    return mStride;
}

WSEGLPixelFormat WSWindow::getFormat() const
{
    return mFormat;
}
