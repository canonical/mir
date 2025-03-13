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
#include "mouse_keys_transformer.h"

#include "mir/input/input_event_transformer.h"
#include "mir/main_loop.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "mir/shell/accessibility_manager.h"
#include "mir/shell/keyboard_helper.h"

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

void mir::shell::AccessibilityManager::toggle_mousekeys(bool on)
{
    if (on)
    {
        if (transformer)
            return;

        transformer = std::make_shared<mir::input::MouseKeysTransformer>(
            main_loop,
            geometry::Displacement{
                options->get<double>(mir::options::mouse_keys_max_speed_x),
                options->get<double>(mir::options::mouse_keys_max_speed_y),
            },
            input::MouseKeysTransformer::AccelerationParameters{
                options->get<double>(mir::options::mouse_keys_acceleration_quadratic_factor),
                options->get<double>(mir::options::mouse_keys_acceleration_linear_factor),
                options->get<double>(mir::options::mouse_keys_acceleration_constant_factor),
            });
        event_transformer->append(transformer);
    }
    else
    {
        event_transformer->remove(transformer);
        transformer.reset();
    }
}

void mir::shell::AccessibilityManager::set_mousekeys_keymap(input::MouseKeysKeymap const& new_keymap) const
{
    transformer->set_keymap(new_keymap);
}

mir::shell::AccessibilityManager::AccessibilityManager(
    std::shared_ptr<MainLoop> const& main_loop,
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<input::InputEventTransformer> const& event_transformer) :
    enable_key_repeat{options->get<bool>(options::enable_key_repeat_opt)},
    event_transformer{event_transformer},
    main_loop{main_loop},
    options{options}
{
    if (options->is_set(options::enable_mouse_keys_opt) && options->get<bool>(options::enable_mouse_keys_opt))
        toggle_mousekeys(true);
}
