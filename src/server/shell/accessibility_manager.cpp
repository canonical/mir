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

#include "mir/shell/accessibility_manager.h"

#include "mir/geometry/displacement.h"
#include "mir/input/event_builder.h"
#include "mir/main_loop.h"
#include "mir/options/configuration.h"
#include "mir/shell/keyboard_helper.h"
#include "mir/time/alarm.h"

#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>
#include <optional>

void mir::shell::AccessibilityManager::register_keyboard_helper(std::shared_ptr<KeyboardHelper> const& helper)
{
    mutable_state.lock()->keyboard_helpers.push_back(helper);
}

std::optional<int> mir::shell::AccessibilityManager::repeat_rate() const {
    if(!enable_key_repeat)
        return {};
    return mutable_state.lock()->repeat_rate;
}

int mir::shell::AccessibilityManager::repeat_delay() const {
    return mutable_state.lock()->repeat_delay;
}

void mir::shell::AccessibilityManager::repeat_rate(int new_rate) {
    mutable_state.lock()->repeat_rate = new_rate;
}

void mir::shell::AccessibilityManager::repeat_delay(int new_delay) {
    mutable_state.lock()->repeat_delay = new_delay;
}

void mir::shell::AccessibilityManager::notify_helpers() const {
    for(auto const& helper: mutable_state.lock()->keyboard_helpers)
        helper->repeat_info_changed(repeat_rate(), repeat_delay());
}

struct MouseKeysTransformer: public mir::input::InputEventTransformer::Transformer
{
    using Dispatcher = mir::input::InputEventTransformer::EventDispatcher;

    bool transform_input_event(
        mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
        mir::input::EventBuilder* builder,
        MirEvent const& event) override
    {
        using namespace mir; // For operator<<

        if (mir_event_get_type(&event) != mir_event_type_input)
            return false;

        MirInputEvent const* input_event = mir_event_get_input_event(&event);

        if (mir_input_event_get_type(input_event) == mir_input_event_type_key)
        {
            MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);

            if(handle_motion(kev, dispatcher, builder))
                return true;
        }

        return false;
    }

    bool handle_motion(
        MirKeyboardEvent const* kev, Dispatcher const& dispatcher, mir::input::EventBuilder* const builder)
    {
        namespace geom = mir::geometry;

        auto const action = mir_keyboard_event_action(kev);
        auto const set_or_clear = [](uint32_t& buttons_down, DirectionalButtons button, MirKeyboardAction action)
        {
            if (action == mir_keyboard_action_down)
                buttons_down |= button;
            else if (action == mir_keyboard_action_up)
                buttons_down &= ~(button);
        };

        switch (mir_keyboard_event_keysym(kev))
        {
        case XKB_KEY_KP_2:
            set_or_clear(buttons_down, down, action);
            break;
        case XKB_KEY_KP_4:
            set_or_clear(buttons_down, left, action);
            break;
        case XKB_KEY_KP_6:
            set_or_clear(buttons_down, right, action);
            break;
        case XKB_KEY_KP_8:
            set_or_clear(buttons_down, up, action);
            break;
        default:
            return false;
        }

        motion_direction = {0, 0};
        if (buttons_down & up)
            motion_direction.dy += geom::DeltaYF{-10};
        if (buttons_down & down)
            motion_direction.dy += geom::DeltaYF{10};
        if (buttons_down & left)
            motion_direction.dx += geom::DeltaXF{-10};
        if (buttons_down & right)
            motion_direction.dx += geom::DeltaXF{10};

        switch (mir_keyboard_event_action(kev))
        {
        case mir_keyboard_action_up:
            if (buttons_down == none)
                motion_event_generator.reset();
            return true;
        case mir_keyboard_action_down:
            {
                if (!motion_event_generator)
                {
                    motion_event_generator =
                        main_loop->create_alarm(
                            [dispatcher, this, builder]
                            {
                                dispatcher(builder->pointer_event(
                                    std::nullopt,
                                    mir_pointer_action_motion,
                                    0,
                                    std::nullopt,
                                    motion_direction,
                                    mir_pointer_axis_source_none,
                                    mir::events::ScrollAxisH{},
                                    mir::events::ScrollAxisV{}));

                                motion_event_generator->reschedule_in(std::chrono::milliseconds(10));
                            });

                    motion_event_generator->reschedule_in(std::chrono::milliseconds(0));
                }
            }
            return true;
        case mir_keyboard_action_repeat:
            return true;
        default:
            return false;
        }

        return false;
    }

    std::shared_ptr<mir::MainLoop> const main_loop;

    std::unique_ptr<mir::time::Alarm> motion_event_generator;

    mir::geometry::DisplacementF motion_direction;

    enum DirectionalButtons
    {
        none = 0,
        up = 1 << 0,
        down = 1 << 1,
        left = 1 << 2,
        right = 1 << 3
    };

    uint32_t buttons_down{none};
};

mir::shell::AccessibilityManager::AccessibilityManager(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<input::InputEventTransformer> const& event_transformer) :
    enable_key_repeat{options->get<bool>(options::enable_key_repeat_opt)},
    enable_mouse_keys{options->get<bool>(options::enable_mouse_keys_opt)},
    event_transformer{event_transformer},
    transformer{std::make_shared<MouseKeysTransformer>()}
{
    if (enable_mouse_keys)
        event_transformer->append(transformer);
}
