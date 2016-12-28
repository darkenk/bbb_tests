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

#include "gbm_kmsint.h"
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

class WSBuffer : public WSEGL::Buffer
{
public:
    WSBuffer(PVR2DCONTEXTHANDLE ctx, gbm_kms_bo* buffer): mContext(ctx), mBuffer(buffer) {
        if (buffer->addr) {
            if (kms_bo_map(buffer->bo, &buffer->addr)) {
                fprintf(stderr, "Mapping failed\n");
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
        Drawable(ctx,
            surface->bo[0]->base.width,
            surface->bo[0]->base.height,
            surface->bo[0]->base.stride / 4,
            WSEGL_PIXELFORMAT_ABGR8888
        ),
        mSurface(surface) {
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
        fprintf(stderr, "DK %dx%d\n", s->base.width, s->base.height);

        auto w = new WSWindow(d->getContext(), s);
        *drawable = w;
    }
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
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    delete reinterpret_cast<WSEGL::Drawable*>(drawable);
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
    auto w = reinterpret_cast<WSEGL::Drawable*>(drawable);
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
    fprintf(stderr, "Render %p, 0x%lx, Source %p 0x%lx\n", renderParams->pvLinearAddress,
            renderParams->ui32HWAddress, sourceParams->pvLinearAddress,
            sourceParams->ui32HWAddress);

    LOGVD("Render: %d, %d, %d\n"
          "Source: %d, %d, %d", renderParams->ui32Width, renderParams->ui32Height, renderParams->ui32Stride,
          sourceParams->ui32Width, sourceParams->ui32Height, sourceParams->ui32Stride);
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
