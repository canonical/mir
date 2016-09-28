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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "src/server/graphics/nested/nested_display_configuration.h"
#include "mir_display_configuration_builder.h"

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
    auto empty_configuration =
        std::shared_ptr<MirDisplayConfiguration>(
            new MirDisplayConfiguration{0, nullptr, 0, nullptr});

    mgn::NestedDisplayConfiguration config(empty_configuration);

    config.for_each_card([](mg::DisplayConfigurationCard const&) { FAIL(); });
    config.for_each_output([](mg::DisplayConfigurationOutput const&) { FAIL(); });
}

TEST(NestedDisplayConfiguration, trivial_configuration_has_one_card)
{
    mgn::NestedDisplayConfiguration config(mt::build_trivial_configuration());

    MockCardVisitor cv;
    EXPECT_CALL(cv, f(_)).Times(Exactly(1));

    config.for_each_card([&cv](mg::DisplayConfigurationCard const& card) { cv.f(card); });
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
    auto const default_current_output_format = mir_config->outputs[0].current_format;
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
            EXPECT_EQ(0, output.current_mode_index);
            EXPECT_EQ(default_current_output_format, output.current_format);
        });
}

// Validation tests once stood here. They've now been replaced by more
// portable validation logic which can be found in:
// TEST(DisplayConfiguration, ...

TEST(NestedDisplayConfiguration, non_trivial_configuration_has_two_cards)
{
    mgn::NestedDisplayConfiguration config(mt::build_non_trivial_configuration());

    MockCardVisitor cv;
    EXPECT_CALL(cv, f(_)).Times(Exactly(2));

    config.for_each_card([&cv](mg::DisplayConfigurationCard const& card) { cv.f(card); });
}

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
    mg::DisplayConfigurationOutputId const id(mir_config->outputs[1].output_id);
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
                EXPECT_EQ(1, output.current_mode_index);
                EXPECT_EQ(mir_pixel_format_argb_8888, output.current_format);
            }
        });
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
