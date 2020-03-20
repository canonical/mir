/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"
#include "mir/geometry/displacement.h"

#include "mir/test/doubles/stub_display_configuration.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace mir::graphics;
using namespace mir::geometry;

namespace
{

class StubDisplayConfiguration : public mir::test::doubles::StubDisplayConfig
{
public:
    using mir::test::doubles::StubDisplayConfig::StubDisplayConfig;

    StubDisplayConfiguration(
        std::vector<DisplayConfigurationOutput> const& outputs)
        : StubDisplayConfig{outputs}
    {
    }
};

}

DisplayConfigurationOutput default_output(DisplayConfigurationOutputId id)
{
    // We name the return value just to work around an apparent clang bug/quirk
    DisplayConfigurationOutput ret{ id, DisplayConfigurationCardId{1},
        DisplayConfigurationOutputType::vga,
        {mir_pixel_format_abgr_8888},
        { {Size{523, 555}, 60.0} },
        0,
        Size{324, 642},
        true,
        false,
        Point{X{123}, Y{343}},
        0,
        mir_pixel_format_abgr_8888,
        mir_power_mode_on,
        mir_orientation_normal,
        1.0f,
        mir_form_factor_monitor,
        mir_subpixel_arrangement_unknown,
        {},
        mir_output_gamma_unsupported,
        {},
        {}
    };
    return ret;
}

DisplayConfigurationOutput connected_with_modes()
{
    DisplayConfigurationOutput output = default_output(DisplayConfigurationOutputId{10}) ;
    output.modes =
    {
        {Size{123, 111}, 59.9},
        {Size{123, 111}, 59.9},
        {Size{123, 111}, 59.9}
    };
    output.preferred_mode_index = 2;
    output.current_mode_index = 1;
    return output;
}
DisplayConfigurationOutput connected_without_modes()
{
    DisplayConfigurationOutput output = default_output(DisplayConfigurationOutputId{11});
    output.pixel_formats = {};
    output.modes = {};
    output.current_format = mir_pixel_format_invalid;
    output.current_mode_index = std::numeric_limits<uint32_t>::max();
    return output;
}

DisplayConfigurationOutput connected_with_single_mode()
{
    return default_output(DisplayConfigurationOutputId{12});
}

DisplayConfigurationOutput not_connected()
{
    DisplayConfigurationOutput output = default_output(DisplayConfigurationOutputId{13});
    output.connected = false;
    output.current_mode_index = 1;
    return output;
}

DisplayConfigurationOutput connected_with_rgba_and_xrgb()
{
    DisplayConfigurationOutput output = default_output(DisplayConfigurationOutputId{14});
    output.pixel_formats = {mir_pixel_format_argb_8888, mir_pixel_format_xrgb_8888};
    return output;
}

DisplayConfigurationOutput connected_with_xrgb_bgr()
{
    DisplayConfigurationOutput output = default_output(DisplayConfigurationOutputId{15});
    output.pixel_formats = {mir_pixel_format_xrgb_8888, mir_pixel_format_bgr_888};
    output.current_format = mir_pixel_format_bgr_888;
    return output;
}

StubDisplayConfiguration create_default_configuration()
{
    return StubDisplayConfiguration
    {
        {
            connected_with_modes(),
            connected_without_modes(),
            connected_with_single_mode(),
            not_connected(),
        }
    };
}

TEST(CloneDisplayConfigurationPolicyTest, uses_all_connected_valid_outputs)
{
    using namespace ::testing;

    CloneDisplayConfigurationPolicy policy;
    StubDisplayConfiguration conf{create_default_configuration()};

    policy.apply_to(conf);

    conf.for_each_output([](DisplayConfigurationOutput const& output)
    {
        if (output.connected && output.modes.size() > 0)
        {
            EXPECT_TRUE(output.used);
            EXPECT_EQ(Point(), output.top_left);
            EXPECT_EQ(output.preferred_mode_index, output.current_mode_index);
        }
        else
        {
            EXPECT_FALSE(output.used);
        }
    });
}

TEST(CloneDisplayConfigurationPolicyTest, default_policy_is_power_mode_on)
{
    using namespace ::testing;

    CloneDisplayConfigurationPolicy policy;
    StubDisplayConfiguration conf{create_default_configuration()};

    policy.apply_to(conf);

    conf.for_each_output([](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_power_mode_on, output.power_mode);
    });
}

TEST(CloneDisplayConfigurationPolicyTest, default_orientation_is_normal)
{
    using namespace ::testing;

    CloneDisplayConfigurationPolicy policy;
    StubDisplayConfiguration conf{create_default_configuration()};

    conf.for_each_output([](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_orientation_normal, output.orientation);
    });
}

TEST(CloneDisplayConfigurationPolicyTest, prefer_opaque_over_alpha)
{
    using namespace ::testing;

    CloneDisplayConfigurationPolicy policy;
    StubDisplayConfiguration pick_xrgb{ { connected_with_rgba_and_xrgb() } };

    policy.apply_to(pick_xrgb);

    pick_xrgb.for_each_output([](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_pixel_format_xrgb_8888, output.current_format);
    });
}

