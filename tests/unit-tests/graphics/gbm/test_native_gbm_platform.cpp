/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/nested_context.h"
#include "src/server/graphics/gbm/native_gbm_platform.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_drm.h"
#include "mir_test_doubles/mock_gbm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

struct MockNestedContext : public mg::NestedContext
{
    MOCK_METHOD0(platform_fd_items, std::vector<int>());
    MOCK_METHOD1(drm_auth_magic, void(int));
};

class NativeGBMPlatformTest : public ::testing::Test
{
public:
    NativeGBMPlatformTest()
    {
        using namespace testing;

        ON_CALL(mock_nested_context, platform_fd_items())
            .WillByDefault(Return(std::vector<int>{mock_drm.fake_drm.fd()}));
    }

protected:
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
    ::testing::NiceMock<MockNestedContext> mock_nested_context;
};

}

TEST_F(NativeGBMPlatformTest, auth_magic_is_delegated_to_nested_context)
{
    using namespace testing;

    mgg::NativeGBMPlatform native;

    EXPECT_CALL(mock_nested_context, drm_auth_magic(_));

    native.initialize(mt::fake_shared(mock_nested_context));
    native.get_ipc_package();
}
