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
using namespace WSEGL;

static WSEGLCaps const wseglDisplayCaps[] = {
    {WSEGL_CAP_WINDOWS_USE_HW_SYNC, 1},
    {WSEGL_CAP_PIXMAPS_USE_HW_SYNC, 1},
    {WSEGL_NO_CAPS, 0}
};

static bool sIsWayland = false;

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
    Display* d;
    if (sIsWayland) {
        d = new WSEGL::WaylandDisplay(reinterpret_cast<wl_display*>(disp));
    } else {
        d = new GBMDisplay;
    }
    *display = d;
    *config = d->getConfigs();
    *caps = wseglDisplayCaps;
    return WSEGL_SUCCESS;
}

WSEGLError CloseDisplay(WSEGLDisplayHandle display)
{
    LOGVP("DK_%s", __FUNCTION__);
    delete reinterpret_cast<Display*>(display);
    return WSEGL_SUCCESS;
}

WSEGLError CreateWindowDrawable(WSEGLDisplayHandle display, WSEGLConfig * config,
                                WSEGLDrawableHandle* drawable, NativeWindowType window,
                                WSEGLRotationAngle* rotation)
{
    auto d = reinterpret_cast<Display*>(display);
    *drawable = d->createWindow(window);
    *rotation = WSEGL_ROTATE_0;
    return WSEGL_SUCCESS;
}

WSEGLError CreatePixmapDrawable(WSEGLDisplayHandle display, WSEGLConfig *config, WSEGLDrawableHandle *drawable,
                                NativePixmapType nativePixmap, WSEGLRotationAngle *rotation)
{
    if (sIsWayland) {
        return WSEGL_BAD_NATIVE_ENGINE;
    }
    auto d = reinterpret_cast<GBMDisplay*>(display);
    *drawable = new GbmPixmapDrawable(d->getContext(), nativePixmap);
    *rotation = WSEGL_ROTATE_0;
    return WSEGL_SUCCESS;
}

WSEGLError DeleteDrawable(WSEGLDrawableHandle drawable)
{
    delete reinterpret_cast<WSEGL::Drawable*>(drawable);
    return WSEGL_SUCCESS;
}

WSEGLError SwapDrawable(WSEGLDrawableHandle window, unsigned long)
{
    reinterpret_cast<WSEGL::Drawable*>(window)->swapBuffers();
    return WSEGL_SUCCESS;
}

WSEGLError SwapControlInterval(WSEGLDrawableHandle, unsigned long)
{
    LOGVP("DK_%s", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError WaitNative(WSEGLDrawableHandle, unsigned long)
{
    LOGVP("DK_%s\n", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError CopyFromDrawable(WSEGLDrawableHandle, NativePixmapType)
{
    LOGVP("DK_%s", __FUNCTION__);
    return WSEGL_BAD_MATCH;
}

WSEGLError CopyFromPBuffer(void *, unsigned long, unsigned long, unsigned long, WSEGLPixelFormat, NativePixmapType)
{
    LOGVP("DK_%s", __FUNCTION__);
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
    return WSEGL_SUCCESS;
}

WSEGLError ConnectDrawable(WSEGLDrawableHandle)
{
    return WSEGL_SUCCESS;
}

WSEGLError DisconnectDrawable(WSEGLDrawableHandle)
{
    LOGVP("DK_%s", __FUNCTION__);
    return WSEGL_SUCCESS;
}

WSEGLError FlagStartFrame(void)
{
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
