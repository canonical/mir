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

#ifndef MIR_SHELL_ACCESSIBILITY_MANAGER_H
#define MIR_SHELL_ACCESSIBILITY_MANAGER_H

#include <memory>
#include <optional>
#include <vector>

namespace mir
{
namespace graphics
{
class Cursor;
}
namespace shell
{
class KeyboardHelper;
class AccessibilityManager
{
public:
    void register_keyboard_helper(std::shared_ptr<shell::KeyboardHelper> const&);

    std::optional<int> repeat_rate() const;
    int repeat_delay() const;

    void repeat_rate(int new_rate);
    void repeat_delay(int new_rate);
    void override_key_repeat_settings(bool enable);

    void notify_helpers() const;

    void set_cursor(std::shared_ptr<graphics::Cursor> const& cursor);
    void cursor_scale_changed(float new_scale);

private:
    std::vector<std::shared_ptr<shell::KeyboardHelper>> keyboard_helpers;

    // 25 rate and 600 delay are the default in Weston and Sway
    int repeat_rate_{25};
    int repeat_delay_{600};
    bool enable_key_repeat{true};

    std::shared_ptr<graphics::Cursor> cursor;
    float cursor_scale{1};
};
}
}

#endif
