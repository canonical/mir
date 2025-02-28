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

#include "mir/graphics/cursor.h"
#include "mir/options/configuration.h"
#include "mir/shell/keyboard_helper.h"

#include <optional>

mir::shell::AccessibilityManager::AccessibilityManager(
    std::shared_ptr<mir::graphics::Cursor> const& cursor, std::shared_ptr<mir::options::Option> const& options) :
    repeat_rate_{options->get<int>(mir::options::key_repeat_rate_opt)},
    repeat_delay_{options->get<int>(mir::options::key_repeat_delay_opt)},
    cursor{cursor},
    cursor_scale{static_cast<float>(options->get<double>(mir::options::cursor_scale_override_opt))}
{
    if(cursor_scale != 1.0f)
        cursor_scale_changed(cursor_scale);
}

void mir::shell::AccessibilityManager::register_keyboard_helper(std::shared_ptr<KeyboardHelper> const& helper)
{
    // Update the keyboard help's rate and delay in case they changed before it
    // registered
    helper->repeat_info_changed(repeat_rate(), repeat_delay());

    keyboard_helpers.push_back(helper);
}

std::optional<int> mir::shell::AccessibilityManager::repeat_rate() const
{
    if (!enable_key_repeat)
        return {};
    return repeat_rate_;
}

int mir::shell::AccessibilityManager::repeat_delay() const
{
    return repeat_delay_;
}

void mir::shell::AccessibilityManager::repeat_rate(int new_rate)
{
    repeat_rate_ = new_rate;
}

void mir::shell::AccessibilityManager::repeat_delay(int new_delay)
{
    repeat_delay_ = new_delay;
}

void mir::shell::AccessibilityManager::override_key_repeat_settings(bool enable)
{
    enable_key_repeat = enable;
}

void mir::shell::AccessibilityManager::notify_helpers() const
{
    for (auto const& helper : keyboard_helpers)
        helper->repeat_info_changed(repeat_rate(), repeat_delay());
}

void mir::shell::AccessibilityManager::cursor_scale_changed(float new_scale)
{
    // Store the cursor scale in case the cursor hasn't been set yet
    cursor_scale = new_scale;

    if(cursor)
        cursor->set_scale(new_scale);
}

