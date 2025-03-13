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


#include <unordered_map>

#ifndef MIR_INPUT_MOUSE_KEYS_COMMON_
#define MIR_INPUT_MOUSE_KEYS_COMMON_

namespace mir
{
namespace input
{
enum class MouseKeysAction
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

using XkbSymkey = unsigned int;
class MouseKeysKeymap : std::unordered_map<XkbSymkey, MouseKeysAction>
{
public:
    using std::unordered_map<XkbSymkey, MouseKeysAction>::unordered_map;
    using std::unordered_map<XkbSymkey, MouseKeysAction>::contains;
    using std::unordered_map<XkbSymkey, MouseKeysAction>::at;

    ~MouseKeysKeymap() = default;
};
}
}

#endif
