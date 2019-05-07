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
#include "utils.h"

#include <libxml++/libxml++.h>

Method::Method(xmlpp::Element const& node, std::string const& class_name, bool is_event)
    : name{node.get_attribute_value("name")},
      class_name{class_name},
      min_version{get_since_version(node)}
{
    for (auto const& child : node.get_children("arg"))
    {
        auto arg_node = dynamic_cast<xmlpp::Element const*>(child);
        arguments.emplace_back(std::ref(*arg_node), is_event);
    }
}

Emitter Method::types_str() const
{
    std::vector<Emitter> contents;

    if (min_version > 0)
        contents.push_back(std::to_string(min_version));

    for (auto const& arg : arguments)
        contents.push_back(arg.type_str_fragment());

    return {"\"", Emitter{contents}, "\""};
}

Emitter Method::types_declare() const
{
    if (use_null_types())
        return nullptr;

    std::vector<Emitter> types;

    for (auto const& arg : arguments)
    {
        Emitter e = arg.object_type_fragment();
        if (e.is_valid())
            types.push_back(e);
        else
            types.push_back("nullptr");
    }

    return {"static struct wl_interface const* ", name, "_types[];"};
}

Emitter Method::types_init() const
{
    if (use_null_types())
        return nullptr;

    std::vector<Emitter> types_vec, declares_vec;

    for (auto const& arg : arguments)
    {
        Emitter e = arg.object_type_fragment();
        if (e.is_valid())
            types_vec.push_back(e);
        else
            types_vec.push_back("nullptr");
    }

    Emitter declares = Lines{declares_vec};

    if (declares.is_valid())
    {
        declares = Lines{
            "// forward declarations of needed types",
            declares,
            empty_line
        };
    }

    return Lines{
        declares,
        {"struct wl_interface const* mw::", class_name, "::Thunks::", name, "_types[] ",
            BraceList{types_vec}}
    };
}

Emitter Method::wl_message_init() const
{
    return {"{\"", name, "\", ", types_str(),  ", ", (use_null_types() ? "all_null_types" : name + "_types"), "}"};
}

void Method::populate_required_interfaces(std::set<std::string>& interfaces) const
{
    for (auto const& arg : arguments)
    {
        arg.populate_required_interfaces(interfaces);
    }
}

int Method::get_since_version(xmlpp::Element const& node)
{
    try
    {
        return std::stoi(node.get_attribute_value("since"));
    }
    catch (std::invalid_argument const&)
    {
        return 0;
    }
}

bool Method::use_null_types() const
{
    if (arguments.size() > all_null_types_size)
        return false;

    for (auto const& i : arguments)
    {
        if (i.object_type_fragment().is_valid())
            return false;
    }

    return true;
}
