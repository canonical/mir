/*
 * Copyright © 2014 Canonical Ltd.
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

#include "src/platform/graphics/mesa/display_helpers.h"
#include "mir/udev/wrapper.h"

#include "mir_test_framework/udev_environment.h"
#include "mir_test_doubles/mock_drm.h"

#include <fcntl.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mgm = mir::graphics::mesa;
namespace mtf = mir::mir_test_framework;
namespace mtd = mir::test::doubles;
                          
namespace
{

MATCHER_P(FlagSet, flag, "")
{
    return (arg & flag);
}

class DRMHelperTest : public ::testing::Test
{              
public:        
    DRMHelperTest()
    {          
        fake_devices.add_standard_device("standard-drm-devices");
    }          
               
protected:       
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    mtf::UdevEnvironment fake_devices;
};

}

TEST_F(DRMHelperTest, closes_drm_fd_on_exec)
{   
    using namespace testing;

    EXPECT_CALL(mock_drm, open(_, FlagSet(O_CLOEXEC), _));

    mgm::helpers::DRMHelper drm_helper;
    drm_helper.setup(std::make_shared<mir::udev::Context>());
}   
