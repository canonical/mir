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

#ifndef TEST_DISPLAY_H_
#define TEST_DISPLAY_H_

#include "mir/test/doubles/mock_display_configuration.h"
#include "mir/renderer/gl/context.h"
#include "mir/test/display_config_matchers.h"

TEST_F(DisplayTestGeneric, configure_disallows_invalid_configuration)
{
    using namespace testing;
    auto display = create_display();
    mir::test::doubles::MockDisplayConfiguration config;

    EXPECT_CALL(config, valid())
        .WillOnce(Return(false));

    EXPECT_THROW({display->configure(config);}, std::logic_error);

    // Determining what counts as a valid configuration is a much trickier
    // platform-dependent exercise, so won't be tested here.
}

#ifdef MIR_DISABLE_TESTS_ON_X11
TEST_F(DisplayTestGeneric, DISABLED_does_not_expose_display_buffer_for_output_with_power_mode_off)
#else
TEST_F(DisplayTestGeneric, does_not_expose_display_buffer_for_output_with_power_mode_off)
#endif
{
    using namespace testing;
    auto display = create_display();
    int db_count{0};

    display->for_each_display_sync_group([&](mg::DisplaySyncGroup& group) {
        group.for_each_display_sink(
            [&](mg::DisplaySink&)
                { ++db_count; });
    });
    EXPECT_THAT(db_count, Eq(1));

    auto conf = display->configuration();
    conf->for_each_output(
        [] (mg::UserDisplayConfigurationOutput& output)
        {
            output.power_mode = mir_power_mode_off;
        });

    display->configure(*conf);

    db_count = 0;
    display->for_each_display_sync_group([&](mg::DisplaySyncGroup& group) {
        group.for_each_display_sink([&](mg::DisplaySink&) { ++db_count; });
    });
    EXPECT_THAT(db_count, Eq(0));
}

TEST_F(DisplayTestGeneric,
       returns_configuration_whose_clone_matches_original_configuration)
{
    using namespace testing;

    auto display = create_display();

    auto config = display->configuration();
    auto cloned_config = config->clone();

    EXPECT_THAT(*cloned_config, mir::test::DisplayConfigMatches(std::cref(*config)));
}

TEST_F(DisplayTestGeneric,
       returns_configuration_whose_clone_is_independent_of_original_configuration)
{
    using namespace testing;

    auto display = create_display();

    auto config = display->configuration();
    auto cloned_config = config->clone();

    config->for_each_output(
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
    config->for_each_output(
        [] (mg::DisplayConfigurationOutput const& output)
        {
            EXPECT_THAT(output.power_mode, Eq(mir_power_mode_off));
        });
}

#endif // TEST_DISPLAY_H_
