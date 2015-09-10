/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "src/platforms/mesa/server/x11/graphics/guest_platform.h"

#include "mir/test/doubles/mock_drm.h"
#include "mir/test/doubles/mock_gbm.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mgx = mir::graphics::X;
namespace mtd = mir::test::doubles;

namespace
{

class X11GuestPlatformTest : public ::testing::Test
{
public:
    void SetUp()
    {
        ::testing::Mock::VerifyAndClearExpectations(&mock_drm);
        ::testing::Mock::VerifyAndClearExpectations(&mock_gbm);
    }

    std::shared_ptr<mgx::GuestPlatform> create_guest_platform()
    {
        return std::make_shared<mgx::GuestPlatform>(nullptr);
    }

    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
};
}

TEST_F(X11GuestPlatformTest, failure_to_open_drm_results_in_an_error)
{
    using namespace ::testing;

    EXPECT_CALL(mock_drm, open(_,_,_))
        .WillRepeatedly(Return(-1));

    EXPECT_THROW({ create_guest_platform(); }, std::exception);
}

TEST_F(X11GuestPlatformTest, failure_to_create_gbm_device_results_in_an_error)
{
    using namespace ::testing;

    EXPECT_CALL(mock_gbm, gbm_create_device(mock_drm.fake_drm.fd()))
        .WillRepeatedly(Return(nullptr));

    EXPECT_THROW({ create_guest_platform(); }, std::exception);
}
