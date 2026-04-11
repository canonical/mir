/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef MIR_GRAPHICS_COMMON_QUIRK_COMMON_H_
#define MIR_GRAPHICS_COMMON_QUIRK_COMMON_H_

#include <mir/log.h>

#include <mir/udev/wrapper.h>

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mir
{
namespace graphics
{
namespace common
{
struct AllowList
{
    AllowList(std::unordered_set<std::string>&& drivers_to_skip);
    void allow(std::string specifier, std::string specifier_value);
    void skip(std::string specifier, std::string specifier_value);

    std::unordered_set<std::string> skipped_drivers;
    std::unordered_set<std::string> skipped_devnodes;
};

struct ValuedOption
{
    void add(std::string_view specifier, std::string const& specifier_value, std::string const& chosen_value);

    std::unordered_map<std::string, std::string> drivers;
    std::unordered_map<std::string, std::string> devnodes;
};

struct OptionStructure
{
    static auto freeform(size_t expected_tokens) -> OptionStructure
    {
        return {
            .expected_token_count = expected_tokens,
            .check_specifiers = false,
        };
    }

    size_t const expected_token_count{3}; // option name, specifier, specifier value
    bool const  check_specifiers{true};
};

auto validate_structure(std::vector<std::string> const& tokens, std::unordered_map<std::string, OptionStructure> const& available_options)
    -> std::optional<std::tuple<std::string, std::string, std::string>>;

auto matches(std::vector<std::string> tokens, std::string option_name, std::initializer_list<std::string> valid_values)
    -> bool;

auto value_or(char const* maybe_null_string, char const* value_if_null) -> char const*;

auto get_device_driver(mir::udev::Device const* parent_device) -> char const*;

auto apply_quirk(
    char const* devnode,
    char const* driver,
    std::unordered_map<std::string, std::string> const& devnode_quirks,
    std::unordered_map<std::string, std::string> const& driver_quirks,
    char const* message) -> std::string;
}
}
}

#endif
