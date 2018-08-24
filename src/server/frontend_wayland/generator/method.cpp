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

#include "method.h"

#include "wrapper_generator.h"

#include <libxml++/libxml++.h>

Method::Method(xmlpp::Element const& node)
    : name{node.get_attribute_value("name")}
{
    for (auto const& child : node.get_children("arg"))
    {
        auto arg_node = dynamic_cast<xmlpp::Element const*>(child);
        arguments.emplace_back(std::ref(*arg_node));
    }
}

// TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
Emitter Method::emit_virtual_prototype(bool is_global) const
{
    std::vector<Emitter> args;
    if (is_global)
    {
        args.push_back("struct wl_client* client");
        args.push_back("struct wl_resource* resource");
    }
    for (auto& i : arguments)
    {
        args.push_back(i.cpp_prototype());
    }

    return {"virtual void ", name, "(", List{args, ", "}, ") = 0;"};
}

// TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
Emitter Method::emit_thunk(std::string const& interface_type, bool is_global) const
{
    std::vector<Emitter> c_args{
        {"struct wl_client*", (is_global ? " client" : "")},
        "struct wl_resource* resource"};
        for (auto const& arg : arguments)
            c_args.push_back(arg.c_prototype());

        std::vector<Emitter> thunk_converters;
        for (auto const& arg : arguments)
        {
            if (auto converter = arg.thunk_converter())
                thunk_converters.push_back(converter.value());
        }

        std::vector<Emitter> call_args;
        if (is_global)
        {
            call_args.push_back("client");
            call_args.push_back("resource");
        }
        for (auto& arg : arguments)
            call_args.push_back(arg.thunk_call_fragment());

        return {"static void ", name, "_thunk(", List{c_args, ", "}, ")",
        Block{
            {"auto me = static_cast<", interface_type, "*>(wl_resource_get_user_data(resource));"},
            Lines{thunk_converters},
            "try",
            Block{
                {"me->", name, "(", List{call_args, ", "}, ");"}
            },
            "catch (...)",
            Block{
                {"::mir::log(",
                    List{{
                        "::mir::logging::Severity::critical",
                        "    \"frontend:Wayland\"",
                        "    std::current_exception()",
                        {"    \"Exception processing ", interface_type, "::", name, "() request\""}
                    }, ","},
                    ");"}
            }
        }
        };
}

Emitter Method::emit_vtable_initialiser() const
{
    return {name, "_thunk"};
}
