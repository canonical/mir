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

#include "src/platforms/mesa/server/x11/platform.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mg = mir::graphics;

namespace
{
class X11GraphicsPlatform : public ::testing::Test
{
public:
    void SetUp()
    {
//        ::testing::Mock::VerifyAndClearExpectations(&mock_drm);
//        ::testing::Mock::VerifyAndClearExpectations(&mock_gbm);
//        fake_devices.add_standard_device("standard-drm-devices");
    }

    std::shared_ptr<mg::Platform> create_platform()
    {
//        return mtd::create_platform_with_null_dependencies();
    	return nullptr;
    }

//    ::testing::NiceMock<mtd::MockDRM> mock_drm;
//    ::testing::NiceMock<mtd::MockGBM> mock_gbm;
//    mtf::UdevEnvironment fake_devices;
};
}

TEST_F(X11GraphicsPlatform, a_failure_while_creating_a_platform_results_in_an_error)
{
    using namespace ::testing;

//    EXPECT_CALL(mock_drm, open(_,_,_))
//            .WillRepeatedly(Return(-1));

    try
    {
//        auto platform = create_platform();
    } catch(...)
    {
        return;
    }

//    FAIL() << "Expected an exception to be thrown.";
}

