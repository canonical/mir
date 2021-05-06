/*
 * Copyright © 2021 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/program_options/options_description.hpp>

#include "mir/options/program_option.h"
#include "mir/udev/wrapper.h"
#include "mir_test_framework/udev_environment.h"
#include "src/platforms/gbm-kms/server/kms/quirks.h"

namespace mtf = mir_test_framework;
namespace mgg = mir::graphics::gbm;


TEST(Quirks, skip_devnode_quirk_fires)
{
    mtf::UdevEnvironment udev;
    udev.add_standard_device("standard-drm-devices");

    boost::program_options::options_description description;
    mir::options::ProgramOption options;

    mgg::Quirks::add_quirks_option(description);

    char const* arguments[] = {"argv0", "--driver-quirks=skip:devnode:/dev/dri/card0"};
    options.parse_arguments(description, 2, arguments);

    mgg::Quirks quirks{options};

    auto udev_ctx = std::make_shared<mir::udev::Context>();
    mir::udev::Enumerator enumerator{udev_ctx};

    enumerator.match_subsystem("drm");
    enumerator.match_sysname("card[0-9]*");
    enumerator.scan_devices();

    bool seen_card[] = {false, false};
    for (auto const& device : enumerator)
    {
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            EXPECT_TRUE(quirks.should_skip(device));
        }
        else if(device.devnode() && strstr(device.devnode(), "card1"))
        {
            seen_card[1] = true;
            EXPECT_FALSE(quirks.should_skip(device));
        }
    }

    ASSERT_TRUE(seen_card[0]);
    ASSERT_TRUE(seen_card[1]);
}

TEST(Quirks, skip_driver_quirk_fires)
{
    mtf::UdevEnvironment udev;
    udev.add_standard_device("standard-drm-devices");

    boost::program_options::options_description description;
    mir::options::ProgramOption options;

    mgg::Quirks::add_quirks_option(description);

    char const* arguments[] = {"argv0", "--driver-quirks=skip:driver:amdgpu"};
    options.parse_arguments(description, 2, arguments);

    mgg::Quirks quirks{options};

    auto udev_ctx = std::make_shared<mir::udev::Context>();
    mir::udev::Enumerator enumerator{udev_ctx};

    enumerator.match_subsystem("drm");
    enumerator.match_sysname("card[0-9]*");
    enumerator.scan_devices();

    bool seen_card[] = {false, false};
    for (auto const& device : enumerator)
    {
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            EXPECT_FALSE(quirks.should_skip(device));
        }
        else if(device.devnode() && strstr(device.devnode(), "card1"))
        {
            seen_card[1] = true;
            EXPECT_TRUE(quirks.should_skip(device));
        }
    }

    ASSERT_TRUE(seen_card[0]);
    ASSERT_TRUE(seen_card[1]);
}

TEST(Quirks, specifying_multiple_quirks_is_additive)
{
    mtf::UdevEnvironment udev;
    udev.add_standard_device("standard-drm-devices");

    boost::program_options::options_description description;
    mir::options::ProgramOption options;

    mgg::Quirks::add_quirks_option(description);

    char const* arguments[] =
        {"argv0", "--driver-quirks=skip:driver:amdgpu", "--driver-quirks=skip:devnode:/dev/dri/card0"};
    options.parse_arguments(description, 3, arguments);

    mgg::Quirks quirks{options};

    auto udev_ctx = std::make_shared<mir::udev::Context>();
    mir::udev::Enumerator enumerator{udev_ctx};

    enumerator.match_subsystem("drm");
    enumerator.match_sysname("card[0-9]*");
    enumerator.scan_devices();

    bool seen_card[] = {false, false};
    for (auto const& device : enumerator)
    {
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            // We have a quirk skipping /dev/dri/card0
            EXPECT_TRUE(quirks.should_skip(device));
        }
        else if(device.devnode() && strstr(device.devnode(), "card1"))
        {
            seen_card[1] = true;
            // We have a quirk skipping the “amdgpu” driver, which is the driver used for /dev/dri/card1
            EXPECT_TRUE(quirks.should_skip(device));
        }
    }

    ASSERT_TRUE(seen_card[0]);
    ASSERT_TRUE(seen_card[1]);
}

