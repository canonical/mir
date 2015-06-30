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

#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"
#include "mir/geometry/displacement.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace mir::graphics;
using namespace mir::geometry;

namespace
{

class MockDisplayConfiguration : public DisplayConfiguration
{
public:
    MockDisplayConfiguration(MockDisplayConfiguration && m)
        : max_simultaneous_outputs{m.max_simultaneous_outputs},
        outputs{std::move(m.outputs)}
    {
    }

    MockDisplayConfiguration(size_t max_simultaneous_outputs, std::vector<DisplayConfigurationOutput> && config)
        : max_simultaneous_outputs{max_simultaneous_outputs},
        outputs{config}
    {
        if (max_simultaneous_outputs == max_simultaneous_outputs_all)
            max_simultaneous_outputs = outputs.size();
    }

    MockDisplayConfiguration(std::vector<DisplayConfigurationOutput> && config)
        : MockDisplayConfiguration(max_simultaneous_outputs_all, std::move(config))
    {
    }

    void for_each_card(std::function<void(DisplayConfigurationCard const&)> f) const
    {
        f({DisplayConfigurationCardId{1}, max_simultaneous_outputs});
    }

    void for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) const override
    {
        for (auto const& output : outputs)
            f(output);
    }

    void for_each_output(std::function<void(UserDisplayConfigurationOutput&)> f)
    {
        for (auto& output : outputs)
        {
            UserDisplayConfigurationOutput user(output);
            f(user);
        }
    }

    static const size_t max_simultaneous_outputs_all{std::numeric_limits<size_t>::max()};
private:
    size_t max_simultaneous_outputs;
    std::vector<DisplayConfigurationOutput> outputs;
};

}

DisplayConfigurationOutput default_output(DisplayConfigurationOutputId id)
{
    return { id, DisplayConfigurationCardId{1},
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
        mir_orientation_normal
    };
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
    output.current_mode_index = std::numeric_limits<size_t>::max();
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

MockDisplayConfiguration create_default_configuration(size_t max_outputs = MockDisplayConfiguration::max_simultaneous_outputs_all)
{
    return MockDisplayConfiguration
    {
        max_outputs,
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
    MockDisplayConfiguration conf{create_default_configuration()};

    policy.apply_to(conf);

    conf.for_each_output([&conf](DisplayConfigurationOutput const& output)
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
    MockDisplayConfiguration conf{create_default_configuration()};

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
    MockDisplayConfiguration conf{create_default_configuration()};

    conf.for_each_output([&conf](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_orientation_normal, output.orientation);
    });
}

TEST(CloneDisplayConfigurationPolicyTest, does_not_enable_more_outputs_than_supported)
{
    using namespace ::testing;

    size_t const max_simultaneous_outputs{1};
    CloneDisplayConfigurationPolicy policy;
    MockDisplayConfiguration conf{create_default_configuration(max_simultaneous_outputs)};

    policy.apply_to(conf);

    size_t used_count{0};
    conf.for_each_output([&used_count](DisplayConfigurationOutput const& output)
    {
        if (output.used)
            ++used_count;
    });

    EXPECT_GE(max_simultaneous_outputs, used_count);
}

TEST(CloneDisplayConfigurationPolicyTest, prefer_opaque_over_alpha)
{
    using namespace ::testing;

    CloneDisplayConfigurationPolicy policy;
    MockDisplayConfiguration pick_xrgb{ { connected_with_rgba_and_xrgb() } };

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
    MockDisplayConfiguration keep_bgr{ { connected_with_xrgb_bgr() } };

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
    MockDisplayConfiguration pick_rgba{ { default_output(DisplayConfigurationOutputId{15}) } };

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
    MockDisplayConfiguration conf{create_default_configuration()};

    policy.apply_to(conf);

    bool is_first{true};

    conf.for_each_output([&conf, &is_first](DisplayConfigurationOutput const& output)
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
    MockDisplayConfiguration conf{create_default_configuration()};

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
    MockDisplayConfiguration conf{create_default_configuration()};

    conf.for_each_output([&conf](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_orientation_normal, output.orientation);
    });
}

TEST(SingleDisplayConfigurationPolicyTest, does_not_enable_more_outputs_than_supported)
{
    using namespace ::testing;

    size_t const max_simultaneous_outputs{1};
    SingleDisplayConfigurationPolicy policy;
    MockDisplayConfiguration conf{create_default_configuration(max_simultaneous_outputs)};

    policy.apply_to(conf);

    size_t used_count{0};
    conf.for_each_output([&used_count](DisplayConfigurationOutput const& output)
    {
        if (output.used)
            ++used_count;
    });

    EXPECT_GE(max_simultaneous_outputs, used_count);
}

TEST(SideBySideDisplayConfigurationPolicyTest, uses_all_connected_valid_outputs)
{
    using namespace ::testing;

    SideBySideDisplayConfigurationPolicy policy;
    MockDisplayConfiguration conf{create_default_configuration()};

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

TEST(SideBySideDisplayConfigurationPolicyTest, default_policy_is_power_mode_on)
{
    using namespace ::testing;

    SideBySideDisplayConfigurationPolicy policy;
    MockDisplayConfiguration conf{create_default_configuration()};

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
    MockDisplayConfiguration conf{create_default_configuration()};

    conf.for_each_output([&conf](DisplayConfigurationOutput const& output)
    {
        EXPECT_EQ(mir_orientation_normal, output.orientation);
    });
}

TEST(SideBySideDisplayConfigurationPolicyTest, does_not_enable_more_outputs_than_supported)
{
    using namespace ::testing;

    size_t const max_simultaneous_outputs{1};
    SideBySideDisplayConfigurationPolicy policy;
    MockDisplayConfiguration conf{create_default_configuration(max_simultaneous_outputs)};

    policy.apply_to(conf);

    size_t used_count{0};
    conf.for_each_output([&used_count](DisplayConfigurationOutput const& output)
    {
        if (output.used)
            ++used_count;
    });

    EXPECT_GE(max_simultaneous_outputs, used_count);
}


