
#include "mir/compositor/buffer_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;

TEST(surface_token, token_test) {
    using namespace testing;

    mc::SurfaceToken token0;
    EXPECT_FALSE(token0.is_valid());

    mc::SurfaceToken token1(0x44);
    EXPECT_TRUE(token1.is_valid());

}
