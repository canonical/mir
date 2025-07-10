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

#include "src/server/shell/basic_sticky_keys_transformer.h"
#include "src/server/input/default_event_builder.h"
#include "mir/test/doubles/advanceable_clock.h"
#include "mir/events/event.h"
#include <linux/input-event-codes.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mt = mir::time;
namespace miev = mir::events;
namespace mtd = mir::test::doubles;

namespace
{
class MockDispatcher
{
public:
    MOCK_METHOD((void), dispatcher, (std::shared_ptr<MirEvent> const&), (const));
};
}

class TestStickyKeysTransformer : public Test
{
public:
    TestStickyKeysTransformer()
        : event_builder(0, clock),
          dispatch([&](std::shared_ptr<MirEvent> const& event) { dispatcher->dispatcher(event); })
    {
    }

    msh::BasicStickyKeysTransformer transformer = msh::BasicStickyKeysTransformer();
    std::shared_ptr<mt::Clock> clock = std::make_shared<mtd::AdvanceableClock>();
    mi::DefaultEventBuilder event_builder;
    std::shared_ptr<MockDispatcher> dispatcher = std::make_shared<MockDispatcher>();
    std::function<void(std::shared_ptr<MirEvent> const& event)> dispatch;
};

TEST_F(TestStickyKeysTransformer, non_modifier_events_are_passed_through_when_no_modifiers_are_previously_clicked)
{
    auto const event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, XKB_KEY_0, KEY_0);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, *event.get()), Eq(false));
}

class TestStickyKeysTransformerWithModifiers : public TestStickyKeysTransformer, public WithParamInterface<int32_t>
{};

TEST_P(TestStickyKeysTransformerWithModifiers, modifier_keyboard_up_events_are_consumed)
{
    auto const modifier_key = GetParam();
    auto const down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, modifier_key, modifier_key);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, *down_event.get()), Eq(false));

    auto const up_event = event_builder.key_event(std::nullopt, mir_keyboard_action_up, modifier_key, modifier_key);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, *up_event.get()), Eq(true));
}

TEST_P(TestStickyKeysTransformerWithModifiers, callback_triggered_on_modifier_key_up)
{
    int count = 0;
    transformer.on_modifier_clicked([&](auto const& key)
    {
        EXPECT_THAT(key, Eq(GetParam()));
        count++;
    });

    auto const modifier_key = GetParam();
    auto const down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, modifier_key, modifier_key);
    transformer.transform_input_event(dispatch, &event_builder, *down_event.get());
    auto const up_event = event_builder.key_event(std::nullopt, mir_keyboard_action_up, modifier_key, modifier_key);
    transformer.transform_input_event(dispatch, &event_builder, *up_event.get());
    EXPECT_THAT(count, Eq(1));
}

TEST_P(TestStickyKeysTransformerWithModifiers, up_events_are_dispatched_following_non_modifier_key_up)
{
    EXPECT_CALL(*dispatcher, dispatcher(_)).Times(2); // Once for the redispatch of the key event, once for the modifier up

    // Setup: "Click" the modifier key
    auto const modifier_key = GetParam();
    auto const down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, modifier_key, modifier_key);
    transformer.transform_input_event(dispatch, &event_builder, *down_event.get());
    auto const up_event = event_builder.key_event(std::nullopt, mir_keyboard_action_up, modifier_key, modifier_key);
    transformer.transform_input_event(dispatch, &event_builder, *up_event.get());

    // Action: "Click" a non-modifier key
    auto const non_modifier_down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, XKB_KEY_0, KEY_0);
    transformer.transform_input_event(dispatch, &event_builder, *non_modifier_down_event.get());

    auto const non_modifier_up_event = event_builder.key_event(std::nullopt, mir_keyboard_action_up, XKB_KEY_0, KEY_0);
    transformer.transform_input_event(dispatch, &event_builder, *non_modifier_up_event.get());
}

