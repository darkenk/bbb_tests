#include "wsegl_plugin.hpp"
#include <cstdlib>
#include <cstdio>
#include <pvr2d/pvr2d.h>
#include <array>
#include <vector>
#include <gbm/gbm.h>
#include <cassert>
#include <dk_utils/noncopyable.hpp>
#include <dk_utils/logger.hpp>

#include <cstring>

#include <wayland-egl.h>
#include "waylandwindow.hpp"
#include <sys/mman.h>
#include "gbmdrawable.hpp"

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
            LOGVP("DK_%s ulDevId: %ld, name: %s\n", __FUNCTION__, dev.ulDevID, dev.szDeviceName);
        }

        auto ret = PVR2DCreateDeviceContext(devices[0].ulDevID, &mContext, 0);
        if (ret != PVR2D_OK) {
            LOGVP("Cannot create device context\n");
        }

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
            LOGVP("Cannot destroy device context\n");
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

class WSBuffer : public WSEGL::Buffer
{
public:
    WSBuffer(PVR2DCONTEXTHANDLE ctx, gbm_kms_bo* buffer): mContext(ctx), mBuffer(buffer) {
        if (buffer->addr) {
            if (kms_bo_map(buffer->bo, &buffer->addr)) {
                LOGVP("Mapping failed\n");
            }
        }
        PVR2DMemWrap(mContext, buffer->addr, PVR2D_WRAPFLAG_CONTIGUOUS, buffer->size, NULL, &mMemInfo);
    }

    ~WSBuffer() {
        PVR2DMemFree(mContext, mMemInfo);
    }

private:
    PVR2DCONTEXTHANDLE mContext;
    gbm_kms_bo* mBuffer;
};

class WSWindow : public WSEGL::Drawable
{
public:
    WSWindow(PVR2DCONTEXTHANDLE ctx, struct gbm_kms_surface* surface):
        Drawable(ctx), mSurface(surface) {
        for (unsigned int i = 0; i < BUFFER_COUNT; i++) {
            mBuffers[i] = new WSBuffer(ctx, surface->bo[i]);
        }
        surface->front = 0;
    }
    virtual ~WSWindow() {
        for (auto b : mBuffers) {
            delete b;
        }
    }

    static WSWindow* getFromWSEGL(WSEGLDrawableHandle handle) {
        return reinterpret_cast<WSWindow*>(handle);
    }

    void swapBuffers() {
        WSEGL::Drawable::swapBuffers();
        mSurface->front = getIndex();
    }

private:
    struct gbm_kms_surface* mSurface;
};

WSEGLError IsDisplayValid(NativeDisplayType disp)
{
    void* head = *reinterpret_cast<void**>(disp);
    sIsWayland = (head == reinterpret_cast<const void*>(&wl_display_interface));
    if (sIsWayland || head == reinterpret_cast<void*>(gbm_create_device)) {
        return WSEGL_SUCCESS;
    } else {
        return WSEGL_BAD_NATIVE_DISPLAY;
    }
}

WSEGLError InitialiseDisplay(NativeDisplayType disp, WSEGLDisplayHandle* display, const WSEGLCaps** caps,
                             WSEGLConfig** config)
{
    LOGVP("DK_%s\n", __FUNCTION__);
    if (true && sIsWayland) {
        auto d = new WaylandDisplay(reinterpret_cast<wl_display*>(disp));
        *display = reinterpret_cast<WSEGLDisplayHandle>(d);
        *config = d->getConfigs();
    } else {
        auto d = new WSDisplayGBM;
        *display = reinterpret_cast<WSEGLDisplayHandle>(d);
        *config = d->getConfigs();
    }
    *caps = wseglDisplayCaps;
    return WSEGL_SUCCESS;
}

