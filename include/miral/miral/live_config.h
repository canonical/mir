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

#ifndef MIRAL_LIVE_CONFIG_H
#define MIRAL_LIVE_CONFIG_H

#include <initializer_list>
#include <string>
#include <memory>
#include <vector>
#include <optional>

namespace miral::live_config
{
/// Registration key for a configuration property. The key is essentially a tuple of
/// identifiers.
/// To simplify mapping these identifiers to multiple configuration backends, each identifier
/// is non-empty, starts with [a..z], and contains only [a..z,0..9,_]
class Key
{
public:
    Key(std::initializer_list<std::string_view const> key);

    auto as_path() const -> std::vector<std::string>;

    auto to_string() const -> std::string;
    auto operator<=>(Key const& that) const -> std::strong_ordering;

private:
   struct State;
   std::shared_ptr<State> self;
};

}

#endif //MIRAL_LIVE_CONFIG_H
