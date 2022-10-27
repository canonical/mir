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
 */

#include "argument.h"
#include "utils.h"

#include <libxml++/libxml++.h>

#include <unordered_map>

namespace
{

Emitter fd_wl2mir(Argument const* me)
{
    return Line{"mir::Fd ", me->name, "_resolved{", me->name, "};"};
}

Emitter fd_mir2wl(Argument const* me)
{
    return Line{"int32_t ", me->name, "_resolved{", me->name, "};"};
}

Emitter new_id_wl2mir(Argument const* me)
{
    return Lines{
        {"wl_resource* ", me->name, "_resolved{"},
        {Emitter::layout({
            "wl_resource_create(client, ", me->object_type_fragment(), ", wl_resource_get_version(resource), ", me->name, ")"
        }, true, false, Emitter::single_indent), "};"},
        {"if (", me->name, "_resolved == nullptr)"},
        Block{
            "wl_client_post_no_memory(client);",
            "BOOST_THROW_EXCEPTION((std::bad_alloc{}));",
        }
    };
}

Emitter fixed_wl2mir(Argument const* me)
{
    return Line{"double ", me->name, "_resolved{wl_fixed_to_double(", me->name, ")};"};
}

Emitter fixed_mir2wl(Argument const* me)
{
    return Line{"wl_fixed_t ", me->name, "_resolved{wl_fixed_from_double(", me->name, ")};"};
}

Emitter string_mir2wl(Argument const* me)
{
    return Lines{
        {"const char* ", me->name, "_resolved = ", me->name, ".c_str();"}
    };
}

Emitter optional_object_wl2mir(Argument const* me)
{
    return Lines{
        {"std::optional<struct wl_resource*> ", me->name, "_resolved;"},
        {"if (", me->name, " != nullptr)"},
        Block{
            {me->name, "_resolved = {", me->name, "};"}
        }
    };
}

Emitter optional_object_mir2wl(Argument const* me)
{
    return Lines{
        {"struct wl_resource* ", me->name, "_resolved = nullptr;"},
        {"if (", me->name, ")"},
        Block{
            {me->name, "_resolved = ", me->name, ".value();"}
        }
    };
}

Emitter optional_string_wl2mir(Argument const* me)
{
    return Lines{
        {"std::optional<std::string> ", me->name, "_resolved;"},
        {"if (", me->name, " != nullptr)"},
        Block{
            {me->name, "_resolved = {", me->name, "};"}
        }
    };
}

Emitter optional_string_mir2wl(Argument const* me)
{
    return Lines{
        {"const char* ", me->name, "_resolved = nullptr;"},
        {"if (", me->name, ")"},
        Block{
            {me->name, "_resolved = ", me->name, ".value().c_str();"}
        }
    };
}

std::unordered_map<std::string, Argument::TypeDescriptor const> const request_type_map = {
    { "uint", { "uint32_t", "uint32_t", "u", {} }},
    { "int", { "int32_t", "int32_t", "i", {} }},
    { "fd", { "mir::Fd", "int32_t", "h", { fd_wl2mir } }},
    { "object", { "struct wl_resource*", "struct wl_resource*", "o", {} }},
    { "string", { "std::string const&", "char const*", "s", {} }},
    { "new_id", { "struct wl_resource*", "uint32_t", "n", {new_id_wl2mir} }},
    { "fixed", { "double", "wl_fixed_t", "f", { fixed_wl2mir } }},
    { "array", { "struct wl_array*", "struct wl_array*", "a", {} }}
};

std::unordered_map<std::string, Argument::TypeDescriptor const> const request_optional_type_map = {
    { "object", { "std::optional<struct wl_resource*> const&", "struct wl_resource*", "?o", { optional_object_wl2mir } }},
    { "string", { "std::optional<std::string> const&", "char const*",  "?s",{ optional_string_wl2mir } }},
};

std::unordered_map<std::string, Argument::TypeDescriptor const> const event_type_map = {
    { "uint", { "uint32_t", "uint32_t", "u", {} }},
    { "int", { "int32_t", "int32_t", "i", {} }},
    { "fd", { "mir::Fd", "int32_t", "h", { fd_mir2wl } }},
    { "object", { "struct wl_resource*", "struct wl_resource*", "o", {} }},
    { "string", { "std::string const&", "char const*", "s", { string_mir2wl } }},
    { "new_id", { "struct wl_resource*", "struct wl_resource*", "n", {} }},
    { "fixed", { "double", "wl_fixed_t", "f", { fixed_mir2wl } }},
    { "array", { "struct wl_array*", "struct wl_array*", "a", {} }}
};

std::unordered_map<std::string, Argument::TypeDescriptor const> const event_optional_type_map = {
    { "object", { "std::optional<struct wl_resource*> const&", "struct wl_resource*", "?o", {optional_object_mir2wl} }},
    { "string", { "std::optional<std::string> const&", "char const*", "?s", { optional_string_mir2wl } }},
};
}

Argument::Argument(xmlpp::Element const& node, bool is_event)
    : name{sanitize_name(node.get_attribute_value("name"))},
      interface{get_interface(node)},
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

Emitter Argument::object_type_fragment() const
{
    if (interface)
        return {"&", interface.value(), "_interface"};
    else
        return nullptr;
}

Emitter Argument::type_str_fragment() const
{
    return descriptor.wl_type_abbr;
}

std::optional<Emitter> Argument::converter() const
{
    if (descriptor.converter)
        return descriptor.converter.value()(this);
    else
        return std::nullopt;
}

void Argument::populate_required_interfaces(std::set<std::string>& interfaces) const
{
    if (interface)
        interfaces.insert(interface.value());
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
        return TypeDescriptor{"unknown_type_" + type, "unknown_type_" + type, "!",
            {[=](Argument const* me) -> Emitter
            {
                return Lines{
                    {"#error '", me->name, "' is of type '" + type + "' (which is unknown for a", is_event ? "n event" : " request", is_optional ? " optional" : "", ")"},
                    {"#error type resolving code can be found in argument.cpp in the wayland scanner"}
                };
            }}};
    }
}

std::optional<std::string> Argument::get_interface(xmlpp::Element const& node)
{
    std::string ret = node.get_attribute_value("interface");
    if (ret.empty())
        return std::nullopt;
    else
        return {ret};
}