WSEGLError CloseDisplay(WSEGLDisplayHandle display)
{
    LOGVP("DK_%s\n", __FUNCTION__);
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
    WSEGL::Drawable* w;
    if (sIsWayland) {
        auto d = WaylandDisplay::getFromWSEGL(display);
        w = new WSWaylandWindow(d, window);
    } else {
        auto d = WSDisplayGBM::getFromWSEGL(display);
        auto s = reinterpret_cast<struct gbm_kms_surface*>(window);
        w = new WSWindow(d->getContext(), s);
    }
    *drawable = w;
    *rotation = WSEGL_ROTATE_0;
    return WSEGL_SUCCESS;
}

WSEGLError CreatePixmapDrawable(WSEGLDisplayHandle display, WSEGLConfig *config, WSEGLDrawableHandle *drawable,
                                NativePixmapType nativePixmap, WSEGLRotationAngle *rotation)
{
    if (sIsWayland) {
        return WSEGL_BAD_NATIVE_ENGINE;
    }
    if (config != nullptr) {
        LOGVD("Config %d", config->ePixelFormat);
    }
    auto d = WSDisplayGBM::getFromWSEGL(display);

    *drawable = (WSEGLDrawableHandle) new GbmPixmapDrawable(d->getContext(), nativePixmap);
    *rotation = WSEGL_ROTATE_0;
    return WSEGL_SUCCESS;
}

WSEGLError DeleteDrawable(WSEGLDrawableHandle drawable)
{
//    LOGVP("DK_%s\n", __FUNCTION__);
    delete reinterpret_cast<WSEGL::Drawable*>(drawable);
    return WSEGL_SUCCESS;
}

WSEGLError SwapDrawable(WSEGLDrawableHandle window, unsigned long)
{
    if (sIsWayland) {
        WSWaylandWindow::getFromWSEGL(window)->swapBuffers();
    } else {
        WSWindow::getFromWSEGL(window)->swapBuffers();
    }
    return WSEGL_SUCCESS;
}

WSEGLError SwapControlInterval(WSEGLDrawableHandle, unsigned long)
{
    LOGVP("DK_%s\n", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError WaitNative(WSEGLDrawableHandle, unsigned long)
{
    LOGVP("DK_%s\n", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError CopyFromDrawable(WSEGLDrawableHandle, NativePixmapType)
{
    LOGVP("DK_%s\n", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError CopyFromPBuffer(void *, unsigned long, unsigned long, unsigned long, WSEGLPixelFormat, NativePixmapType)
{
    LOGVP("DK_%s\n", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

static void fillDrawableParams(WSEGLDrawableParams* params, WSEGL::Buffer* b)
{
    memset(params, 0, sizeof (WSEGLDrawableParams));
    params->ui32Width = b->getWidth();
    params->ui32Height = b->getHeight();
    params->ui32Stride = b->getStride();
    params->ePixelFormat = b->getFormat();
    auto memInfo = b->getMemInfo();
    params->pvLinearAddress = memInfo->pBase;
    params->ui32HWAddress = memInfo->ui32DevAddr;
    params->ulFlags = memInfo->ulFlags;
}

WSEGLError GetDrawableParameters(WSEGLDrawableHandle drawable, WSEGLDrawableParams* sourceParams,
                                 WSEGLDrawableParams* renderParams, unsigned long flags)
{
    auto w = reinterpret_cast<WSEGL::Drawable*>(drawable);
    fillDrawableParams(sourceParams, w->getFrontBuffer());
    fillDrawableParams(renderParams, w->getBackBuffer());
    memset(sourceParams, 0, sizeof (WSEGLDrawableParams));
    memset(renderParams, 0, sizeof (WSEGLDrawableParams));
    return WSEGL_SUCCESS;
}

WSEGLError ConnectDrawable(WSEGLDrawableHandle)
{
    LOGVP("DK_%s\n", __FUNCTION__);
    return WSEGL_SUCCESS;
}

WSEGLError DisconnectDrawable(WSEGLDrawableHandle)
{
    LOGVP("DK_%s\n", __FUNCTION__);
    return WSEGL_SUCCESS;
}

WSEGLError FlagStartFrame(void)
{
    LOGVP("DK_%s\n", __FUNCTION__);
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
