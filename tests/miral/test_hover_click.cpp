/*
 * Copyright © Canonical Ltd.
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
 */

#include "miral/hover_click.h"
#include "miral/accessibility_test_server.h"

#include <gmock/gmock.h>

using namespace testing;

class TestHoverClick: public miral::TestAccessibilityManager
{
public:
    TestHoverClick()
    {
        using testing::_;
        ON_CALL(accessibility_manager->hover_click_transformer, on_hover_start(_))
            .WillByDefault([](auto ohs) { ohs(); });
        ON_CALL(accessibility_manager->hover_click_transformer, on_hover_cancel(_))
            .WillByDefault([](auto ohc) { ohc(); });
        ON_CALL(accessibility_manager->hover_click_transformer, on_click_dispatched(_))
            .WillByDefault([](auto ocd) { ocd(); });
    }

    miral::HoverClick hover_click{miral::HoverClick::enabled()};
};

TEST_F(TestHoverClick, enabling_from_miral_enables_it_in_the_accessibility_manager)
{
    InSequence seq;
    // Called on server start since we enabled slow keys above
    EXPECT_CALL(*accessibility_manager, hover_click_enabled(true));
    
    // Each one corresponds to a call below
    EXPECT_CALL(*accessibility_manager, hover_click_enabled(true));
    EXPECT_CALL(*accessibility_manager, hover_click_enabled(false));

    add_server_init(hover_click);
    start_server();
    
    hover_click.enable();
    hover_click.disable();
}

TEST_F(TestHoverClick, setting_on_hover_duration_sets_transformer_hover_duration)
{
    auto const hover_duration1 = std::chrono::milliseconds{123};
    auto const hover_duration2 = std::chrono::milliseconds{321};

    InSequence seq;
    EXPECT_CALL(accessibility_manager->hover_click_transformer, hover_duration(hover_duration1));
    EXPECT_CALL(accessibility_manager->hover_click_transformer, hover_duration(hover_duration2));

    add_server_init(hover_click);
    hover_click.hover_duration(hover_duration1);
    start_server();
    hover_click.hover_duration(hover_duration2);
}

TEST_F(TestHoverClick, setting_cancel_displacement_threshold_sets_transformer_cancel_displacement_threshold)
{
    auto const threshold1 = 20;
    auto const threshold2 = 100;

    InSequence seq;
    EXPECT_CALL(accessibility_manager->hover_click_transformer, cancel_displacement_threshold(threshold1));
    EXPECT_CALL(accessibility_manager->hover_click_transformer, cancel_displacement_threshold(threshold2));

    add_server_init(hover_click);
    hover_click.cancel_displacement_threshold(threshold1);
    start_server();
    hover_click.cancel_displacement_threshold(threshold2);
}

TEST_F(TestHoverClick, setting_reclick_displacement_threshold_sets_transformer_reclick_displacement_threshold)
{
    auto const threshold1 = 20;
    auto const threshold2 = 100;

    InSequence seq;
    EXPECT_CALL(accessibility_manager->hover_click_transformer, reclick_displacement_threshold(threshold1));
    EXPECT_CALL(accessibility_manager->hover_click_transformer, reclick_displacement_threshold(threshold2));

    add_server_init(hover_click);
    hover_click.reclick_displacement_threshold(threshold1);
    start_server();
    hover_click.reclick_displacement_threshold(threshold2);
}

TEST_F(TestHoverClick, setting_on_hover_start_sets_transformer_on_hover_start)
{
    auto calls = 0;
    auto const on_hover_start = [&calls] { calls += 1; };

    add_server_init(hover_click);
    
    EXPECT_CALL(accessibility_manager->hover_click_transformer, on_hover_start(_)).Times(0);
    hover_click.on_hover_start(on_hover_start);
    EXPECT_THAT(calls, Eq(0));

    EXPECT_CALL(accessibility_manager->hover_click_transformer, on_hover_start(_)).Times(1);
    start_server();
    EXPECT_THAT(calls, Eq(1));
    
    EXPECT_CALL(accessibility_manager->hover_click_transformer, on_hover_start(_)).Times(1);
    hover_click.on_hover_start(on_hover_start);
    EXPECT_THAT(calls, Eq(2));
}

TEST_F(TestHoverClick, setting_on_hover_cancel_sets_transformer_on_hover_cancel)
{
    auto calls = 0;
    auto const on_hover_cancel = [&calls] { calls += 1; };

    add_server_init(hover_click);

    InSequence seq;

    EXPECT_CALL(accessibility_manager->hover_click_transformer, on_hover_cancel(_)).Times(0);
    hover_click.on_hover_cancel(on_hover_cancel);
    EXPECT_THAT(calls, Eq(0));

    EXPECT_CALL(accessibility_manager->hover_click_transformer, on_hover_cancel(_)).Times(1);
    start_server();
    EXPECT_THAT(calls, Eq(1));
    
    EXPECT_CALL(accessibility_manager->hover_click_transformer, on_hover_cancel(_)).Times(1);
    hover_click.on_hover_cancel(on_hover_cancel);
    EXPECT_THAT(calls, Eq(2));
}

TEST_F(TestHoverClick, setting_on_click_dispatched_sets_transformer_on_click_dispatched)
{
    auto calls = 0;
    auto const on_click_dispatched = [&calls] { calls += 1; };

    add_server_init(hover_click);

    InSequence seq;

    EXPECT_CALL(accessibility_manager->hover_click_transformer, on_click_dispatched(_)).Times(0);
    hover_click.on_click_dispatched(on_click_dispatched);
    EXPECT_THAT(calls, Eq(0));

    EXPECT_CALL(accessibility_manager->hover_click_transformer, on_click_dispatched(_)).Times(1);
    start_server();
    EXPECT_THAT(calls, Eq(1));
    
    EXPECT_CALL(accessibility_manager->hover_click_transformer, on_click_dispatched(_)).Times(1);
    hover_click.on_click_dispatched(on_click_dispatched);
    EXPECT_THAT(calls, Eq(2));
}
