#include "mockdrm.hpp"
#include <dk_utils/exceptions.hpp>

using namespace testing;

static MockDRM* gMock = nullptr;

MockDRM::MockDRM()
{
    if (gMock) {
        throw Exception<MockDRM>("gMock already created");
    }

    gMock = this;

    ON_CALL(*this, Ioctl(_, _, _)).WillByDefault(Return(0));
    ON_CALL(*this, GetMagic(_, _)).WillByDefault(Return(0));
    ON_CALL(*this, PrimeHandleToFD(_, _, _, _)).WillByDefault(Return(0));
}

MockDRM::~MockDRM()
{
    gMock = nullptr;
}

extern "C" {

int drmIoctl(int fd, unsigned long request, void *arg)
{
    return gMock->Ioctl(fd, request, arg);
}

int drmGetMagic(int fd, int* magic)
{
    return gMock->GetMagic(fd, magic);
}

int drmPrimeHandleToFD(int fd, uint32_t handle, uint32_t flags, int *prime_fd)
{
    return gMock->PrimeHandleToFD(fd, handle, flags, prime_fd);
}

}
