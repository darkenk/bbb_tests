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

using namespace std;


static WSEGLCaps const wseglDisplayCaps[] = {
    {WSEGL_CAP_WINDOWS_USE_HW_SYNC, 1},
    {WSEGL_CAP_PIXMAPS_USE_HW_SYNC, 1},
    {WSEGL_NO_CAPS, 0}
};

class WSDisplay : NonCopyable
{
public:
    WSDisplay() {
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

    ~WSDisplay() {
        auto ret = PVR2DDestroyDeviceContext(mContext);
        if (ret != PVR2D_OK) {
            fprintf(stderr, "Cannot destroy device context\n");
        }
    }

    static WSDisplay* getFromWSEGL(WSEGLDisplayHandle handle) {
        return reinterpret_cast<WSDisplay*>(handle);
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
    if (head == reinterpret_cast<void*>(gbm_create_device)) {
        return WSEGL_SUCCESS;
    } else {
        return WSEGL_BAD_NATIVE_DISPLAY;
    }
}

WSEGLError InitialiseDisplay(NativeDisplayType, WSEGLDisplayHandle* display, const WSEGLCaps** caps,
                             WSEGLConfig** config)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    auto d = new WSDisplay;
    *display = reinterpret_cast<WSEGLDisplayHandle>(d);
    *caps = wseglDisplayCaps;
    *config = d->getConfigs();
    return WSEGL_SUCCESS;
}

WSEGLError CloseDisplay(WSEGLDisplayHandle display)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    delete WSDisplay::getFromWSEGL(display);
    return WSEGL_SUCCESS;
}

WSEGLError CreateWindowDrawable(WSEGLDisplayHandle display, WSEGLConfig *,
                                WSEGLDrawableHandle* drawable, NativeWindowType window,
                                WSEGLRotationAngle* rotation)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    auto d = WSDisplay::getFromWSEGL(display);
    auto s = reinterpret_cast<struct gbm_kms_surface*>(window);

    auto w = new WSWindow(d->getContext(), s);
    *drawable = w;
    *rotation = WSEGL_ROTATE_0;
    return WSEGL_SUCCESS;
}

WSEGLError CreatePixmapDrawable(WSEGLDisplayHandle, WSEGLConfig *, WSEGLDrawableHandle *, NativePixmapType, WSEGLRotationAngle *)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_CANNOT_INITIALISE;
}

WSEGLError DeleteDrawable(WSEGLDrawableHandle drawable)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    delete WSWindow::getFromWSEGL(drawable);
    return WSEGL_SUCCESS;
}

WSEGLError SwapDrawable(WSEGLDrawableHandle window, unsigned long)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    WSWindow::getFromWSEGL(window)->swapBuffers();
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
    auto w = WSWindow::getFromWSEGL(drawable);
    memset(sourceParams, 0, sizeof (WSEGLDrawableParams));
    memset(renderParams, 0, sizeof (WSEGLDrawableParams));
    sourceParams->ui32Width = w->getWidth();
    sourceParams->ui32Height = w->getHeight();
    sourceParams->ui32Stride = w->getStride();
    sourceParams->ePixelFormat = w->getFormat();
    auto memInfo = w->getFrontBuffer()->getMemInfo();
    sourceParams->pvLinearAddress = memInfo->pBase;
    sourceParams->ui32HWAddress = memInfo->ui32DevAddr;
    sourceParams->ulFlags = memInfo->ulFlags;

    renderParams->ui32Width = w->getWidth();
    renderParams->ui32Height = w->getHeight();
    renderParams->ui32Stride = w->getStride();
    renderParams->ePixelFormat = w->getFormat();
    memInfo = w->getBackBuffer()->getMemInfo();
    renderParams->pvLinearAddress = memInfo->pBase;
    renderParams->ui32HWAddress = memInfo->ui32DevAddr;
    renderParams->ulFlags = memInfo->ulFlags;
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
