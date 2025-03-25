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

#include "mir/input/input_event_transformer.h"
#include "mir/main_loop.h"
#include "mir/shell/keyboard_helper.h"

#include <xkbcommon/xkbcommon-keysyms.h>

#include <memory>
#include <optional>

void mir::shell::BasicAccessibilityManager::register_keyboard_helper(std::shared_ptr<KeyboardHelper> const& helper)
{
    keyboard_helpers.push_back(helper);
}

std::optional<int> mir::shell::BasicAccessibilityManager::repeat_rate() const {
    if(!enable_key_repeat)
        return {};
    return repeat_rate_;
}

int mir::shell::BasicAccessibilityManager::repeat_delay() const {
    return repeat_delay_;
}

void mir::shell::BasicAccessibilityManager::repeat_rate(int new_rate) {
    repeat_rate_ = new_rate;
}

void mir::shell::BasicAccessibilityManager::repeat_delay(int new_delay) {
    repeat_delay_ = new_delay;
}

void mir::shell::BasicAccessibilityManager::notify_helpers() const {
    for(auto const& helper: keyboard_helpers)
        helper->repeat_info_changed(repeat_rate(), repeat_delay());
}

void mir::shell::BasicAccessibilityManager::mousekeys_enabled(bool on)
{
    if (on)
    {
        if (transformer)
            return;

        event_transformer->append(transformer);
    }
    else
    {
        event_transformer->remove(transformer);
    }
}

void mir::shell::BasicAccessibilityManager::mousekeys_keymap(input::MouseKeysKeymap const& new_keymap)
{
    if(transformer)
        transformer->keymap(new_keymap);
}

mir::shell::BasicAccessibilityManager::BasicAccessibilityManager(
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<input::InputEventTransformer> const& event_transformer,
    std::shared_ptr<time::Clock> const& clock,
    bool enable_key_repeat) :
    enable_key_repeat{enable_key_repeat},
    event_transformer{event_transformer},
    main_loop{main_loop},
    clock{clock},
    transformer{std::make_shared<mir::input::BasicMouseKeysTransformer>(main_loop, clock)}
{
}

mir::shell::BasicAccessibilityManager::BasicAccessibilityManager(
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<input::InputEventTransformer> const& event_transformer,
    std::shared_ptr<time::Clock> const& clock,
    bool enable_key_repeat,
    std::shared_ptr<input::MouseKeysTransformer> const& mousekeys_transformer) :
    enable_key_repeat{enable_key_repeat},
    event_transformer{event_transformer},
    main_loop{main_loop},
    clock{clock},
    transformer{mousekeys_transformer}
{
}

void mir::shell::BasicAccessibilityManager::acceleration_factors(double constant, double linear, double quadratic)
{
    if(transformer)
        transformer->acceleration_factors(constant, linear, quadratic);
}

void mir::shell::BasicAccessibilityManager::max_speed(double x_axis, double y_axis)
{
    if(transformer)
        transformer->max_speed(x_axis, y_axis);
}
