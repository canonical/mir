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
#include "locate_pointer_filter.h"

#include "mir/graphics/cursor.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/input_event_transformer.h"
#include "mir/shell/keyboard_helper.h"

#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>
#include <optional>

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

void mir::shell::BasicAccessibilityManager::repeat_rate(int new_rate) {
    mutable_state.lock()->repeat_rate = new_rate;
}

void mir::shell::BasicAccessibilityManager::repeat_delay(int new_delay) {
    mutable_state.lock()->repeat_delay = new_delay;
}

void mir::shell::BasicAccessibilityManager::notify_helpers() const {
    for (auto const& helper: mutable_state.lock()->keyboard_helpers)
        helper->repeat_info_changed(repeat_rate(), repeat_delay());
}

void mir::shell::BasicAccessibilityManager::mousekeys_enabled(bool on)
{
    if (on)
        event_transformer->append(transformer);
    else
        event_transformer->remove(transformer);
}

mir::shell::BasicAccessibilityManager::BasicAccessibilityManager(
    std::shared_ptr<input::InputEventTransformer> const& event_transformer,
    std::shared_ptr<input::CompositeEventFilter> const& composite_filter,
    bool enable_key_repeat,
    std::shared_ptr<mir::graphics::Cursor> const& cursor,
    std::shared_ptr<shell::MouseKeysTransformer> const& mousekeys_transformer,
    LocatePointerFilterBuilder&& locate_pointer_filter_builder) :
    enable_key_repeat{enable_key_repeat},
    cursor{cursor},
    event_transformer{event_transformer},
    composite_filter{composite_filter},
    transformer{mousekeys_transformer},
    locate_pointer_filter_builder{std::move(locate_pointer_filter_builder)}
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

void mir::shell::BasicAccessibilityManager::locate_pointer_delay(std::chrono::milliseconds delay)
{
    locate_pointer_filter_state.delay = delay;
}

void mir::shell::BasicAccessibilityManager::on_locate_pointer(std::function<void(float x, float y)>&& on_locate_pointer)
{
    locate_pointer_filter_state.on_locate_pointer = std::move(on_locate_pointer);
}

void mir::shell::BasicAccessibilityManager::locate_pointer_enabled(bool on)
{
    if (on)
    {
        locate_pointer_filter = locate_pointer_filter_builder(locate_pointer_filter_state);
        composite_filter->append(locate_pointer_filter);
    }
    else if (locate_pointer_filter)
    {
        locate_pointer_filter.reset();
    };
}
