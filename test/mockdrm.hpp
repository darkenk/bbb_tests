#ifndef MOCKDRM_HPP
#define MOCKDRM_HPP

#include <gmock/gmock.h>
#include <pvr2d/pvr2d.h>

class MockDRM
{
public:
    MockDRM();
    ~MockDRM();

    MOCK_METHOD3(Ioctl, int(int, unsigned long, void*));
    MOCK_METHOD2(GetMagic, int(int, int*));
    MOCK_METHOD4(PrimeHandleToFD, int(int, uint32_t, uint32_t, int*));

};

#endif // MOCKDRM_HPP
