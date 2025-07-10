/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "basic_sticky_keys_transformer.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include <linux/input-event-codes.h>
#include <vector>

#include "mir/synchronised.h"
#include "mir/events/pointer_event.h"
#include "mir/events/touch_event.h"

namespace msh = mir::shell;
namespace mi = mir::input;

namespace
{
bool is_modifier(int32_t scan_code)
{
    std::array constexpr modifiers = {
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
    };

    for (auto const modifier : modifiers)
    {
        if (modifier == scan_code)
            return true;
    }

    return false;
}
}

struct msh::BasicStickyKeysTransformer::Self
{
    struct Userdata
    {
        bool should_disable_if_two_keys_are_pressed_together = true;
        std::function<void(int32_t)> on_modifier_clicked{[](int32_t) {}};
    };

    Synchronised<Userdata> userdata;
    uint32_t held_count = 0;
    bool is_temporarily_disabled = false;
    std::vector<std::shared_ptr<MirKeyboardEvent>> pending;

    bool try_dispatch_pending(
        MirEvent const& event,
        bool is_up_event,
        mi::Transformer::EventDispatcher const& dispatcher)
    {
        if (!is_up_event)
            return false;

        dispatcher(std::shared_ptr<MirEvent>(event.clone()));

        for (auto const& item : pending)
        {
            dispatcher(item);
        }
        pending.clear();
        return true;
    }
};

msh::BasicStickyKeysTransformer::BasicStickyKeysTransformer()
    : self{std::make_shared<Self>()}
{
}

bool msh::BasicStickyKeysTransformer::transform_input_event(
    mi::Transformer::EventDispatcher const& dispatcher,
    input::EventBuilder*,
    MirEvent const& event)
{
    if (event.type() != mir_event_type_input)
        return false;

    auto* const input_event = event.to_input();
    auto const input_type = input_event->input_type();
    if (input_type == mir_input_event_type_key)
    {
        auto* const key_event = input_event->to_keyboard();
        auto const scan_code = key_event->scan_code();
        if (!is_modifier(scan_code))
        {
            return self->try_dispatch_pending(
                event,
                key_event->action() == mir_keyboard_action_up,
                dispatcher);
        }

        switch (key_event->action())
        {
            case mir_keyboard_action_down:
            {
                self->held_count++;

                // In the case that we're holding more than one modifier
                // keys down simultaneously, the transformer is temporarily
                // disabled until all modifiers are released. This is optionally
                // turned on by [should_disable_if_two_keys_are_pressed_together].
                // By only setting [is_temporarily_disabled] when it is unset,
                // we make sure that setting [should_disable_if_two_keys_are_pressed_together]
                // to true while we are in the middle of holding modifiers has no
                // effect. It will only go into effect after all modifiers have been released.
                if (!self->is_temporarily_disabled)
                {
                    self->is_temporarily_disabled =
                        self->userdata.lock()->should_disable_if_two_keys_are_pressed_together
                        && self->held_count > 1;
                }
                break;
            }
            case mir_keyboard_action_up:
                self->held_count--;
                if (self->is_temporarily_disabled)
                {
                    if (self->held_count == 0)
                        self->is_temporarily_disabled = false;
                    return false;
                }

                self->userdata.lock()->on_modifier_clicked(scan_code);
                self->pending.push_back(std::shared_ptr<MirKeyboardEvent>(key_event->clone()));
                return true;
            default:
                break;
        }
    }
    else if (input_type == mir_input_event_type_pointer)
    {
        auto const pointer_event = input_event->to_pointer();
        return self->try_dispatch_pending(
            event,
            pointer_event->action() == mir_pointer_action_button_up,
            dispatcher);
    }
    else if (input_type == mir_input_event_type_touch)
    {
        auto const touch_event = input_event->to_touch();
        auto const pointer_count = touch_event->pointer_count();
        return self->try_dispatch_pending(
            event,
            pointer_count == 1 && touch_event->action(0) == mir_touch_action_up,
            dispatcher);
    }

    return false;
}

void msh::BasicStickyKeysTransformer::should_disable_if_two_keys_are_pressed_together(bool on)
{
    self->userdata.lock()->should_disable_if_two_keys_are_pressed_together = on;
}

void msh::BasicStickyKeysTransformer::on_modifier_clicked(std::function<void(int32_t)>&& callback)
{
    self->userdata.lock()->on_modifier_clicked = std::move(callback);
}
