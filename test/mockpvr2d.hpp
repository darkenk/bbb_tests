#ifndef MOCKPVR2D_HPP
#define MOCKPVR2D_HPP

#include <gmock/gmock.h>
#include <pvr2d/pvr2d.h>

class FakePVR2D;

class MockPVR2D
{
public:
    MockPVR2D();
    ~MockPVR2D();

    MOCK_METHOD1(EnumerateDevices, PVR2D_INT(PVR2DDEVICEINFO*));
    MOCK_METHOD3(CreateDeviceContext, PVR2DERROR(PVR2D_ULONG, PVR2DCONTEXTHANDLE*, PVR2D_ULONG));
    MOCK_METHOD1(DestroyDeviceContext, PVR2DERROR(PVR2DCONTEXTHANDLE));
    MOCK_METHOD6(MemWrap, PVR2DERROR(PVR2DCONTEXTHANDLE, PVR2D_VOID*, PVR2D_ULONG, PVR2D_ULONG,
                                     PVR2D_ULONG[], PVR2DMEMINFO**));
    MOCK_METHOD2(MemFree, PVR2DERROR(PVR2DCONTEXTHANDLE, PVR2DMEMINFO*));

private:
    FakePVR2D* fake;
};

#endif // MOCKPVR2D_HPP
