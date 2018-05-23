/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#include "src/platforms/mesa/server/display_helpers.h"
#include "mir/udev/wrapper.h"

#include "mir_test_framework/udev_environment.h"
#include "mir/test/doubles/mock_drm.h"

#include <fcntl.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mgm = mir::graphics::mesa;
namespace mtf = mir_test_framework;
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
    mgm::helpers::DRMHelper drm_helper{mgm::helpers::DRMNodeToUse::card};
};

}

TEST_F(DRMHelperTest, closes_drm_fd_on_exec)
{   
    using namespace testing;

    EXPECT_CALL(mock_drm, open(_, FlagSet(O_CLOEXEC), _));

    drm_helper.setup(std::make_shared<mir::udev::Context>());
}   

TEST_F(DRMHelperTest, throws_if_drm_auth_magic_fails)
{
    using namespace testing;

    drm_magic_t const magic{0x10111213};

    EXPECT_CALL(
        mock_drm,
        drmAuthMagic(
            AnyOf(
                mtd::IsFdOfDevice("/dev/dri/card0"),
                mtd::IsFdOfDevice("/dev/dri/card1")),
            magic))
            .WillOnce(Return(-1));

    drm_helper.setup(std::make_shared<mir::udev::Context>());

    EXPECT_THROW({
        drm_helper.auth_magic(magic);
    }, std::runtime_error);
}
