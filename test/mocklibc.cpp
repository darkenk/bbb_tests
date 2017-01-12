#include "mocklibc.hpp"
#include <dk_utils/exceptions.hpp>

using namespace testing;

static MockLibc* gMock = nullptr;

MockLibc::MockLibc()
{
    if (gMock) {
        throw Exception<MockLibc>("gMock already created");
    }

    gMock = this;

    ON_CALL(*this, mmap(_, _, _, _, _, _)).WillByDefault(Return((void*)0xdaafdaaf));
    ON_CALL(*this, munmap(_, _)).WillByDefault(Return(0));
}

MockLibc::~MockLibc()
{
    gMock = nullptr;
}

extern "C" {

void *mmap (void *__addr, size_t __len, int __prot, int __flags, int __fd, __off_t __offset)
{
    return gMock->mmap(__addr, __len, __prot, __flags, __fd, __offset);
}

int munmap (void *__addr, size_t __len)
{
    return gMock->munmap(__addr, __len);
}
}
