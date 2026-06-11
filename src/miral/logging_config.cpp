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

#include <miral/logging_config.h>
#include <miral/live_config.h>
#include <mir/logging/tag.h>

#include <boost/throw_exception.hpp>
#include <format>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace
{
auto split_assignment(std::string_view config_value)
    -> std::pair<std::string_view, std::string_view>
{
    auto equals_pos = config_value.find('=');
    if (equals_pos == config_value.npos)
    {
      BOOST_THROW_EXCEPTION((std::runtime_error{
          std::format("Failed to parse {} as key=value pair", config_value)}));
    }

    auto const key = config_value.substr(0, equals_pos);
    auto const value = config_value.substr(equals_pos + 1);

    if (key.empty() || value.empty())
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            std::format("Failed to parse {} as key=value pair", config_value)}));
    }
    return std::make_pair(key, value);
}
}

void miral::register_log_filtering_config(live_config::Store &config_store)
{
    config_store.add_strings_attribute(
        {"log_level"},
        "Minimum severity level for a log message with a given tag to be emitted, in the form tag=severity",
        [](live_config::Key const& /*key*/, std::optional<std::span<std::string const>> value)
        {
            // Restore the default state before applying any new configuration
            mir::logging::tag::set_severity("base", mir::logging::Severity::warning);

            if (value.has_value())
            {

                for (auto const& filter : *value)
                {
                    auto const [tag_name, severity_name] = split_assignment(filter);
                    auto const severity = mir::logging::parse_severity(severity_name);
                    mir::logging::tag::set_severity(tag_name, severity);
                }
            }
        }
    );
}
