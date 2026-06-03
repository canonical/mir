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

#ifndef MIR_LOGGING_TAG_H_
#define MIR_LOGGING_TAG_H_

#include <format>
#include <functional>
#include <initializer_list>
#include <string_view>
#include <vector>

namespace mir::logging
{
struct Tag;
/**
 * Create a tag for use in logging
 *
 * This creates a new tag and registers it with the logging subsystem,
 * with lifetime of the whole process. It should be called exactly once
 * for each desired tag and the reference it returns stored.
 *
 * Valid tag names consist of lowercase ASCII ('a'-'z'), '_', or '-' characters.
 *
 * For leaf tags — tags not used as a `parent` or which are only used in a single
 * translation unit, it is suggested that this be called at global scope
 * so that the tag is registered with the logging subsystem during
 * server initialisation. Eg:
 *
 * `static Tag const& bypass = mir::logging::create_tag(logging::graphics(), "bypass");`
 */
Tag const& create_tag(Tag const& parent, std::string_view name);

Tag const& base();
Tag const& input();
Tag const& wayland();
Tag const& graphics();
Tag const& window_management();

using Tags = std::initializer_list<std::reference_wrapper<Tag const> const>;

enum class Severity
{
    critical = 0,
    error = 1,
    warning = 2,
    informational = 3,
    debug = 4
};

auto parse_severity(std::string_view severity_name) -> Severity;

auto logging_enabled_for(Tag const& tag, Severity sev) -> bool;

auto list_known_tags() -> std::vector<std::string>;

namespace tag
{
auto name(Tag const& tag) -> std::string_view;

void set_severity(std::string_view name, Severity severity);
}
}

template<>
struct std::formatter<mir::logging::Severity> : std::formatter<std::string>
{
    auto format(mir::logging::Severity sev, std::format_context& ctx) const
        -> std::format_context::iterator;
};


#endif // MIR_LOGGING_TAG_H_
