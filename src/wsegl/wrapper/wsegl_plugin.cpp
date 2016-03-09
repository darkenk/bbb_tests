#include "wsegl_plugin.hpp"
#include <dlfcn.h>
#include <cstdlib>
#include <cstdio>

static void* gLib = nullptr;
const WSEGL_FunctionTable* (*_WSEGL_GetFunctionTablePointer)(void) = nullptr;
static const WSEGL_FunctionTable* gWrappedTable = nullptr;


WSEGLError IsDisplayValid(NativeDisplayType disp)
{
    fprintf(stderr, "DK_%s 0x%x\n", __FUNCTION__, disp);
    return gWrappedTable->pfnWSEGL_IsDisplayValid(disp);
}

WSEGLError InitialiseDisplay(NativeDisplayType nativeDisplay, WSEGLDisplayHandle* display, const WSEGLCaps** caps,
                             WSEGLConfig** config)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return gWrappedTable->pfnWSEGL_InitialiseDisplay(nativeDisplay, display, caps, config);
}

WSEGLError CloseDisplay(WSEGLDisplayHandle display)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return gWrappedTable->pfnWSEGL_CloseDisplay(display);
}

WSEGLError CreateWindowDrawable(WSEGLDisplayHandle display, WSEGLConfig * config,
                                WSEGLDrawableHandle* drawable, NativeWindowType window,
                                WSEGLRotationAngle* rotation)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return gWrappedTable->pfnWSEGL_CreateWindowDrawable(display, config, drawable, window, rotation);
}

WSEGLError CreatePixmapDrawable(WSEGLDisplayHandle, WSEGLConfig *, WSEGLDrawableHandle *, NativePixmapType, WSEGLRotationAngle *)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_CANNOT_INITIALISE;
}

WSEGLError DeleteDrawable(WSEGLDrawableHandle)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_SUCCESS;
}

WSEGLError SwapDrawable(WSEGLDrawableHandle window, unsigned long flags)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return gWrappedTable->pfnWSEGL_SwapDrawable(window, flags);
}

WSEGLError SwapControlInterval(WSEGLDrawableHandle, unsigned long)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_SUCCESS;
}

WSEGLError WaitNative(WSEGLDrawableHandle handle, unsigned long flags)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return gWrappedTable->pfnWSEGL_WaitNative(handle, flags);
}

WSEGLError CopyFromDrawable(WSEGLDrawableHandle, NativePixmapType)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_SUCCESS;
}

WSEGLError CopyFromPBuffer(void *, unsigned long, unsigned long, unsigned long, WSEGLPixelFormat, NativePixmapType)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return WSEGL_SUCCESS;
}

WSEGLError GetDrawableParameters(WSEGLDrawableHandle drawable, WSEGLDrawableParams* sourceParams,
                                 WSEGLDrawableParams* renderParams, unsigned long flags)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return gWrappedTable->pfnWSEGL_GetDrawableParameters(drawable, sourceParams, renderParams, flags);
}

WSEGLError ConnectDrawable(WSEGLDrawableHandle handle)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return gWrappedTable->pfnWSEGL_ConnectDrawable(handle);
}

WSEGLError DisconnectDrawable(WSEGLDrawableHandle handle)
{
    fprintf(stderr, "DK_%s\n", __FUNCTION__);
    return gWrappedTable->pfnWSEGL_DisconnectDrawable(handle);
}

WSEGLError FlagStartFrame(void)
{
    fprintf(stderr, "DK_%s=====================================================\n", __FUNCTION__);
    return gWrappedTable->pfnWSEGL_FlagStartFrame();
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

const WSEGL_FunctionTable* WSEGL_GetFunctionTablePointer(void)
{
    if (not gLib) {
        auto wsegl = getenv("DK_WSEGL");
        if (not wsegl) {
            wsegl = "/usr/lib/libpvrPVR2D_FRONTWSEGL.so";
        }
        gLib = dlopen(wsegl, RTLD_LAZY);
        _WSEGL_GetFunctionTablePointer =
                (const WSEGL_FunctionTable*(*)())dlsym(gLib, "WSEGL_GetFunctionTablePointer");
        gWrappedTable = _WSEGL_GetFunctionTablePointer();
    }
    return &gFunctionTable;
}