TEST_P(TestStickyKeysTransformerWithModifiers, up_events_are_dispatched_following_pointer_button_up_event)
{
    EXPECT_CALL(*dispatcher, dispatcher(_)).Times(2); // Once for the redispatch of the mouse up event, once for the modifier up

    // Setup: "Click" the modifier key
    auto const modifier_key = GetParam();
    auto const down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, modifier_key, modifier_key);
    transformer.transform_input_event(dispatch, &event_builder, *down_event.get());
    auto const up_event = event_builder.key_event(std::nullopt, mir_keyboard_action_up, modifier_key, modifier_key);
    transformer.transform_input_event(dispatch, &event_builder, *up_event.get());

    // Action: "Click" a mouse button
    auto const mouse_down_event = event_builder.pointer_event(
        std::nullopt, mir_pointer_action_button_down, mir_pointer_button_primary, 0, 0, 0, 0);
    transformer.transform_input_event(dispatch, &event_builder, *mouse_down_event.get());

    auto const mouse_up_event = event_builder.pointer_event(
        std::nullopt, mir_pointer_action_button_up, mir_pointer_button_primary, 0, 0, 0, 0);
    transformer.transform_input_event(dispatch, &event_builder, *mouse_up_event.get());
}

TEST_P(TestStickyKeysTransformerWithModifiers, up_events_are_dispatched_following_touch_up_event)
{
    EXPECT_CALL(*dispatcher, dispatcher(_)).Times(2); // Once for the redispatch of the touch up event, once for the modifier up

    // Setup: "Click" the modifier key
    auto const modifier_key = GetParam();
    auto const down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, modifier_key, modifier_key);
    transformer.transform_input_event(dispatch, &event_builder, *down_event.get());
    auto const up_event = event_builder.key_event(std::nullopt, mir_keyboard_action_up, modifier_key, modifier_key);
    transformer.transform_input_event(dispatch, &event_builder, *up_event.get());

    // Action: "Tap" on the screen
    miev::TouchContactV2 contact_down;
    contact_down.action = mir_touch_action_down;
    auto const touch_down_event = event_builder.touch_event(std::nullopt, { contact_down });
    transformer.transform_input_event(dispatch, &event_builder, *touch_down_event.get());

    miev::TouchContactV2 contact_up;
    contact_up.action = mir_touch_action_up;
    auto const touch_up_event = event_builder.touch_event(std::nullopt, { contact_up });
    transformer.transform_input_event(dispatch, &event_builder, *touch_up_event.get());
}

INSTANTIATE_TEST_SUITE_P(TestStickyKeysTransformerWithModifiers, TestStickyKeysTransformerWithModifiers, Values(
    KEY_RIGHTSHIFT,
    KEY_LEFTSHIFT,
    KEY_RIGHTALT,
    KEY_LEFTALT,
    KEY_RIGHTCTRL,
    KEY_LEFTCTRL,
    KEY_RIGHTMETA,
    KEY_LEFTMETA,
    KEY_CAPSLOCK,
    KEY_SCREENLOCK,
    KEY_NUMLOCK
));

TEST_F(TestStickyKeysTransformer, holding_two_modifiers_simultaneously_disables_sticky_keys_transformer)
{
    transformer.should_disable_if_two_keys_are_pressed_together(true);

    auto const first_down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, KEY_RIGHTSHIFT, KEY_RIGHTSHIFT);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, *first_down_event.get()), Eq(false));

    auto const second_down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, KEY_RIGHTALT, KEY_RIGHTALT);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, *second_down_event.get()), Eq(false));

    auto const up_event = event_builder.key_event(std::nullopt, mir_keyboard_action_up, KEY_RIGHTSHIFT, KEY_RIGHTSHIFT);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, *up_event.get()), Eq(false));
}

TEST_F(TestStickyKeysTransformer, holding_two_modifiers_simultaneously_does_not_disable_transformer_when_options_is_false)
{
    transformer.should_disable_if_two_keys_are_pressed_together(false);

    auto const first_down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, KEY_RIGHTSHIFT, KEY_RIGHTSHIFT);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, *first_down_event.get()), Eq(false));

    auto const second_down_event = event_builder.key_event(std::nullopt, mir_keyboard_action_down, KEY_RIGHTALT, KEY_RIGHTALT);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, *second_down_event.get()), Eq(false));

    auto const up_event = event_builder.key_event(std::nullopt, mir_keyboard_action_up, KEY_RIGHTSHIFT, KEY_RIGHTSHIFT);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, *up_event.get()), Eq(true));
}

TEST_F(TestStickyKeysTransformer, non_input_events_are_ignored)
{
    class StubEvent : public MirEvent
    {
        public:
            explicit StubEvent(MirEventType type) : MirEvent(type) {}
            auto clone() const -> MirEvent* override { return nullptr; }
    };

    StubEvent event(mir_event_type_close_window);
    EXPECT_THAT(transformer.transform_input_event(dispatch, &event_builder, event), Eq(false));
}
