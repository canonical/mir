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
#include "mir/options/configuration.h"
#include "mir/options/option.h"
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

void mir::shell::BasicAccessibilityManager::set_mousekeys_enabled(bool on)
{
    if (on)
    {
        if (transformer)
            return;

        transformer = mousekeys_transformer_builder(
            keymap,
            acceleration_constant,
            acceleration_linear,
            acceleration_quadratic,
            max_speed.dx.as_value(),
            max_speed.dy.as_value());
        event_transformer->append(transformer);
    }
    else
    {
        event_transformer->remove(transformer);
        transformer.reset();
    }
}

void mir::shell::BasicAccessibilityManager::set_mousekeys_keymap(input::MouseKeysKeymap const& new_keymap)
{
    keymap = new_keymap;
    if(transformer)
        transformer->set_keymap(new_keymap);
}

mir::shell::BasicAccessibilityManager::BasicAccessibilityManager(
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<input::InputEventTransformer> const& event_transformer,
    std::shared_ptr<time::Clock> const& clock,
    bool enable_key_repeat) :
    mir::shell::BasicAccessibilityManager(
        main_loop,
        event_transformer,
        clock,
        enable_key_repeat,
        [&](input::MouseKeysKeymap const& keymap,
            double acceleration_constant_factor,
            double acceleration_linear_factor,
            double acceleration_quadratic_factor,
            double max_speed_x,
            double max_speed_y)
        {
            return std::make_shared<mir::input::BasicMouseKeysTransformer>(
                main_loop,
                mir::geometry::DisplacementF{max_speed_x, max_speed_y},
                input::BasicMouseKeysTransformer::AccelerationParameters{
                    acceleration_quadratic_factor,
                    acceleration_linear_factor,
                    acceleration_constant_factor,
                },
                clock,
                keymap);
        })
{
}

void mir::shell::BasicAccessibilityManager::set_acceleration_factors(double constant, double linear, double quadratic)
{
    acceleration_constant = constant;
    acceleration_linear = linear;
    acceleration_quadratic = quadratic;
    if(transformer)
        transformer->set_acceleration_factors(constant, linear, quadratic);
}

void mir::shell::BasicAccessibilityManager::set_max_speed(double x_axis, double y_axis)
{
    max_speed = {x_axis, y_axis};
    if(transformer)
        transformer->set_max_speed(x_axis, y_axis);
}

mir::shell::BasicAccessibilityManager::BasicAccessibilityManager(
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<input::InputEventTransformer> const& event_transformer,
    std::shared_ptr<time::Clock> const& clock,
    bool enable_key_repeat,
    MouseKeysTransformerBuilder&& builder) :
    enable_key_repeat{enable_key_repeat},
    event_transformer{event_transformer},
    main_loop{main_loop},
    clock{clock},
    keymap{input::BasicMouseKeysTransformer::default_keymap},
    mousekeys_transformer_builder{std::move(builder)}
{
}
