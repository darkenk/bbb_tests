#include "drawable.hpp"
#include <gtest/gtest.h>

using namespace testing;

class FakeContext {};

class WSEGLDrawable : public Test {
public:
    virtual void SetUp() {
        ctx = new FakeContext();
        drawable = new WSEGL::Drawable(ctx);
    }

    virtual void TearDown() {
        delete drawable;
        delete ctx;
    }

protected:
    WSEGL::Drawable* drawable;
    FakeContext* ctx;

};

TEST_F(WSEGLDrawable, is_it_compile) {
}
