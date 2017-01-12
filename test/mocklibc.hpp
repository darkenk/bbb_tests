#ifndef MOCKLIBC_HPP
#define MOCKLIBC_HPP

#include <gmock/gmock.h>

class MockLibc
{
public:
    MockLibc();
    ~MockLibc();
    MOCK_METHOD6(mmap, void *(void *, size_t, int, int, int, __off_t));
    MOCK_METHOD2(munmap, int(void*, size_t));
};

#endif // MOCKLIBC_HPP
