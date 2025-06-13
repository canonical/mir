/*
 * Copyright © Canonical Ltd.
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
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/program_options/options_description.hpp>

#include "mir/options/program_option.h"
#include "mir/udev/wrapper.h"
#include "mir_test_framework/udev_environment.h"
#include "src/platforms/gbm-kms/server/kms/quirks.h"
#include "mir_test_framework/temporary_environment_value.h"

namespace mtf = mir_test_framework;
namespace mgg = mir::graphics::gbm;

class Quirks : public testing::Test
{
public:
    void SetUp() override
    {
        udev.add_standard_device("standard-drm-devices");
    }

    auto make_drm_device_enumerator() -> std::unique_ptr<mir::udev::Enumerator>
    {
        auto udev_ctx = std::make_shared<mir::udev::Context>();
        auto enumerator = std::make_unique<mir::udev::Enumerator>(udev_ctx);

        enumerator->match_subsystem("drm");
        enumerator->match_sysname("card[0-9]*");
        enumerator->scan_devices();

        return enumerator;
    }

    auto parsed_options_from_args(std::initializer_list<char const*> const& options) const
        -> std::unique_ptr<mir::options::ProgramOption>
    {
        auto parsed_options = std::make_unique<mir::options::ProgramOption>();

        auto argv = std::make_unique<char const*[]>(options.size() + 1);
        argv[0] = "argv0";
        std::copy(options.begin(), options.end(), &argv[1]);

        parsed_options->parse_arguments(description, static_cast<int>(options.size() + 1), argv.get());
        return parsed_options;
    }

    mtf::UdevEnvironment udev;
    boost::program_options::options_description description;

    // Need a clean environment for kms probe
    mtf::TemporaryEnvironmentValue mesa_disable_modeset_probe{"MIR_MESA_KMS_DISABLE_MODESET_PROBE", nullptr};
    mtf::TemporaryEnvironmentValue gbm_disable_modeset_probe{"MIR_GBM_KMS_DISABLE_MODESET_PROBE", nullptr};
};


TEST_F(Quirks, skip_devnode_quirk_fires)
{
    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({"--driver-quirks=skip:devnode:/dev/dri/card0"})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
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

TEST_F(Quirks, skip_driver_quirk_fires)
{
    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({"--driver-quirks=skip:driver:amdgpu"})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
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

TEST_F(Quirks, specifying_multiple_quirks_is_additive)
{
    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({"--driver-quirks=skip:driver:amdgpu", "--driver-quirks=skip:devnode:/dev/dri/card0"})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
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

TEST_F(Quirks, allowing_a_driver_overrides_previous_skip)
{
    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({"--driver-quirks=skip:driver:amdgpu", "--driver-quirks=allow:driver:amdgpu"})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
    {
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            EXPECT_FALSE(quirks.should_skip(device));
        }
        else if(device.devnode() && strstr(device.devnode(), "card1"))
        {
            seen_card[1] = true;
            // The quirk skipping the “amdgpu” driver, should be overridden by the "allow"
            EXPECT_FALSE(quirks.should_skip(device));
        }
    }

    ASSERT_TRUE(seen_card[0]);
    ASSERT_TRUE(seen_card[1]);
}

TEST_F(Quirks, skipping_a_driver_overrides_previous_allow)
{
    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({"--driver-quirks=allow:driver:amdgpu", "--driver-quirks=skip:driver:amdgpu"})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
    {
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            EXPECT_FALSE(quirks.should_skip(device));
        }
        else if(device.devnode() && strstr(device.devnode(), "card1"))
        {
            seen_card[1] = true;
            // The quirk allowing the “amdgpu” driver, should be overridden by the "skip"
            EXPECT_TRUE(quirks.should_skip(device));
        }
    }

    ASSERT_TRUE(seen_card[0]);
    ASSERT_TRUE(seen_card[1]);
}

TEST_F(Quirks, allowing_a_devnode_overrides_previous_skip)
{
    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({"--driver-quirks=skip:devnode:/dev/dri/card0", "--driver-quirks=allow:devnode:/dev/dri/card0"})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
    {
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            // The quirk skipping "/dev/dri/card0", should be overridden by the "allow"
            EXPECT_FALSE(quirks.should_skip(device));
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

TEST_F(Quirks, skipping_a_devnode_overrides_previous_allow)
{
    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({"--driver-quirks=allow:devnode:/dev/dri/card0", "--driver-quirks=skip:devnode:/dev/dri/card0"})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
    {
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            // The quirk allowing "/dev/dri/card0", should be overridden by the "skip"
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

TEST_F(Quirks, default_requires_modesetting_support)
{
    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({""})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
    {
        printf("Device: %s\n", device.devnode());
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            EXPECT_TRUE(quirks.require_modesetting_support(device));
        }
        else if(device.devnode() && strstr(device.devnode(), "card1"))
        {
            seen_card[1] = true;
            EXPECT_TRUE(quirks.require_modesetting_support(device));
        }
    }

    ASSERT_TRUE(seen_card[0]);
    ASSERT_TRUE(seen_card[1]);
}

TEST_F(Quirks, disable_kms_probe_selectively_disables_modesetting_support)
{
    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({"--driver-quirks=disable-kms-probe:driver:amdgpu"})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
    {
        printf("Device: %s\n", device.devnode());
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            EXPECT_TRUE(quirks.require_modesetting_support(device));
        }
        else if(device.devnode() && strstr(device.devnode(), "card1"))
        {
            seen_card[1] = true;
            EXPECT_FALSE(quirks.require_modesetting_support(device));
        }
    }

    ASSERT_TRUE(seen_card[0]);
    ASSERT_TRUE(seen_card[1]);
}

TEST_F(Quirks, mesa_disable_modeset_probe_globally_disables_modesetting_support)
{
    mtf::TemporaryEnvironmentValue mesa_disable_modeset_probe{"MIR_MESA_KMS_DISABLE_MODESET_PROBE", "1"};

    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({""})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
    {
        printf("Device: %s\n", device.devnode());
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            EXPECT_FALSE(quirks.require_modesetting_support(device));
        }
        else if(device.devnode() && strstr(device.devnode(), "card1"))
        {
            seen_card[1] = true;
            EXPECT_FALSE(quirks.require_modesetting_support(device));
        }
    }

    ASSERT_TRUE(seen_card[0]);
    ASSERT_TRUE(seen_card[1]);
}

TEST_F(Quirks, gbm_disable_modeset_probe_globally_disables_modesetting_support)
{
    mtf::TemporaryEnvironmentValue gbm_disable_modeset_probe{"MIR_GBM_KMS_DISABLE_MODESET_PROBE", "1"};

    mgg::Quirks::add_quirks_option(description);
    mgg::Quirks quirks{*parsed_options_from_args({""})};

    auto const enumerator = make_drm_device_enumerator();

    bool seen_card[] = {false, false};
    for (auto const& device : *enumerator)
    {
        printf("Device: %s\n", device.devnode());
        if (device.devnode() && strstr(device.devnode(), "card0"))
        {
            seen_card[0] = true;
            EXPECT_FALSE(quirks.require_modesetting_support(device));
        }
        else if(device.devnode() && strstr(device.devnode(), "card1"))
        {
            seen_card[1] = true;
            EXPECT_FALSE(quirks.require_modesetting_support(device));
        }
    }

    ASSERT_TRUE(seen_card[0]);
    ASSERT_TRUE(seen_card[1]);
}
