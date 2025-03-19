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
#include <functional>
#include <initializer_list>
#include <memory>
#include <unordered_map>

struct mir::input::MouseKeysKeymap::Self
{
    Self() = default;

    Self(std::initializer_list<std::pair<XkbSymkey, Action>> keymap)
    {
        for(auto const& [symkey, action]: keymap)
            set_action(symkey, action);
    }

    void set_action(XkbSymkey key, std::optional<Action> action)
    {
        if (!action)
        {
            keymap.erase(key);
            return;
        }

        keymap.insert_or_assign(key, *action);
    }

    std::optional<Action> get_action(XkbSymkey key) const
    {
        if(!keymap.contains(key))
            return {};

        return keymap.at(key);
    }

    void for_each_key_action_pair(std::function<void(XkbSymkey, Action)>&& f) const
    {
        for (auto const& [key, action] : keymap)
            f(key, action);
    }

    std::unordered_map<XkbSymkey, Action> keymap;
};

mir::input::MouseKeysKeymap::MouseKeysKeymap()
    : self{std::make_shared<Self>()}
{
}

mir::input::MouseKeysKeymap::MouseKeysKeymap(std::initializer_list<std::pair<XkbSymkey, Action>> keymap) :
    self{std::make_shared<Self>(keymap)}
{
}

void mir::input::MouseKeysKeymap::set_action(XkbSymkey key, std::optional<Action> action)
{
    self->set_action(key, action);
}

std::optional<mir::input::MouseKeysKeymap::Action> mir::input::MouseKeysKeymap::get_action(XkbSymkey key) const
{
    return self->get_action(key);
}

void mir::input::MouseKeysKeymap::for_each_key_action_pair(std::function<void(XkbSymkey, Action)>&& f) const
{
    self->for_each_key_action_pair(std::move(f));
}
