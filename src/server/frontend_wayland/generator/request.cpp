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

#include "request.h"

Request::Request(xmlpp::Element const& node, std::string const& class_name, bool is_global)
    : Method{node, class_name, is_global}
{
}

// TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
Emitter Request::virtual_mir_prototype() const
{
    return {"virtual void ", name, "(", mir_args(), ") = 0;"};
}

// TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
Emitter Request::thunk_impl() const
{
    return {"static void ", name, "_thunk(", wl_args(), ")",
        Block{
            {"auto me = static_cast<", class_name, "*>(wl_resource_get_user_data(resource));"},
            wl2mir_converters(),
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
                    {"\"Exception processing ", class_name, "::", name, "() request\""}},
                    Line{{","}, false, true}, "           "},
                    ");"
            }}
        }
    };
}

Emitter Request::vtable_initialiser() const
{
    return {name, "_thunk"};
}

Emitter Request::wl_args() const
{
    Emitter client_arg = "struct wl_client*";
    if (is_global) // only bind it to a variable if we need it
        client_arg = {client_arg, " client"};
    std::vector<Emitter> wl_args{client_arg, "struct wl_resource* resource"};
    for (auto const& arg : arguments)
        wl_args.push_back(arg.wl_prototype());
    return List{wl_args, ", "};
}

Emitter Request::wl2mir_converters() const
{
    std::vector<Emitter> thunk_converters;
    for (auto const& arg : arguments)
    {
        if (auto converter = arg.wl2mir_converter())
            thunk_converters.push_back(converter.value());
    }
    return Lines{thunk_converters};
}

Emitter Request::mir_call_args() const
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
