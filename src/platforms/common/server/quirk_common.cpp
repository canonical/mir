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

#include "quirk_common.h"

mir::graphics::common::AllowList::AllowList(std::unordered_set<std::string>&& drivers_to_skip) :
    skipped_drivers{std::move(drivers_to_skip)}
{
}

void mir::graphics::common::AllowList::allow(std::string specifier, std::string specifier_value)
{
    if (specifier == "devnode")
        skipped_devnodes.erase(specifier_value);
    else if (specifier == "driver")
        skipped_drivers.erase(specifier_value);
}

void mir::graphics::common::AllowList::skip(std::string specifier, std::string specifier_value)
{
    if (specifier == "devnode")
        skipped_devnodes.insert(specifier_value);
    else if (specifier == "driver")
        skipped_drivers.insert(specifier_value);
}

void mir::graphics::common::ValuedOption::add(
    std::string_view specifier, std::string const& specifier_value, std::string const& chosen_value)
{
    if (specifier == "devnode")
        devnodes.insert_or_assign(specifier_value, chosen_value);
    else if (specifier == "driver")
        drivers.insert_or_assign(specifier_value, chosen_value);
}


auto mir::graphics::common::validate_structure(
    std::vector<std::string> const& tokens, std::set<std::string> const& available_options)
    -> std::optional<std::tuple<std::string, std::string, std::string>>
{
    if (tokens.size() < 1)
        return {};
    auto const option = tokens[0];

    if (!available_options.contains(option))
        return {};

    if (tokens.size() < 3)
        return {};
    auto const specifier = tokens[1];
    auto const specifier_value = tokens[2];

    if (specifier != "driver" && specifier != "devnode")
        return {};

    return {{option, specifier, specifier_value}};
}

auto mir::graphics::common::matches(
    std::vector<std::string> tokens, std::string option_name, std::initializer_list<std::string> valid_values) -> bool
{
    if (tokens.size() < 1 || tokens[0] != option_name)
        return false;

    // Specifier and its value are already checked with `validate_structure`

    if (tokens.size() < 4 ||
        std::ranges::none_of(valid_values, [&tokens](auto valid_value) { return tokens[3] == valid_value; }))
        return false;

    return true;
}

auto mir::graphics::common::value_or(char const* maybe_null_string, char const* value_if_null) -> char const*
{
    if (maybe_null_string)
    {
        return maybe_null_string;
    }
    else
    {
        return value_if_null;
    }
}

auto mir::graphics::common::get_device_driver(mir::udev::Device const* parent_device) -> char const*
{
    if (parent_device)
    {
        return value_or(parent_device->driver(), "");
    }
    mir::log_warning("udev device has no parent! Unable to determine driver for quirks.");
    return "<UNKNOWN>";
}

auto mir::graphics::common::apply_quirk(
    std::string_view devnode,
    std::string_view driver,
    std::unordered_map<std::string, std::string> const& devnode_quirks,
    std::unordered_map<std::string, std::string> const& driver_quirks,
    std::string_view message) -> std::string
{
    if (auto p = devnode_quirks.find(std::string{devnode}); p != devnode_quirks.end())
    {
        mir::log_debug("Quirks(%s): forcing %s implementation", message.data(), p->second.c_str());
        return p->second;
    }

    if (auto p = driver_quirks.find(std::string{driver}); p != driver_quirks.end())
    {
        mir::log_debug("Quirks(%s): forcing %s implementation", message.data(), p->second.c_str());
        return p->second;
    }

    log_debug("Quirks(%s): using default implementation for %s driver", message.data(), driver.data());

    // Not specified
    return std::string{driver};
}
