/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "test_window_manager_tools.h"
#include "miral/output.h"

#include <experimental/optional>

using namespace miral;
using namespace testing;
namespace mt = mir::test;

namespace
{
Rectangle const display_area_a{{20, 30}, {600, 400}};
Rectangle const display_area_b{{620, 0}, {800, 500}};

struct OutputUpdates : mt::TestWindowManagerTools
{
    void SetUp() override
    {
        basic_window_manager.add_session(session);
    }

    auto create_window(mir::scene::SurfaceCreationParameters creation_parameters) -> Window
    {
        Window result;

        EXPECT_CALL(*window_manager_policy, advise_new_window(_))
            .WillOnce(
                Invoke(
                    [&result](WindowInfo const& window_info)
                        { result = window_info.window(); }));

        basic_window_manager.add_surface(session, creation_parameters, &create_surface);
        basic_window_manager.select_active_window(result);

        // Clear the expectations used to capture parent & child
        Mock::VerifyAndClearExpectations(window_manager_policy);

        return result;
    }
};
}

TEST_F(OutputUpdates, policy_notified_of_output_creation)
{
    std::experimental::optional<Output> output_a;
    auto display_config_a = create_fake_display_configuration({display_area_a});

    EXPECT_CALL(*window_manager_policy, advise_output_create(_))
        .WillOnce(Invoke([&](Output const& output){ output_a = output; }));

    notify_configuration_applied(display_config_a);

    Mock::VerifyAndClearExpectations(window_manager_policy);
    EXPECT_THAT(output_a.value().extents(), Eq(display_area_a));
}

TEST_F(OutputUpdates, policy_notified_of_multiple_outputs)
{
    std::experimental::optional<Output> output_a;
    std::experimental::optional<Output> output_b;
    auto display_config_a_b = create_fake_display_configuration({display_area_a, display_area_b});

    EXPECT_CALL(*window_manager_policy, advise_output_create(_))
        .WillOnce(Invoke([&](Output const& output){ output_a = output; }))
        .WillOnce(Invoke([&](Output const& output){ output_b = output; }));

    notify_configuration_applied(display_config_a_b);

    Mock::VerifyAndClearExpectations(window_manager_policy);
    EXPECT_THAT(output_a.value().extents(), Eq(display_area_a));
    EXPECT_THAT(output_b.value().extents(), Eq(display_area_b));
    EXPECT_FALSE(output_a.value().is_same_output(output_b.value()));
}

TEST_F(OutputUpdates, policy_notified_of_output_update)
{
    std::experimental::optional<Output> output_initial;
    std::experimental::optional<Output> output_original;
    std::experimental::optional<Output> output_updated;
    auto display_config_a = create_fake_display_configuration({display_area_a});
    auto display_config_b = create_fake_display_configuration({display_area_b});

    EXPECT_CALL(*window_manager_policy, advise_output_create(_))
        .WillOnce(Invoke([&](Output const& output){ output_initial = output; }));

    notify_configuration_applied(display_config_a);

    // Before continuing with the test, we must insure output_initial has been set
    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_output_update(_, _))
        .WillOnce(Invoke([&](Output const& updated, Output const& original){
            output_original = original;
            output_updated = updated;
        }));

    notify_configuration_applied(display_config_b);

    Mock::VerifyAndClearExpectations(window_manager_policy);
    EXPECT_TRUE(output_initial.value().is_same_output(output_original.value()));
    EXPECT_TRUE(output_original.value().is_same_output(output_updated.value()));

    EXPECT_THAT(output_initial.value().extents(), Eq(display_area_a));
    EXPECT_THAT(output_original.value().extents(), Eq(display_area_a));
    EXPECT_THAT(output_updated.value().extents(), Eq(display_area_b));
}

