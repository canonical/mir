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

#include "mir/input/mousekeys_common.h"
#include <unordered_map>

struct mir::input::MouseKeysKeymap::Self: public std::unordered_map<XkbSymkey, Action>
{
    void set_action(XkbSymkey key, std::optional<Action> action)
    {
        if(!keymap.contains(key))
            return;

        if(!action)
            keymap.erase(key);

        keymap.insert_or_assign(key, *action);
    }

    std::optional<Action> get_action(XkbSymkey key)
    {
        if(keymap.contains(key))
            return {};

        return keymap.at(key);
    }

    std::unordered_map<XkbSymkey, Action> keymap;
};

void mir::input::MouseKeysKeymap::set_action(XkbSymkey key, std::optional<Action> action)
{
    self->set_action(key, action);
}

std::optional<mir::input::MouseKeysKeymap::Action> mir::input::MouseKeysKeymap::get_action(XkbSymkey key)
{
    return self->get_action(key);
}
