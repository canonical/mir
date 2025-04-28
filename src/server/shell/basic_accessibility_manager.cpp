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

#include "basic_accessibility_manager.h"
#include "mouse_keys_transformer.h"

#include "mir/graphics/cursor.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/event_filter.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_event_transformer.h"
#include "mir/input/input_sink.h"
#include "mir/input/virtual_input_device.h"
#include "mir/main_loop.h"
#include "mir/shell/keyboard_helper.h"

#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>
#include <optional>

class mir::shell::BasicAccessibilityManager::MousekeyPointer : public mir::input::VirtualInputDevice, public input::EventFilter
{
public:
    MousekeyPointer(std::shared_ptr<MainLoop> main_loop, std::shared_ptr<input::InputEventTransformer> iet) :
        mir::input::VirtualInputDevice{"mousekey-pointer", mir::input::DeviceCapability::pointer},
        main_loop{std::move(main_loop)},
        iet{std::move(iet)}
    {
    }

    bool handle(MirEvent const& event) override
    {
        auto handled = false;
        if_started_then([this, &event, &handled](auto* sink, auto* builder)
        {
            handled = iet->transform(event, builder, [this, sink](auto event)
                {
                    main_loop->spawn([sink, event]{ sink->handle_input(event); });
                });
        });
        return handled;
    }

private:
    std::shared_ptr<MainLoop> const main_loop;
    std::shared_ptr<input::InputEventTransformer> const iet;
};

void mir::shell::BasicAccessibilityManager::register_keyboard_helper(std::shared_ptr<KeyboardHelper> const& helper)
{
    mutable_state.lock()->keyboard_helpers.push_back(helper);
}

std::optional<int> mir::shell::BasicAccessibilityManager::repeat_rate() const {
    if (!enable_key_repeat)
        return {};
    return mutable_state.lock()->repeat_rate;
}

int mir::shell::BasicAccessibilityManager::repeat_delay() const {
    return mutable_state.lock()->repeat_delay;
}

void mir::shell::BasicAccessibilityManager::repeat_rate_and_delay(
    std::optional<int> new_rate, std::optional<int> new_delay)
{
    auto const state = mutable_state.lock();

    auto const maybe_new_rate = new_rate.value_or(state->repeat_rate);
    auto const maybe_new_delay = new_delay.value_or(state->repeat_delay);

    auto const changed = maybe_new_rate != state->repeat_rate || maybe_new_delay != state->repeat_delay;

    state->repeat_rate = maybe_new_rate;
    state->repeat_delay = maybe_new_delay;

    if (changed)
    {
        for (auto const& helper : state->keyboard_helpers)
            helper->repeat_info_changed(
                enable_key_repeat ? state->repeat_rate : std::optional<int>{}, state->repeat_delay);
    }
}

void mir::shell::BasicAccessibilityManager::mousekeys_enabled(bool on)
{
    if (on)
    {
        main_loop->spawn([this]
        {
            auto state = this->mutable_state.lock();
            if (!state->mousekey_pointer)
            {
                state->mousekey_pointer = std::make_shared<MousekeyPointer>(main_loop, event_transformer);

                event_transformer->append(transformer);
                input_device_registry->add_device(state->mousekey_pointer);
                the_composite_event_filter->prepend(state->mousekey_pointer);
            }
        });
    }
    else
    {
        main_loop->spawn([this]
        {
            auto state = this->mutable_state.lock();
            if (state->mousekey_pointer)
            {
                input_device_registry->remove_device(state->mousekey_pointer);
                event_transformer->remove(transformer);
                state->mousekey_pointer.reset();
            }
        });
    }
}

mir::shell::BasicAccessibilityManager::BasicAccessibilityManager(
    std::shared_ptr<MainLoop> main_loop,
    std::shared_ptr<input::CompositeEventFilter> the_composite_event_filter,
    std::shared_ptr<input::InputEventTransformer> const& event_transformer,
    bool enable_key_repeat,
    std::shared_ptr<mir::graphics::Cursor> const& cursor,
    std::shared_ptr<shell::MouseKeysTransformer> const& mousekeys_transformer,
    std::shared_ptr<input::InputDeviceRegistry> const& input_device_registry) :
    main_loop{std::move(main_loop)},
    the_composite_event_filter{std::move(the_composite_event_filter)},
    enable_key_repeat{enable_key_repeat},
    cursor{cursor},
    event_transformer{event_transformer},
    transformer{mousekeys_transformer},
    input_device_registry{input_device_registry}
{
}

void mir::shell::BasicAccessibilityManager::cursor_scale(float new_scale)
{
    cursor->scale(std::clamp(0.0f, 100.0f, new_scale));
}

void mir::shell::BasicAccessibilityManager::mousekeys_keymap(input::MouseKeysKeymap const& new_keymap)
{
    transformer->keymap(new_keymap);
}

void mir::shell::BasicAccessibilityManager::acceleration_factors(double constant, double linear, double quadratic)
{
    transformer->acceleration_factors(constant, linear, quadratic);
}

void mir::shell::BasicAccessibilityManager::max_speed(double x_axis, double y_axis)
{
    transformer->max_speed(x_axis, y_axis);
}
