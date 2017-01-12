#include "mockkms.hpp"
#include <dk_utils/exceptions.hpp>
#include <libkms/libkms.h>
#include <map>

using namespace testing;
using namespace std;

struct kms_bo
{
    kms_bo(unsigned w, unsigned h, unsigned p): width(w), height(h), pitch(p) {}
    unsigned width;
    unsigned height;
    unsigned pitch;
};

class FakeKms
{
public:
    int bo_create(struct kms_driver*, const unsigned *attr, struct kms_bo** buff) {
        unsigned w = 0;
        unsigned h = 0;
        unsigned p = 0;
        while(*attr != KMS_TERMINATE_PROP_LIST) {
            switch(*attr) {
            case KMS_WIDTH: w = *(++attr); break;
            case KMS_HEIGHT: h = *(++attr); break;
            default:
                ++attr;
            }
        }
        p = w * 4;
        *buff = new kms_bo(w, h, p);
        return 0;
    }

    int bo_get_prop(struct kms_bo* buffer, unsigned prop, unsigned* out) {
        switch(prop) {
        case KMS_WIDTH: *out = buffer->width; break;
        case KMS_HEIGHT: *out = buffer->height; break;
        case KMS_PITCH: *out = buffer->pitch; break;
        case KMS_HANDLE: *out = 0xdaafdaaf; break;
        default:
            return -1;
        }

        return 0;
    }

    int bo_destroy(struct kms_bo** buffer) {
        delete *buffer;
        return 0;
    }

private:

};

static MockKms* gMock = nullptr;

MockKms::MockKms()
{
    if (gMock) {
        throw Exception<MockKms>("gMock already created");
    }

    gMock = this;

    ON_CALL(*this, create(_, _)).WillByDefault(
                DoAll(SetArgPointee<1>(reinterpret_cast<kms_driver*>(0xdaafdaaf)), Return(0)));
    ON_CALL(*this, destroy(_)).WillByDefault(Return(0));
    ON_CALL(*this, bo_create(_, _, _)).WillByDefault(Invoke(fake, &FakeKms::bo_create));
    ON_CALL(*this, bo_get_prop(_, _, _)).WillByDefault(Invoke(fake, &FakeKms::bo_get_prop));
    ON_CALL(*this, bo_map(_, _)).WillByDefault(
                DoAll(SetArgPointee<1>(reinterpret_cast<void*>(0xdaafdaaf)), Return(0)));
    ON_CALL(*this, bo_unmap(_)).WillByDefault(Return(0));
    ON_CALL(*this, bo_destroy(_)).WillByDefault(Invoke(fake, &FakeKms::bo_destroy));
    fake = new FakeKms;
}

MockKms::~MockKms()
{
    delete fake;
    gMock = nullptr;
}

extern "C" {

int kms_create(int fd, struct kms_driver **out)
{
    return gMock->create(fd, out);
}


int kms_destroy(struct kms_driver **kms)
{
    return gMock->destroy(kms);
}

int kms_bo_create(struct kms_driver *kms, const unsigned *attr, struct kms_bo **out)
{
    return gMock->bo_create(kms, attr, out);
}

int kms_bo_get_prop(struct kms_bo *bo, unsigned key, unsigned *out)
{
    return gMock->bo_get_prop(bo, key, out);
}

int kms_bo_map(struct kms_bo *bo, void **out)
{
    return gMock->bo_map(bo, out);
}

int kms_bo_unmap(struct kms_bo *bo)
{
    return gMock->bo_unmap(bo);
}

int kms_bo_destroy(struct kms_bo **bo)
{
    return gMock->bo_destroy(bo);
}

}
