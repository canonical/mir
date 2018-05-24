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
#include "mir/test/doubles/stub_console_services.h"

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
protected:
    ::testing::NiceMock<mtd::MockDRM> mock_drm;
    mtf::UdevEnvironment fake_devices;
};

}

TEST_F(DRMHelperTest, closes_drm_fd_on_exec)
{   
    using namespace testing;

    fake_devices.add_standard_device("standard-drm-render-nodes");

    EXPECT_CALL(mock_drm, open(_, FlagSet(O_CLOEXEC), _));

    auto helper = mgm::helpers::DRMHelper::open_any_render_node(
        std::make_shared<mir::udev::Context>());
}   

TEST_F(DRMHelperTest, throws_if_drm_auth_magic_fails)
{
    using namespace testing;

    fake_devices.add_standard_device("standard-drm-devices");
    drm_magic_t const magic{0x10111213};

    EXPECT_CALL(
        mock_drm,
        drmAuthMagic(
            AnyOf(
                mtd::IsFdOfDevice("/dev/dri/card0"),
                mtd::IsFdOfDevice("/dev/dri/card1")),
            magic))
            .WillOnce(Return(-1));

    mtd::StubConsoleServices console;

    auto helpers = mgm::helpers::DRMHelper::open_all_devices(
        std::make_shared<mir::udev::Context>(),
        console);

    ASSERT_THAT(helpers, Not(IsEmpty()));

    EXPECT_THROW({
        helpers[0]->auth_magic(magic);
    }, std::runtime_error);
}
