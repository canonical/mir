/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include "argument.h"

#include "wrapper_generator.h"

#include <libxml++/libxml++.h>

#include <unordered_map>

std::vector<std::string> fd_converter{
    "mir::Fd $NAME_resolved{$NAME};"
};

std::vector<std::string> optional_object_converter{
    "std::experimental::optional<struct wl_resource*> $NAME_resolved;",
    "if ($NAME != nullptr)",
    "{",
    "    $NAME_resolved = $NAME;",
    "}"
};

std::vector<std::string> optional_string_converter{
    "std::experimental::optional<std::string> $NAME_resolved;",
    "if ($NAME != nullptr)",
    "{",
    "    $NAME_resolved = std::experimental::make_optional<std::string>($NAME);",
    "}"
};

std::unordered_map<std::string, ArgumentTypeDescriptor const> type_map = {
    { "uint", { "uint32_t", "uint32_t", {} }},
    { "int", { "int32_t", "int32_t", {} }},
    { "fd", { "mir::Fd", "int", { fd_converter }}},
    { "object", { "struct wl_resource*", "struct wl_resource*", {} }},
    { "string", { "std::string const&", "char const*", {} }},
    { "new_id", { "uint32_t", "uint32_t", {} }}
};

std::unordered_map<std::string, ArgumentTypeDescriptor const> optional_type_map = {
    { "object", { "std::experimental::optional<struct wl_resource*> const&", "struct wl_resource*", { optional_object_converter }}},
    { "string", { "std::experimental::optional<std::string> const&", "char const*", { optional_string_converter} }},
};

bool parse_optional(xmlpp::Element const& arg)
{
    if (auto allow_null = arg.get_attribute("allow-null"))
    {
        return allow_null->get_value() == "true";
    }
    return false;
}

Argument::Argument(xmlpp::Element const& node)
    : name{sanitize_name(node.get_attribute_value("name"))},
      descriptor{parse_optional(node) ?
          optional_type_map.at(node.get_attribute_value("type")) :
          type_map.at(node.get_attribute_value("type"))}
{
}

Emitter Argument::emit_c_prototype() const
{
    return {descriptor.c_type, " ", name};
}

Emitter Argument::emit_cpp_prototype() const
{
    return {descriptor.cpp_type, " ", name};
}

Emitter Argument::emit_thunk_call_fragment() const
{
    return descriptor.converter ? (name + "_resolved") : name;
}

Emitter Argument::emit_thunk_converter() const
{
    std::vector<Emitter> substituted_lines;
    for (auto const& line : descriptor.converter.value_or(std::vector<std::string>{}))
    {
        std::string substituted_line = line;
        size_t substitution_pos = substituted_line.find("$NAME");
        while (substitution_pos != std::string::npos)
        {
            substituted_line = substituted_line.replace(substitution_pos, 5, name);
            substitution_pos = substituted_line.find("$NAME");
        }
        if (!substituted_line.empty())
            substituted_lines.push_back(substituted_line);
    }
    return Lines{substituted_lines};
}
