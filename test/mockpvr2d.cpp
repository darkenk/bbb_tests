#include "mockpvr2d.hpp"
#include <dk_utils/exceptions.hpp>

using namespace testing;

static MockPVR2D* gMock = nullptr;

class FakePVR2D
{
public:
    PVR2DERROR MemWrap(PVR2DCONTEXTHANDLE hContext, PVR2D_VOID *pMem, PVR2D_ULONG ulFlags,
                            PVR2D_ULONG ulBytes, PVR2D_ULONG alPageAddress[],
                            PVR2DMEMINFO **ppsMemInfo) {
        auto mem = new PVR2DMEMINFO();
        memset(mem, 0, sizeof(*mem));
        mem->ui32MemSize = ulBytes;
        mem->ulFlags = ulFlags;
        *ppsMemInfo = mem;
        return PVR2D_OK;
    }

    PVR2DERROR MemFree(PVR2DCONTEXTHANDLE hContext, PVR2DMEMINFO *psMemInfo) {
        delete psMemInfo;
        return PVR2D_OK;
    }


};

MockPVR2D::MockPVR2D()
{
    if (gMock) {
        throw Exception<MockPVR2D>("gMock already created");
    }
    gMock = this;

    ON_CALL(*this, EnumerateDevices(_)).WillByDefault(Return(0));
    ON_CALL(*this, EnumerateDevices(Eq(nullptr))).WillByDefault(Return(1));

    ON_CALL(*this, CreateDeviceContext(_, _, _)).WillByDefault(Return(PVR2D_OK));
    ON_CALL(*this, DestroyDeviceContext(_)).WillByDefault(Return(PVR2D_OK));

    ON_CALL(*this, MemWrap(_, _, _, _, _, _)).WillByDefault(Invoke(fake, &FakePVR2D::MemWrap));

    ON_CALL(*this, MemFree(_, _)).WillByDefault(Invoke(fake, &FakePVR2D::MemFree));

    fake = new FakePVR2D();
}

MockPVR2D::~MockPVR2D()
{
    gMock = nullptr;
}

PVR2D_INT PVR2DEnumerateDevices(PVR2DDEVICEINFO *pDevInfo)
{
    return gMock->EnumerateDevices(pDevInfo);
}

PVR2DERROR PVR2DCreateDeviceContext(PVR2D_ULONG ulDevID, PVR2DCONTEXTHANDLE* phContext,
                                    PVR2D_ULONG ulFlags)
{
    return gMock->CreateDeviceContext(ulDevID, phContext, ulFlags);
}

PVR2DERROR PVR2DDestroyDeviceContext(PVR2DCONTEXTHANDLE hContext)
{
    return gMock->DestroyDeviceContext(hContext);
}

PVR2DERROR PVR2DMemWrap(PVR2DCONTEXTHANDLE hContext, PVR2D_VOID *pMem, PVR2D_ULONG ulFlags,
                        PVR2D_ULONG ulBytes, PVR2D_ULONG alPageAddress[],
                        PVR2DMEMINFO **ppsMemInfo)
{
    return gMock->MemWrap(hContext, pMem, ulFlags, ulBytes, alPageAddress, ppsMemInfo);
}

PVR2DERROR PVR2DMemFree(PVR2DCONTEXTHANDLE hContext, PVR2DMEMINFO *psMemInfo)
{
    return gMock->MemFree(hContext, psMemInfo);
}

