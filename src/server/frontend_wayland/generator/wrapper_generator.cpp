/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "emitter.h"
#include "interface.h"
#include "utils.h"

#include <libxml++/libxml++.h>
#include <iostream>
#include <fstream>

Emitter comment_header(std::string const& input_file_path)
{
    return Lines{
        "/*",
        " * AUTOGENERATED - DO NOT EDIT",
        " *",
        {" * This header is generated by wrapper_generator.cpp from ", file_name_from_path(input_file_path)},
        " * To regenerate, run the “refresh-wayland-wrapper” target.",
        " */",
    };
}

Emitter include_guard_top(std::string const& macro)
{
    return Lines{
        {"#ifndef ", macro},
        {"#define ", macro},
    };
}

Emitter required_headers(std::string const& custom_header)
{
    return Lines{
        "#include <experimental/optional>",
        "#include <boost/throw_exception.hpp>",
        "#include <boost/exception/diagnostic_information.hpp>",
        empty_line,
        {"#include \"", custom_header, "\""},
        empty_line,
        "#include \"mir/fd.h\"",
        "#include \"mir/log.h\"",
    };
}

Emitter include_guard_bottom(std::string const& macro)
{
    return Lines{
        {"#endif // ", macro}
    };
}

Emitter header_file(std::string custom_header, std::string input_file_path, std::vector<Interface> const& interfaces)
{
    std::string const include_guard_macro = to_upper_case("MIR_FRONTEND_WAYLAND_" + file_name_from_path(input_file_path) + "_WRAPPER");

    std::vector<Emitter> interface_emitters;
    for (auto const& interface : interfaces)
    {
        interface_emitters.push_back(interface.full_class());
    }

    return Lines{
        comment_header(input_file_path),
        empty_line,
        include_guard_top(include_guard_macro),
        empty_line,
        required_headers(custom_header),
        empty_line,
        "namespace mir",
        "{",
        "namespace frontend",
        "{",
        "namespace wayland",
        "{",
        empty_line,
        List{interface_emitters, empty_line},
        empty_line,
        "}",
        "}",
        "}",
        empty_line,
        include_guard_bottom(include_guard_macro)
    };
}

int main(int argc, char** argv)
{
    std::cerr << "wrapper generator started" << std::endl;

    if (argc != 5)
    {
        std::cerr << "wrong number of args" << std::endl;
        Emitter msg = Lines{
            "/*",
            "Incorrect number of arguments",
            {"Usage:", file_name_from_path(argv[0]), " prefix", " header", " input"},
            Block{
                "prefix: the name prefix which will be removed, such as wl_",
                "        to not use a prefix, use _ or anything that won't match the start of a name",
                "header: the C header to include, such as wayland-server.h",
                "input: the input xml file path",
            },
            "*/",
        };
        exit(1);
    }

    std::cerr << "right number of args" << std::endl;

    std::string const prefix{argv[1]};
    std::string const custom_header{argv[2]};
    std::string const input_file_path{argv[3]};
    std::string const output_file_path{argv[4]};

    auto name_transform = [prefix](std::string protocol_name)
    {
        std::string transformed_name = protocol_name;
        if (protocol_name.find(prefix) == 0) // if the first instance of prefix is at the start of protocol_name
        {
            // cut off the prefix
            transformed_name = protocol_name.substr(prefix.length());
        }
        return to_camel_case(transformed_name);
    };

    xmlpp::DomParser parser(input_file_path);

    std::cerr << "parser loaded" << std::endl;

    auto document = parser.get_document();

    auto root_node = document->get_root_node();

    auto constructor_nodes = root_node->find("//arg[@type='new_id']");
    std::unordered_set<std::string> constructible_interfaces;
    for (auto const node : constructor_nodes)
    {
        auto arg = dynamic_cast<xmlpp::Element const*>(node);
        constructible_interfaces.insert(arg->get_attribute_value("interface"));
    }

    std::vector<Interface> interfaces;
    for (auto top_level : root_node->get_children("interface"))
    {
        auto interface = dynamic_cast<xmlpp::Element*>(top_level);

        if (interface->get_attribute_value("name") == "wl_display" ||
            interface->get_attribute_value("name") == "wl_registry")
        {
            // These are special, and don't need binding.
            continue;
        }
        interfaces.emplace_back(*interface, name_transform, constructible_interfaces);
    }

    std::cerr << "xml parsed" << std::endl;

    auto header = header_file(custom_header, input_file_path, interfaces);

    std::cerr << "emitters generated" << std::endl;

    std::ofstream out_file{output_file_path};

    header.emit({out_file, std::make_shared<bool>(true), ""});

    std::cerr << "file written" << std::endl;
}
