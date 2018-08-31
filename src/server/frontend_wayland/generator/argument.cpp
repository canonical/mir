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

namespace
{

Emitter fd_wl2mir(std::string name)
{
    return Line{"mir::Fd ", name, "_resolved{", name, "};"};
}

Emitter fixed_wl2mir(std::string name)
{
    return Line{"double ", name, "_resolved{wl_fixed_to_double(", name, ")};"};
}

Emitter fixed_mir2wl(std::string name)
{
    return Line{"wl_fixed_t ", name, "_resolved{wl_fixed_from_double(", name, ")};"};
}

Emitter string_mir2wl(std::string name)
{
    return Lines{
        {"const char* ", name, "_resolved = ", name, ".c_str();"}
    };
}

Emitter optional_object_wl2mir(std::string name)
{
    return Lines{
        {"std::experimental::optional<struct wl_resource*> ", name, "_resolved;"},
        {"if (", name, " != nullptr)"},
        Block{
            {name, "_resolved = {", name, "};"}
        }
    };
}

Emitter optional_object_mir2wl(std::string name)
{
    return Lines{
        {"struct wl_resource* ", name, "_resolved = nullptr;"},
        {"if (", name, ")"},
        Block{
            {name, "_resolved = ", name, ".value();"}
        }
    };
}

Emitter optional_string_wl2mir(std::string name)
{
    return Lines{
        {"std::experimental::optional<std::string> ", name, "_resolved;"},
        {"if (", name, " != nullptr)"},
        Block{
            {name, "_resolved = {", name, "};"}
        }
    };
}

Emitter optional_string_mir2wl(std::string name)
{
    return Lines{
        {"const char* ", name, "_resolved = nullptr;"},
        {"if (", name, ")"},
        Block{
            {name, "_resolved = ", name, ".value().c_str();"}
        }
    };
}

std::unordered_map<std::string, Argument::TypeDescriptor const> const request_type_map = {
    { "uint", { "uint32_t", "uint32_t", {} }},
    { "int", { "int32_t", "int32_t", {} }},
    { "fd", { "mir::Fd", "int", { fd_wl2mir } }},
    { "object", { "struct wl_resource*", "struct wl_resource*", {} }},
    { "string", { "std::string const&", "char const*", {} }},
    { "new_id", { "uint32_t", "uint32_t", {} }},
    { "fixed", { "double", "wl_fixed_t", { fixed_wl2mir } }},
    { "array", { "struct wl_array*", "struct wl_array*", {} }}
};

std::unordered_map<std::string, Argument::TypeDescriptor const> const request_optional_type_map = {
    { "object", { "std::experimental::optional<struct wl_resource*> const&", "struct wl_resource*", { optional_object_wl2mir } }},
    { "string", { "std::experimental::optional<std::string> const&", "char const*", { optional_string_wl2mir } }},
};

std::unordered_map<std::string, Argument::TypeDescriptor const> const event_type_map = {
    { "uint", { "uint32_t", "uint32_t", {} }},
    { "int", { "int32_t", "int32_t", {} }},
    { "fd", { "mir::Fd", "int", { /* converted implicitly */ } }},
    { "object", { "struct wl_resource*", "struct wl_resource*", {} }},
    { "string", { "std::string const&", "char const*", { string_mir2wl } }},
    { "new_id", { "struct wl_resource*", "struct wl_resource*", {} }},
    { "fixed", { "double", "wl_fixed_t", { fixed_mir2wl } }},
    { "array", { "struct wl_array*", "struct wl_array*", {} }}
};

std::unordered_map<std::string, Argument::TypeDescriptor const> const event_optional_type_map = {
    { "object", { "std::experimental::optional<struct wl_resource*> const&", "struct wl_resource*", {optional_object_mir2wl} }},
    { "string", { "std::experimental::optional<std::string> const&", "char const*", { optional_string_mir2wl } }},
};
}

Argument::Argument(xmlpp::Element const& node, bool is_event)
    : name{sanitize_name(node.get_attribute_value("name"))},
      descriptor{get_type(node, is_event)}
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

Emitter Argument::call_fragment() const
{
    return descriptor.converter ? (name + "_resolved") : name;
}

std::experimental::optional<Emitter> Argument::converter() const
{
    if (descriptor.converter)
        return descriptor.converter.value()(name);
    else
        return std::experimental::nullopt;
}

Argument::TypeDescriptor Argument::get_type(xmlpp::Element const& node, bool is_event)
{
    bool is_optional = false;
    if (auto allow_null = node.get_attribute("allow-null"))
        is_optional = (allow_null->get_value() == "true");
    std::string type = node.get_attribute_value("type");

    try
    {
        if (is_optional)
        {
            if (is_event)
            {
                return event_optional_type_map.at(type);
            }
            else
            {
                return request_optional_type_map.at(type);
            }
        }
        else
        {
            if (is_event)
            {
                return event_type_map.at(type);
            }
            else
            {
                return request_type_map.at(type);
            }
        }
    }
    catch (std::out_of_range const&)
    {
        // its an error message, so who cares about the memory leak?
        return TypeDescriptor{"unknown_type_" + type, "unknown_type_" + type,
            {[=](std::string name) -> Emitter
            {
                return Lines{
                    {"#error '", name, "' is of type '" + type + "' (which is unknown for a", is_event ? "n event" : " request", is_optional ? " optional" : "", ")"},
                    {"#error type resolving code can be found in argument.cpp in the wayland scanner"}
                };
            }}};
    }
}
