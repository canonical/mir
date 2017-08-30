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

#include <libxml++/libxml++.h>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include <experimental/optional>
#include <vector>
#include <locale>
#include <stdio.h>

void emit_required_headers()
{
    std::cout << "#include <experimental/optional>" << std::endl;
    std::cout << "#include <boost/throw_exception.hpp>" << std::endl;
    std::cout << std::endl;
    std::cout << "#include <wayland-server.h>" << std::endl;
    std::cout << "#include <wayland-server-protocol.h>" << std::endl;
    std::cout << std::endl;
    std::cout << "#include \"mir/fd.h\"" << std::endl;
}

std::string strip_wl_prefix(std::string const& name)
{
    return name.substr(3);
}

std::string camel_case_string(std::string const& name)
{
    std::string camel_cased_name;
    camel_cased_name = std::string{std::toupper(name[0], std::locale("C"))} + name.substr(1);
    auto next_underscore_offset = name.find('_');
    while (next_underscore_offset != std::string::npos)
    {
        if (next_underscore_offset < camel_cased_name.length())
        {
            camel_cased_name = camel_cased_name.substr(0, next_underscore_offset) +
                               std::toupper(camel_cased_name[next_underscore_offset + 1], std::locale("C")) +
                               camel_cased_name.substr(next_underscore_offset + 2);
        }
        next_underscore_offset = camel_cased_name.find('_', next_underscore_offset);
    }
    return camel_cased_name;
}

struct ArgumentTypeDescriptor
{
    std::string cpp_type;
    std::string c_type;
    std::experimental::optional<std::vector<std::string>> converter;
};

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

class Interface;

class Argument
{
public:
    Argument(xmlpp::Element const& node)
        : name{node.get_attribute_value("name")},
          descriptor{parse_optional(node) ? optional_type_map.at(node.get_attribute_value("type"))
                                          : type_map.at(node.get_attribute_value("type"))}
    {
    }

    void emit_c_prototype(std::ostream& out) const
    {
        out << descriptor.c_type << " " << name;
    }
    void emit_cpp_prototype(std::ostream& out) const
    {
        out << descriptor.cpp_type << " " << name;
    }
    void emit_thunk_call_fragment(std::ostream& out) const
    {
        out << (descriptor.converter ? (name + "_resolved") : name);
    }

    void emit_thunk_converter(std::ostream& out, std::string const& indent) const
    {
        for (auto const& line : descriptor.converter.value_or(std::vector<std::string>{}))
        {
            std::string substituted_line = line;
            size_t substitution_pos = substituted_line.find("$NAME");
            while (substitution_pos != std::string::npos)
            {
                substituted_line = substituted_line.replace(substitution_pos, 5, name);
                substitution_pos = substituted_line.find("$NAME");
            }
            out << indent << substituted_line << std::endl;
        }
    }

private:
    std::string const name;
    ArgumentTypeDescriptor const& descriptor;
};

class Method
{
public:
    Method(xmlpp::Element const& node)
        : name{node.get_attribute_value("name")}
    {
        for (auto const& child : node.get_children("arg"))
        {
            auto arg_node = dynamic_cast<xmlpp::Element const*>(child);
            arguments.emplace_back(std::ref(*arg_node));
        }
    }

    // TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
    void emit_virtual_prototype(std::ostream& out, std::string const& indent, bool is_global) const
    {
        out << indent << "virtual void " << name << "(";
        if (is_global)
        {
            out << "struct wl_client* client, struct wl_resource* resource";
            if (!arguments.empty())
            {
                out << ", ";
            }
        }
        for (size_t i = 0 ; i < arguments.size() ; ++i)
        {
            arguments[i].emit_cpp_prototype(out);
            if (i != arguments.size() - 1)
            {
                out << ", ";
            }
        }
        out << ") = 0;" << std::endl;
    }

    // TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
    void emit_thunk(std::ostream& out, std::string const& indent,
                    std::string const& interface_type, bool is_global) const
    {
        out << indent <<  "static void " << name << "_thunk("
            << "struct wl_client*" << (is_global ? " client" : "")
            << ", struct wl_resource* resource";
        for (auto const& arg : arguments)
        {
            out << ", ";
            arg.emit_c_prototype(out);
        }
        out << ")" << std::endl;

        out << indent << "{" << std::endl;
        out << indent << "    auto me = reinterpret_cast<" << interface_type << "*>("
            << "wl_resource_get_user_data(resource));" << std::endl;
        for (auto const& arg : arguments)
        {
            arg.emit_thunk_converter(out, indent + "    ");
        }

        out << indent << "    me->" << name << "(";
        if (is_global)
        {
            out << "client, resource";
            if (!arguments.empty())
            {
                out << ", ";
            }
        }
        for (size_t i = 0; i < arguments.size(); ++i)
        {
            arguments[i].emit_thunk_call_fragment(out);
            if (i != arguments.size() - 1)
            {
                out << ", ";
            }
        }
        out << ");" << std::endl;

        out << indent << "}" << std::endl;
    }

    void emit_vtable_initialiser(std::ostream& out, std::string const& indent) const
    {
        out << indent << name << "_thunk," << std::endl;
    }

private:
    std::string const name;
    std::vector<Argument> arguments;
};

void emit_indented_lines(std::ostream& out, std::string const& indent,
                         std::initializer_list<std::initializer_list<std::string>> lines)
{
    for (auto const& line : lines)
    {
        out << indent;
        for (auto const& fragment : line)
        {
            out << fragment;
        }
        out << std::endl;
    }
}

class Interface
{
public:
    Interface(
        xmlpp::Element const& node,
        std::function<std::string(std::string)> const& name_transform,
        std::unordered_set<std::string> const& constructable_interfaces)
        : wl_name{node.get_attribute_value("name")},
          generated_name{name_transform(wl_name)},
          is_global{constructable_interfaces.count(wl_name) == 0}
    {
        for (auto method_node : node.get_children("request"))
        {
            auto method = dynamic_cast<xmlpp::Element*>(method_node);
            methods.emplace_back(std::ref(*method));
        }
    }

    void emit_constructor(std::ostream& out, std::string const& indent, bool has_vtable)
    {
        if (is_global)
        {
            emit_constructor_for_global(out, indent);
        }
        else
        {
            emit_constructor_for_regular(out, indent, has_vtable);
        }
    }

    void emit_bind(std::ostream& out, std::string const& indent, bool has_vtable)
    {
        emit_indented_lines(out, indent, {
            {"static void bind(struct wl_client* client, void* data, uint32_t version, uint32_t id)"},
            {"{"},
        });
        emit_indented_lines(out, indent + "    ", {
            {"auto me = reinterpret_cast<", generated_name, "*>(data);"},
            {"auto resource = wl_resource_create(client, &", wl_name, "_interface,"},
            {"                                   std::min(version, me->max_version), id);"},
            {"if (resource == nullptr)"},
            {"{"},
            {"    wl_client_post_no_memory(client);"},
            {"    BOOST_THROW_EXCEPTION((std::bad_alloc{}));"},
            {"}"},
        });
        if (has_vtable)
        {
            emit_indented_lines(out, indent + "    ",
                {{"wl_resource_set_implementation(resource, &vtable, me, nullptr);"}});
        }
        emit_indented_lines(out, indent, {
            {"}"}
        });
    }

