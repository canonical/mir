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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "src/server/graphics/nested/nested_display_configuration.h"
#include "mir_display_configuration_builder.h"
#include "mir_toolkit/mir_display_configuration.h"

#include "src/client/display_configuration.h"

#include "mir/test/display_config_matchers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;
namespace mt = mir::test;
using namespace testing;

namespace
{

struct MockCardVisitor
{
    MOCK_CONST_METHOD1(f, void(mg::DisplayConfigurationCard const&));
};

struct MockOutputVisitor
{
    MOCK_CONST_METHOD1(f, void(mg::DisplayConfigurationOutput const&));
};

}

TEST(NestedDisplayConfiguration, empty_configuration_is_read_correctly)
{
    auto conf = mir::protobuf::DisplayConfiguration{};
    auto empty_configuration =
        std::shared_ptr<MirDisplayConfig>(
            new MirDisplayConfig{conf});

    mgn::NestedDisplayConfiguration config(empty_configuration);

    config.for_each_output([](mg::DisplayConfigurationOutput const&) { FAIL(); });
}

TEST(NestedDisplayConfiguration, trivial_configuration_has_one_output)
{
    mgn::NestedDisplayConfiguration config(mt::build_trivial_configuration());

    MockOutputVisitor ov;
    EXPECT_CALL(ov, f(_)).Times(Exactly(1));

    config.for_each_output([&ov](mg::DisplayConfigurationOutput const& output) { ov.f(output); });
}

TEST(NestedDisplayConfiguration, trivial_configuration_can_be_configured)
{
    auto const mir_config = mt::build_trivial_configuration();
    auto const output = mir_display_config_get_output(mir_config.get(), 0);
    auto const default_current_output_format = mir_output_get_current_pixel_format(output);
    geom::Point const new_top_left{10,20};
    mgn::NestedDisplayConfiguration config(mir_config);

    config.for_each_output([&](mg::UserDisplayConfigurationOutput& output)
        {
            output.used = true;
            output.top_left = new_top_left;
        });

    MockOutputVisitor ov;
    EXPECT_CALL(ov, f(_)).Times(Exactly(1));

    config.for_each_output([&](mg::DisplayConfigurationOutput const& output)
        {
            ov.f(output);
            EXPECT_EQ(true, output.used);
            EXPECT_EQ(new_top_left, output.top_left);
            EXPECT_EQ(0u, output.current_mode_index);
            EXPECT_EQ(default_current_output_format, output.current_format);
        });
}

// Validation tests once stood here. They've now been replaced by more
// portable validation logic which can be found in:
// TEST(DisplayConfiguration, ...

TEST(NestedDisplayConfiguration, non_trivial_configuration_has_three_outputs)
{
    mgn::NestedDisplayConfiguration config(mt::build_non_trivial_configuration());

    MockOutputVisitor ov;
    EXPECT_CALL(ov, f(_)).Times(Exactly(3));

    config.for_each_output([&ov](mg::DisplayConfigurationOutput const& output) { ov.f(output); });
}

TEST(NestedDisplayConfiguration, non_trivial_configuration_can_be_configured)
{
    auto const mir_config = mt::build_non_trivial_configuration();
    auto const output = mir_display_config_get_output(mir_config.get(), 1);
    mg::DisplayConfigurationOutputId const id(mir_output_get_id(output));
    geom::Point const top_left{100,200};
    mgn::NestedDisplayConfiguration config(mir_config);

    config.for_each_output([&](mg::UserDisplayConfigurationOutput& output)
        {
            if (output.id == id)
            {
                output.used = true;
                output.top_left = top_left;
                output.current_mode_index = 1;
                output.current_format = mir_pixel_format_argb_8888;
            }
        });

    MockOutputVisitor ov;
    EXPECT_CALL(ov, f(_)).Times(Exactly(3));
    config.for_each_output([&](mg::DisplayConfigurationOutput const& output)
        {
            ov.f(output);
            if (output.id == id)
            {
                EXPECT_EQ(true, output.used);
                EXPECT_EQ(top_left, output.top_left);
                EXPECT_EQ(1u, output.current_mode_index);
                EXPECT_EQ(mir_pixel_format_argb_8888, output.current_format);
            }
        });
}

TEST(NestedDisplayConfiguration, includes_host_edid)
{
    auto host_conf = mt::build_trivial_configuration();
    auto const output = mir_display_config_get_output(host_conf.get(), 0);
    auto edid_start = mir_output_get_edid(output);
    auto edid_size = mir_output_get_edid_size(output);

    ASSERT_NE(nullptr, edid_start);
    ASSERT_NE(0u, edid_size);

    std::vector<uint8_t> host_edid(edid_start, edid_start+edid_size);

    mgn::NestedDisplayConfiguration nested_conf(host_conf);
    int matches = 0;
    nested_conf.for_each_output([&](mg::DisplayConfigurationOutput const& output)
        {
            ASSERT_EQ(host_edid, output.edid);
            ++matches;
        });
    EXPECT_NE(0, matches);
}

TEST(NestedDisplayConfiguration, includes_host_subpixel_arrangement)
{
    auto host_conf = mt::build_trivial_configuration();
    auto const output = mir_display_config_get_output(host_conf.get(), 0);
    auto host_subpixel = mir_output_get_subpixel_arrangement(output);

    mgn::NestedDisplayConfiguration nested_conf(host_conf);
    int matches = 0;
    nested_conf.for_each_output([&](mg::DisplayConfigurationOutput const& output)
        {
            ASSERT_EQ(host_subpixel, output.subpixel_arrangement);
            ++matches;
        });
    EXPECT_NE(0, matches);
}

TEST(NestedDisplayConfiguration, clone_matches_original_configuration)
{
    mgn::NestedDisplayConfiguration config(mt::build_non_trivial_configuration());
    auto cloned_config = config.clone();

    EXPECT_THAT(*cloned_config, mir::test::DisplayConfigMatches(config));
}

TEST(NestedDisplayConfiguration, clone_is_independent_of_original_configuration)
{
    mgn::NestedDisplayConfiguration config(mt::build_non_trivial_configuration());
    auto cloned_config = config.clone();

    config.for_each_output(
        [] (mg::UserDisplayConfigurationOutput& output)
        {
            output.power_mode = mir_power_mode_off;
        });

    cloned_config->for_each_output(
        [] (mg::UserDisplayConfigurationOutput& output)
        {
            output.power_mode = mir_power_mode_on;
        });

    // Check that changes to cloned_config haven't affected original config
    config.for_each_output(
        [] (mg::DisplayConfigurationOutput const& output)
        {
            EXPECT_THAT(output.power_mode, Eq(mir_power_mode_off));
        });
}