TEST(CloneDisplayConfigurationPolicyTest, preserve_opaque_selection)
{
    using namespace ::testing;

    CloneDisplayConfigurationPolicy policy;
    StubDisplayConfiguration keep_bgr{ { connected_with_xrgb_bgr() } };

    policy.apply_to(keep_bgr);

    keep_bgr.for_each_output([](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_pixel_format_bgr_888, output.current_format);
    });
}

TEST(CloneDisplayConfigurationPolicyTest, accept_transparency_when_only_option)
{
    using namespace ::testing;

    CloneDisplayConfigurationPolicy policy;
    StubDisplayConfiguration pick_rgba{ { default_output(DisplayConfigurationOutputId{15}) } };

    policy.apply_to(pick_rgba);

    pick_rgba.for_each_output([](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_pixel_format_abgr_8888, output.current_format);
    });
}

TEST(SingleDisplayConfigurationPolicyTest, uses_first_of_connected_valid_outputs)
{
    using namespace ::testing;

    SingleDisplayConfigurationPolicy policy;
    StubDisplayConfiguration conf{create_default_configuration()};

    policy.apply_to(conf);

    bool is_first{true};

    conf.for_each_output([&is_first](DisplayConfigurationOutput const& output)
    {
        if (output.connected && output.modes.size() > 0 && is_first)
        {
            EXPECT_TRUE(output.used);
            EXPECT_EQ(Point(), output.top_left);
            EXPECT_EQ(output.preferred_mode_index, output.current_mode_index);
            is_first = false;
        }
        else
        {
            EXPECT_FALSE(output.used);
        }
    });
}

TEST(SingleDisplayConfigurationPolicyTest, default_policy_is_power_mode_on)
{
    using namespace ::testing;

    SingleDisplayConfigurationPolicy policy;
    StubDisplayConfiguration conf{create_default_configuration()};

    policy.apply_to(conf);

    bool is_first{true};

    conf.for_each_output([&is_first](DisplayConfigurationOutput const& output)
    {
         if (output.connected && output.modes.size() > 0 && is_first)
         {
             EXPECT_EQ(mir_power_mode_on, output.power_mode);
             is_first = false;
         }
    });
}

TEST(SingleDisplayConfigurationPolicyTest, default_orientation_is_normal)
{
    using namespace ::testing;

    SingleDisplayConfigurationPolicy policy;
    auto conf = create_default_configuration();
    //StubDisplayConfiguration conf{create_default_configuration()};

    conf.for_each_output([](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_orientation_normal, output.orientation);
    });
}

TEST(SideBySideDisplayConfigurationPolicyTest, uses_all_connected_valid_outputs)
{
    using namespace ::testing;

    SideBySideDisplayConfigurationPolicy policy;
    StubDisplayConfiguration conf{create_default_configuration()};

    policy.apply_to(conf);

    Point expected_position;

    conf.for_each_output([&](DisplayConfigurationOutput const& output)
    {
        if (output.connected && output.modes.size() > 0)
        {
            EXPECT_TRUE(output.used);
            EXPECT_EQ(expected_position, output.top_left);
            EXPECT_EQ(output.preferred_mode_index, output.current_mode_index);

            expected_position += as_displacement(output.extents().size).dx;
        }
        else
        {
            EXPECT_FALSE(output.used);
        }
    });
}

TEST(SideBySideDisplayConfigurationPolicyTest, placement_respects_scale)
{
    using namespace ::testing;

    SideBySideDisplayConfigurationPolicy policy;
    StubDisplayConfiguration conf{create_default_configuration()};

    conf.for_each_output([&](UserDisplayConfigurationOutput const& output)
    {
        output.scale = 2.0f;
    });

    policy.apply_to(conf);

    Point expected_position;

    conf.for_each_output([&](DisplayConfigurationOutput const& output)
    {
        if (output.connected && output.modes.size() > 0)
        {
            EXPECT_EQ(expected_position, output.top_left);

            expected_position.x += DeltaX{round(output.modes[output.current_mode_index].size.width.as_int()/2.0f)};
        }
        else
        {
            EXPECT_FALSE(output.used);
        }
    });
}

TEST(SideBySideDisplayConfigurationPolicyTest, default_policy_is_power_mode_on)
{
    using namespace ::testing;

    SideBySideDisplayConfigurationPolicy policy;
    StubDisplayConfiguration conf{create_default_configuration()};

    policy.apply_to(conf);

    conf.for_each_output([](DisplayConfigurationOutput const& output)
    {
        if (output.connected && output.modes.size() > 0)
        {
            EXPECT_EQ(mir_power_mode_on, output.power_mode);
        }
    });
}

TEST(SideBySideDisplayConfigurationPolicyTest, default_orientation_is_normal)
{
    using namespace ::testing;

    SideBySideDisplayConfigurationPolicy policy;
    StubDisplayConfiguration conf{create_default_configuration()};

    conf.for_each_output([](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_orientation_normal, output.orientation);
    });
}