TEST_F(OutputUpdates, policy_notified_of_output_delete)
{
    std::experimental::optional<Output> output_a;
    std::experimental::optional<Output> output_b;
    std::experimental::optional<Output> output_b_deleted;
    auto display_config_a_b = create_fake_display_configuration({display_area_a, display_area_b});
    auto display_config_a = create_fake_display_configuration({display_area_a});

    EXPECT_CALL(*window_manager_policy, advise_output_create(_))
        .WillOnce(Invoke([&](Output const& output){ output_a = output; }))
        .WillOnce(Invoke([&](Output const& output){ output_b = output; }));

    notify_configuration_applied(display_config_a_b);

    // Before continuing with the test, we must insure output_a and output_b have been set
    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_CALL(*window_manager_policy, advise_output_delete(_))
        .WillOnce(Invoke([&](Output const& output){ output_b_deleted = output; }));

    notify_configuration_applied(display_config_a);

    Mock::VerifyAndClearExpectations(window_manager_policy);

    EXPECT_TRUE(output_b_deleted.value().is_same_output(output_b.value()));
    EXPECT_THAT(output_b_deleted.value().extents(), Eq(output_b.value().extents()));
}

TEST_F(OutputUpdates, maximized_window_not_moved_when_new_output_connected)
{
    auto display_config_a = create_fake_display_configuration({display_area_a});
    auto display_config_a_b = create_fake_display_configuration({display_area_a, display_area_b});
    notify_configuration_applied(display_config_a);

    mir::scene::SurfaceCreationParameters creation_parameters;
    creation_parameters.type = mir_window_type_normal;
    creation_parameters.state = mir_window_state_maximized;

    Window window = create_window(creation_parameters);

    ASSERT_THAT(window.top_left(), Eq(display_area_a.top_left));
    ASSERT_THAT(window.size(), Eq(display_area_a.size));

    notify_configuration_applied(display_config_a_b);
    Mock::VerifyAndClearExpectations(window_manager_policy);

    ASSERT_THAT(window.top_left(), Eq(display_area_a.top_left));
    ASSERT_THAT(window.size(), Eq(display_area_a.size));
}

TEST_F(OutputUpdates, maximized_window_moved_with_its_output)
{
    auto display_config_a = create_fake_display_configuration({display_area_a});
    auto display_config_b = create_fake_display_configuration({display_area_b});
    notify_configuration_applied(display_config_a);

    mir::scene::SurfaceCreationParameters creation_parameters;
    creation_parameters.type = mir_window_type_normal;
    creation_parameters.state = mir_window_state_maximized;

    Window window = create_window(creation_parameters);

    ASSERT_THAT(window.top_left(), Eq(display_area_a.top_left));
    ASSERT_THAT(window.size(), Eq(display_area_a.size));

    EXPECT_CALL(*window_manager_policy, advise_move_to(_, _))
        .WillOnce(Invoke([&](miral::WindowInfo const& window_info, mir::geometry::Point top_left)
            {
                EXPECT_THAT(window_info.window(), Eq(window));
                EXPECT_THAT(top_left, Eq(display_area_b.top_left));
            }));
    EXPECT_CALL(*window_manager_policy, advise_resize(_, _))
        .WillOnce(Invoke([&](miral::WindowInfo const& window_info, mir::geometry::Size size)
            {
                EXPECT_THAT(window_info.window(), Eq(window));
                EXPECT_THAT(size, Eq(display_area_b.size));
            }));

    notify_configuration_applied(display_config_b);
    Mock::VerifyAndClearExpectations(window_manager_policy);

    ASSERT_THAT(window.top_left(), Eq(display_area_b.top_left));
    ASSERT_THAT(window.size(), Eq(display_area_b.size));
}

TEST_F(OutputUpdates, maximized_window_moved_when_its_output_disconnected)
{
    auto display_config_a = create_fake_display_configuration({display_area_a});
    auto display_config_a_b = create_fake_display_configuration({display_area_a, display_area_b});
    notify_configuration_applied(display_config_a_b);

    mir::scene::SurfaceCreationParameters creation_parameters;
    creation_parameters.type = mir_window_type_normal;
    creation_parameters.state = mir_window_state_maximized;
    creation_parameters.output_id = mir::graphics::DisplayConfigurationOutputId{2};

    Window window = create_window(creation_parameters);

    ASSERT_THAT(window.top_left(), Eq(display_area_b.top_left));
    ASSERT_THAT(window.size(), Eq(display_area_b.size));

    notify_configuration_applied(display_config_a);
    Mock::VerifyAndClearExpectations(window_manager_policy);

    ASSERT_THAT(window.top_left(), Eq(display_area_a.top_left));
    ASSERT_THAT(window.size(), Eq(display_area_a.size));
}
