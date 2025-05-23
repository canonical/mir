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
#include "mir/shell/keyboard_helper.h"

#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>
#include <optional>

template <typename Transformer>
inline Transformer* mir::shell::BasicAccessibilityManager::Registration<Transformer>::operator->() const noexcept
{
    return transformer.get();
}

template <typename Transformer>
inline void mir::shell::BasicAccessibilityManager::Registration<Transformer>::remove_registration() const
{
    // atomic::compare_exchange_strong(expected, desired) checks the atomic
    // value against the "expected" value. If they're equal, it returns `true`
    // and sets atomic = desired. If they're not equal, it returns `false` and
    // sets expected = atomic which doesn't matter in the case below.
    auto expected = true;
    if (registered.compare_exchange_strong(expected, false))
    {
        event_transformer->remove(transformer);
    }
}

template <typename Transformer>
inline void mir::shell::BasicAccessibilityManager::Registration<Transformer>::add_registration() const
{
    auto expected = false;
    if (registered.compare_exchange_strong(expected, true))
    {
        event_transformer->append(transformer);
    }
}

template <typename Transformer>
inline mir::shell::BasicAccessibilityManager::Registration<Transformer>::Registration(
    std::shared_ptr<Transformer> const& transformer,
    std::shared_ptr<input::InputEventTransformer> const& event_transformer) :
    transformer{transformer},
    event_transformer{event_transformer}
{
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
    if (on)
        transformer.add_registration();
    else
        transformer.remove_registration();
}

mir::shell::BasicAccessibilityManager::BasicAccessibilityManager(
    std::shared_ptr<input::InputEventTransformer> const& event_transformer,
    bool enable_key_repeat,
    std::shared_ptr<mir::graphics::Cursor> const& cursor,
    std::shared_ptr<shell::MouseKeysTransformer> const& mousekeys_transformer) :
    enable_key_repeat{enable_key_repeat},
    cursor{cursor},
    transformer{mousekeys_transformer, event_transformer}
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
