#ifndef MOCKKMS_HPP
#define MOCKKMS_HPP

#include <gmock/gmock.h>
#include <pvr2d/pvr2d.h>

class FakeKms;

class MockKms
{
public:
    MockKms();
    ~MockKms();

    MOCK_METHOD2(create, int(int, struct kms_driver**));
    MOCK_METHOD1(destroy, int(struct kms_driver**));
    MOCK_METHOD3(bo_create, int(struct kms_driver*, const unsigned *, struct kms_bo**));
    MOCK_METHOD3(bo_get_prop, int(struct kms_bo*, unsigned, unsigned*));
    MOCK_METHOD2(bo_map, int(struct kms_bo*, void**));
    MOCK_METHOD1(bo_unmap, int(struct kms_bo*));
    MOCK_METHOD1(bo_destroy, int(struct kms_bo**));

private:
    FakeKms* fake;
};

#endif // MOCKKMS_HPP
