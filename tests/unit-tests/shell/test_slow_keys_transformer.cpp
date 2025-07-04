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

#include "mir_toolkit/event.h"
#include "mir/shell/accessibility_manager.h"
#include "src/server/input/default_event_builder.h"
#include "src/server/shell/basic_slow_keys_transformer.h"

#include "mir/test/doubles/advanceable_clock.h"
#include "mir/test/doubles/queued_alarm_stub_main_loop.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mt::doubles;
using namespace ::testing;
using namespace std::chrono_literals;

namespace
{
static auto constexpr test_slow_keys_delay = 100ms;
}

struct TestSlowKeysTransformer : Test
{
    TestSlowKeysTransformer() :
        main_loop{std::make_shared<mtd::QueuedAlarmStubMainLoop>(mt::fake_shared(clock))},
        transformer{
            std::make_shared<mir::shell::BasicSlowKeysTransformer>(main_loop),
        },
        event_builder{0, mt::fake_shared(clock)},
        dispatch{[&](std::shared_ptr<MirEvent> const&) {}}
    {
        transformer->delay(test_slow_keys_delay);
    }

    auto key_down(unsigned int keysym, unsigned int scancode) -> mir::EventUPtr
    {
        return event_builder.key_event(std::nullopt, mir_keyboard_action_down, keysym, scancode);
    }

    auto key_up(unsigned int keysym, unsigned int scancode) -> mir::EventUPtr
    {
        return event_builder.key_event(std::nullopt, mir_keyboard_action_up, keysym, scancode);
    }

    mtd::AdvanceableClock clock;
    std::shared_ptr<mtd::QueuedAlarmStubMainLoop> const main_loop;
    std::shared_ptr<mir::shell::SlowKeysTransformer> const transformer;
    mi::DefaultEventBuilder event_builder;

    std::function<void(std::shared_ptr<MirEvent> const& event)> dispatch;
};

struct TestDifferentPressDelays: public TestSlowKeysTransformer, public WithParamInterface<std::chrono::milliseconds>
{
};

TEST_P(TestDifferentPressDelays, only_presses_held_for_slow_keys_delay_are_accepted)
{
    auto const press_delay = GetParam();

    auto accepted = 0, rejected = 0, down = 0;
    transformer->on_key_accepted([&accepted](auto) { accepted += 1; });
    transformer->on_key_rejected([&rejected](auto) { rejected += 1; });
    transformer->on_key_down([&down](auto) { down += 1; });

    auto const attempts = 5;
    for(auto i = 0; i < attempts; i++)
    {
        transformer->transform_input_event(dispatch, &event_builder, *key_down(XKB_KEY_d, 32));

        clock.advance_by(press_delay);
        main_loop->call_queued();

        transformer->transform_input_event(dispatch, &event_builder, *key_up(XKB_KEY_d, 32));
    }

    EXPECT_THAT(down, Eq(attempts));
    EXPECT_THAT(press_delay < test_slow_keys_delay ? rejected : accepted, Eq(attempts));
}

INSTANTIATE_TEST_SUITE_P(
    TestSlowKeysTransformer, TestDifferentPressDelays, Values(test_slow_keys_delay - 1ms, test_slow_keys_delay + 1ms));


struct TestKeyInterference: public TestSlowKeysTransformer, public WithParamInterface<std::chrono::milliseconds>
{
};

TEST_P(TestKeyInterference, different_buttons_dont_interfere_with_each_other)
{
    auto const press_delay = GetParam();

    auto accepted = 0, rejected = 0, down = 0;
    transformer->on_key_accepted([&accepted](auto) { accepted += 1; });
    transformer->on_key_rejected([&rejected](auto) { rejected += 1; });
    transformer->on_key_down([&down](auto) { down += 1; });

    auto const attempts = 5;
    for(auto i = 0; i < attempts; i++)
    {
        transformer->transform_input_event(dispatch, &event_builder, *key_down(XKB_KEY_d, 32));
        transformer->transform_input_event(dispatch, &event_builder, *key_down(XKB_KEY_f, 33));

        clock.advance_by(press_delay);
        main_loop->call_queued();

        transformer->transform_input_event(dispatch, &event_builder, *key_up(XKB_KEY_d, 32));
        transformer->transform_input_event(dispatch, &event_builder, *key_up(XKB_KEY_f, 33));
    }

    EXPECT_THAT(down, Eq(attempts * 2));

    if (press_delay < test_slow_keys_delay)
    {
        EXPECT_THAT(accepted, Eq(0));
        EXPECT_THAT(rejected, Eq(attempts * 2));
    }
    else
    {
        EXPECT_THAT(accepted, Eq(attempts * 2));
        EXPECT_THAT(rejected, Eq(0));
    }
}

INSTANTIATE_TEST_SUITE_P(
    TestSlowKeysTransformer, TestKeyInterference, Values(test_slow_keys_delay - 1ms, test_slow_keys_delay + 1ms));
