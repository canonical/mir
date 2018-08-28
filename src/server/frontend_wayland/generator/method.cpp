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

#include <libxml++/libxml++.h>

Method::Method(xmlpp::Element const& node, bool is_global)
    : name{node.get_attribute_value("name")},
      is_global{is_global}
{
    for (auto const& child : node.get_children("arg"))
    {
        auto arg_node = dynamic_cast<xmlpp::Element const*>(child);
        arguments.emplace_back(std::ref(*arg_node));
    }
}

// TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
Emitter Method::virtual_mir_prototype() const
{
    return {"virtual void ", name, "(", mir_args(), ") = 0;"};
}

// TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
Emitter Method::thunk_body(std::string const& interface_type) const
{
    return {"static void ", name, "_thunk(", wl_args(), ")",
        Block{
            {"auto me = static_cast<", interface_type, "*>(wl_resource_get_user_data(resource));"},
            converters(),
            "try",
            Block{
                {"me->", name, "(", mir_call_args(), ");"}
            },
            "catch(...)",
            Block{{
                "::mir::log(",
                    List{{"::mir::logging::Severity::critical",
                          "\"frontend:Wayland\"",
                          "std::current_exception()",
                          {"\"Exception processing ", interface_type, "::", name, "() request\""}},
                        Line{{","}, false, true}, "           "},
                ");"
            }}
        }
    };
}

Emitter Method::vtable_initialiser() const
{
    return {name, "_thunk"};
}

Emitter Method::wl_args() const
{
    Emitter client_arg = "struct wl_client*";
    if (is_global) // only bind it to a variable if we need it
        client_arg = {client_arg, " client"};
    std::vector<Emitter> wl_args{client_arg, "struct wl_resource* resource"};
    for (auto const& arg : arguments)
        wl_args.push_back(arg.wl_prototype());
    return List{wl_args, ", "};
}

Emitter Method::mir_args() const
{
    std::vector<Emitter> mir_args;
    if (is_global)
    {
        mir_args.push_back("struct wl_client* client");
        mir_args.push_back("struct wl_resource* resource");
    }
    for (auto& i : arguments)
    {
        mir_args.push_back(i.mir_prototype());
    }
    return List{mir_args, ", "};
}

Emitter Method::converters() const
{
    std::vector<Emitter> thunk_converters;
    for (auto const& arg : arguments)
    {
        if (auto converter = arg.converter())
            thunk_converters.push_back(converter.value());
    }
    return Lines{thunk_converters};
}

Emitter Method::mir_call_args() const
{
    std::vector<Emitter> call_args;
    if (is_global)
    {
        call_args.push_back("client");
        call_args.push_back("resource");
    }
    for (auto& arg : arguments)
        call_args.push_back(arg.mir_call_fragment());
    return List{call_args, ", "};
}