    void emit_class(std::ostream& out)
    {
        out << "class " << generated_name << std::endl;
        out << "{" << std::endl;
        out << "protected:" << std::endl;

        emit_constructor(out, "    ", !methods.empty());
        out << "    virtual ~" << generated_name << "() = default;" << std::endl;
        out << std::endl;

        for (auto const& method : methods)
        {
            method.emit_virtual_prototype(out, "    ", is_global);
        }
        out << std::endl;

        if (!is_global)
        {
            emit_indented_lines(out, "    ", {
                { "struct wl_client* const client;" },
                { "struct wl_resource* const resource;"}
            });
            out << std::endl;
        }

        if (!methods.empty())
        {
            out << "private:" << std::endl;
        }

        for (auto const& method : methods)
        {
            method.emit_thunk(out, "    ", generated_name, is_global);
            out << std::endl;
        }

        if (is_global)
        {
            emit_bind(out, "    ", !methods.empty());
            out << std::endl;
            emit_indented_lines(out, "    ", {
                { "uint32_t const max_version;" }
            });
        }

        if (!methods.empty())
        {
            emit_indented_lines(out, "    ", {
                { "static struct ", wl_name, "_interface const vtable;" }
            });
        }

        out << "};" << std::endl;

        out << std::endl;

        if (!methods.empty())
        {
            out << "struct " << wl_name << "_interface const " << generated_name << "::vtable = {" << std::endl;
            for (auto const& method : methods)
            {
                method.emit_vtable_initialiser(out, "    ");
            }
            out << "};" << std::endl;
        }
    }

private:
    void emit_constructor_for_global(std::ostream& out, std::string const& indent)
    {
        out << indent << generated_name << "(struct wl_display* display, uint32_t max_version)" << std::endl;
        out << indent << "    : max_version{max_version}" << std::endl;
        out << indent << "{" << std::endl;
        out << indent << "    if (!wl_global_create(display, " << std::endl;
        out << indent << "                          &" << wl_name << "_interface, max_version," << std::endl;
        out << indent << "                          this, &" << generated_name << "::bind))" << std::endl;
        out << indent << "    {" << std::endl;
        out << indent << "        BOOST_THROW_EXCEPTION((std::runtime_error{\"Failed to export "
                      << wl_name << " interface\"}));" << std::endl;
        out << indent << "    }" << std::endl;
        out << indent << "}" << std::endl;
    }

    void emit_constructor_for_regular(std::ostream& out, std::string const& indent, bool has_vtable)
    {
        emit_indented_lines(out, indent, {
            { generated_name, "(struct wl_client* client, struct wl_resource* parent, uint32_t id)" },
            { "    : client{client}," },
            { "      resource{wl_resource_create(client, &", wl_name, "_interface, wl_resource_get_version(parent), id)}" },
            { "{" }
        });
        emit_indented_lines(out, indent + "    ", {
            { "if (resource == nullptr)" },
            { "{" },
            { "    wl_resource_post_no_memory(parent);" },
            { "    BOOST_THROW_EXCEPTION((std::bad_alloc{}));" },
            { "}" },
        });
        if (has_vtable)
        {
            emit_indented_lines(out, indent + "    ",
                {{ "wl_resource_set_implementation(resource, &vtable, this, nullptr);" }});
        }
        emit_indented_lines(out, indent, {
            { "}" }
        });
    }

    std::string const wl_name;
    std::string const generated_name;
    bool const is_global;
    std::vector<Method> methods;
};

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        exit(1);
    }

    std::string const prefix{argv[1]};

    auto name_transform = [prefix](std::string protocol_name)
    {
        std::string transformed_name = protocol_name;
        if (protocol_name.find(prefix) == 0)
        {
            transformed_name = protocol_name.substr(prefix.length());
        }
        return camel_case_string(transformed_name);
    };

    xmlpp::DomParser parser(argv[2]);

    auto document = parser.get_document();

    auto root_node = document->get_root_node();

    auto constructor_nodes = root_node->find("//arg[@type='new_id']");
    std::unordered_set<std::string> constructible_interfaces;
    for (auto const node : constructor_nodes)
    {
        auto arg = dynamic_cast<xmlpp::Element const*>(node);
        constructible_interfaces.insert(arg->get_attribute_value("interface"));
    }

    emit_required_headers();

    std::cout << std::endl;

    std::cout << "namespace mir" << std::endl;
    std::cout << "{" << std::endl;
    std::cout << "namespace frontend" << std::endl;
    std::cout << "{" << std::endl;
    std::cout << "namespace wayland" << std::endl;
    std::cout << "{" << std::endl;

    for (auto top_level : root_node->get_children("interface"))
    {
        auto interface = dynamic_cast<xmlpp::Element*>(top_level);

        if (interface->get_attribute_value("name") == "wl_display" ||
            interface->get_attribute_value("name") == "wl_registry")
        {
            // These are special, and don't need binding.
            continue;
        }
        Interface(*interface, name_transform, constructible_interfaces).emit_class(std::cout);

        std::cout << std::endl << std::endl;
    }
    std::cout << "}" << std::endl;
    std::cout << "}" << std::endl;
    std::cout << "}" << std::endl;
    return 0;
}
