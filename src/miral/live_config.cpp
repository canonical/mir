/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/live_config.h"

#include <format>
#include <stdexcept>

struct miral::live_config::Key::State
{
    std::vector<std::string> const path;
    std::string const key{to_string()};

    State(std::initializer_list<std::string_view const> key) :
        path{key.begin(), key.end()}
    {
        if (key.begin() == key.end())
            throw std::invalid_argument{std::format("Invalid (empty) live config key")};

        for (auto const &element : key)
        {
            if (element.empty())
                throw std::invalid_argument{std::format("Invalid config key element: {}", element)};

            if (!islower(element[0]))
                throw std::invalid_argument{std::format("Invalid config key element: {}", element)};

            for (auto const &c : element)
                if (!islower(c) && !isdigit(c) && c != '_')
                    throw std::invalid_argument{std::format("Invalid config key element: {}", element)};
        }
    }

    auto operator<=>(State const& that) const { return key <=> that.key; }

    auto to_string() const -> std::string
    {
        std::string result;
        for (auto const& element : path)
        {
            if (!result.empty())
                result += '_';

            result += element;
        }

        return result;
    }
};


miral::live_config::Key::Key(std::initializer_list<std::string_view const> key) :
    self{std::make_unique<State>(key)}
{
}

auto miral::live_config::Key::as_path() const -> std::vector<std::string>
{
    return self->path;
}

auto miral::live_config::Key::to_string() const -> std::string
{
    return self->key;
}

auto miral::live_config::Key::operator<=>(Key const& that) const -> std::strong_ordering
{
    return *self <=> *that.self;
}

