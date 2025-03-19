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

#ifndef MIR_MOUSE_KEYS_COMMON_
#define MIR_MOUSE_KEYS_COMMON_

#include <initializer_list>
#include <memory>
#include <optional>
#include <functional>


namespace mir
{
namespace input
{
using XkbSymkey = unsigned int;

/// Maps between XKB keysyms and mousekeys actions.
class MouseKeysKeymap
{
public:
    enum class Action
    {
        move_left,
        move_right,
        move_up,
        move_down,
        click,
        double_click,
        drag_start,
        drag_end,
        button_primary,
        button_secondary,
        button_tertiary
    };

    /// Default constructor, creates a keymap with no mapping set up.
    MouseKeysKeymap();

    /// Creates a keymap with only the specified mappings.
    MouseKeysKeymap(std::initializer_list<std::pair<XkbSymkey, Action>>);

    /// Sets up a mapping between a keysym and an action. Pass std::nullopt to
    /// disable an action.
    void set_action(XkbSymkey key, std::optional<Action> action);

    /// Returns the action corresponding to the passed keysym, if any.
    std::optional<Action> get_action(XkbSymkey key) const;

    /// Allows introspection of the current key-action mappings.
    void for_each_key_action_pair(std::function<void(XkbSymkey, Action)>&&) const;

private:
    struct Self;
    std::shared_ptr<Self> self;
};
}
}

#endif
