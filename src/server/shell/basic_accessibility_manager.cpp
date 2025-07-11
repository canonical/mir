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
#include "basic_hover_click_transformer.h"
#include "mouse_keys_transformer.h"
#include "basic_simulated_secondary_click_transformer.h"
#include "basic_slow_keys_transformer.h"

#include "mir/graphics/cursor.h"
#include "mir/shell/keyboard_helper.h"

#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>
#include <optional>

namespace
{
using miet = mir::input::InputEventTransformer;
void toggle_transformer(
    bool should_turn_on,
    bool& is_enabled,
    std::shared_ptr<miet::Transformer> const& transformer,
    std::shared_ptr<miet> const& event_transformer)
{
    if (should_turn_on && !is_enabled)
    {
        event_transformer->append(transformer);
        is_enabled = true;
    }
    else if (!should_turn_on && is_enabled)
    {
        event_transformer->remove(transformer);
        is_enabled = false;
    }
}
}

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
    auto const state = mutable_state.lock();
    toggle_transformer(on, state->mousekeys_on, mouse_keys_transformer, event_transformer);
}

mir::shell::BasicAccessibilityManager::BasicAccessibilityManager(
    std::shared_ptr<input::InputEventTransformer> const& event_transformer,
    bool enable_key_repeat,
    std::shared_ptr<mir::graphics::Cursor> const& cursor,
    std::shared_ptr<shell::MouseKeysTransformer> const& mousekeys_transformer,
    std::shared_ptr<SimulatedSecondaryClickTransformer> const& simulated_secondary_click_transformer,
    std::shared_ptr<HoverClickTransformer> const& hover_click_transformer,
    std::shared_ptr<SlowKeysTransformer> const& slow_keys_transformer) :
    enable_key_repeat{enable_key_repeat},
    cursor{cursor},
    event_transformer{event_transformer},
    mouse_keys_transformer{mousekeys_transformer},
    simulated_secondary_click_transformer{simulated_secondary_click_transformer},
    hover_click_transformer{hover_click_transformer},
    slow_keys_transformer{slow_keys_transformer}
{
}

mir::shell::BasicAccessibilityManager::~BasicAccessibilityManager() = default;

void mir::shell::BasicAccessibilityManager::cursor_scale(float new_scale)
{
    cursor->scale(std::clamp(0.0f, 100.0f, new_scale));
}

void mir::shell::BasicAccessibilityManager::mousekeys_keymap(input::MouseKeysKeymap const& new_keymap)
{
    mouse_keys_transformer->keymap(new_keymap);
}

void mir::shell::BasicAccessibilityManager::acceleration_factors(double constant, double linear, double quadratic)
{
    mouse_keys_transformer->acceleration_factors(constant, linear, quadratic);
}

void mir::shell::BasicAccessibilityManager::max_speed(double x_axis, double y_axis)
{
    mouse_keys_transformer->max_speed(x_axis, y_axis);
}

void mir::shell::BasicAccessibilityManager::simulated_secondary_click_enabled(bool enabled)
{
    auto const state = mutable_state.lock();
    toggle_transformer(enabled, state->ssc_on, simulated_secondary_click_transformer, event_transformer);
}

auto mir::shell::BasicAccessibilityManager::simulated_secondary_click()
    -> SimulatedSecondaryClickTransformer&
{
    return *simulated_secondary_click_transformer.operator->();
}

void mir::shell::BasicAccessibilityManager::hover_click_enabled(bool enabled)
{
    auto const state = mutable_state.lock();
    toggle_transformer(enabled, state->hover_click_on, hover_click_transformer, event_transformer);
}

auto mir::shell::BasicAccessibilityManager::hover_click() -> HoverClickTransformer&
{
    return *hover_click_transformer;
}

void mir::shell::BasicAccessibilityManager::slow_keys_enabled(bool enabled)
{
    auto const state = mutable_state.lock();
    toggle_transformer(enabled, state->slow_keys_on, slow_keys_transformer, event_transformer);
}

auto mir::shell::BasicAccessibilityManager::slow_keys() -> SlowKeysTransformer&
{
    return *slow_keys_transformer;
}
