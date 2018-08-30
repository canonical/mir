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
#include "utils.h"

#include <libxml++/libxml++.h>

#include <unordered_map>

std::unordered_map<std::string, Argument::TypeDescriptor const> const Argument::type_map = {
    { "uint", { "uint32_t", "uint32_t", {} }},
    { "int", { "int32_t", "int32_t", {} }},
    { "fd", { "mir::Fd", "int", { Argument::fd_converter }}},
    { "object", { "struct wl_resource*", "struct wl_resource*", {} }},
    { "string", { "std::string const&", "char const*", {} }},
    { "new_id", { "uint32_t", "uint32_t", {} }}
};

std::unordered_map<std::string, Argument::TypeDescriptor const> const Argument::optional_type_map = {
    { "object", { "std::experimental::optional<struct wl_resource*> const&", "struct wl_resource*", { Argument::optional_object_converter }}},
    { "string", { "std::experimental::optional<std::string> const&", "char const*", { Argument::optional_string_converter }}},
};

Argument::Argument(xmlpp::Element const& node)
    : name{sanitize_name(node.get_attribute_value("name"))},
      descriptor{argument_is_optional(node) ?
          optional_type_map.at(node.get_attribute_value("type")) :
          type_map.at(node.get_attribute_value("type"))}
{
}

Emitter Argument::wl_prototype() const
{
    return {descriptor.wl_type, " ", name};
}

Emitter Argument::mir_prototype() const
{
    return {descriptor.mir_type, " ", name};
}

Emitter Argument::mir_call_fragment() const
{
    return descriptor.converter ? (name + "_resolved") : name;
}

std::experimental::optional<Emitter> Argument::converter() const
{
    if (descriptor.converter)
        return descriptor.converter.value()(name);
    else
        return {};
}

bool Argument::argument_is_optional(xmlpp::Element const& arg)
{
    if (auto allow_null = arg.get_attribute("allow-null"))
    {
        return allow_null->get_value() == "true";
    }
    return false;
}

Emitter Argument::fd_converter(std::string name)
{
    return Line{"mir::Fd ", name, "_resolved{", name, "};"};
}

Emitter Argument::optional_object_converter(std::string name)
{
    return Lines{
        {"std::experimental::optional<struct wl_resource*> ", name, "_resolved;"},
        {"if (", name, " != nullptr)"},
        Block{
            {name, "_resolved = {", name, "};"}
        }
    };
}

Emitter Argument::optional_string_converter(std::string name)
{
    return Lines{
        {"std::experimental::optional<std::string> ", name, "_resolved;"},
        {"if (", name, " != nullptr)"},
        Block{
            {name, "_resolved = {", name, "};"}
        }
    };

}
