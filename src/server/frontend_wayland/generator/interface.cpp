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

#include "interface.h"

#include <libxml++/libxml++.h>

#include "method.h"

Interface::Interface(xmlpp::Element const& node,
                     std::function<std::string(std::string)> const& name_transform,
                     std::unordered_set<std::string> const& constructable_interfaces)
    : wl_name{node.get_attribute_value("name")},
      generated_name{name_transform(wl_name)},
      nmspace{"mfw::" + generated_name + "::"},
      is_global{constructable_interfaces.count(wl_name) == 0},
      methods{get_methods(node, is_global)},
      has_vtable{!methods.empty()}
{
}

Emitter Interface::declaration() const
{
    return Lines{
        {"class ", generated_name},
        "{",
        "protected:",
        List {{
            Lines {
                constructor_prototype(),
                destructor_prototype(),
            },
            (is_global ? bind_prototype() : nullptr),
            virtual_method_prototypes(),
            member_vars(),
        }, empty_line, Emitter::single_indent},
        empty_line,
        "private:",
        List {{
            thunk_bodies(),
            (is_global ? bind_thunk() : nullptr),
            (has_vtable && !is_global ? resource_destroyed_thunk() : nullptr),
            (has_vtable ? vtable_declare() : nullptr),
        }, empty_line, Emitter::single_indent},
        "};"
    };
}

Emitter Interface::implementation() const
{
    return Lines{
        {"// ", generated_name},
        empty_line,
        List{{
            constructor_impl(),
            destructor_impl(),
            (has_vtable ? vtable_init() : nullptr),
        }, empty_line},
    };
}

Emitter Interface::constructor_prototype() const
{
    return Line{generated_name, "(", constructor_args(), ");"};
}

Emitter Interface::constructor_impl() const
{
    if (is_global)
    {
        return Lines{
            {nmspace, generated_name, "(", constructor_args(), ")"},
            {"    : global{wl_global_create(display, &", wl_name, "_interface, max_version, this, &", generated_name, "::bind_thunk)},"},
            {"      max_version{max_version}"},
            Block{
                "if (global == nullptr)",
                Block{
                    "BOOST_THROW_EXCEPTION((std::runtime_error{",
                    {"    \"Failed to export ", wl_name, " interface\"}));"}
                }
            }
        };
    }
    else
    {
        return Lines{
            {nmspace, generated_name, "(", constructor_args(), ")"},
            {"    : client{client},"},
            {"      resource{wl_resource_create(client, &", wl_name, "_interface, wl_resource_get_version(parent), id)}"},
            Block{
                "if (resource == nullptr)",
                Block{
                    "wl_resource_post_no_memory(parent);",
                    "BOOST_THROW_EXCEPTION((std::bad_alloc{}));",
                },
                (has_vtable ?
                "wl_resource_set_implementation(resource, &vtable, this, &resource_destroyed_thunk);" :
                Emitter{nullptr})
            }
        };
    }
}

Emitter Interface::constructor_args() const
{
    if (is_global)
    {
        return List{{"struct wl_display* display", "uint32_t max_version"},
                    ", "};
    }
    else
    {
        return List{{"struct wl_client* client", "struct wl_resource* parent", "uint32_t id"},
                    ", "};
    }
}

Emitter Interface::destructor_prototype() const
{
    return Line{"virtual ~", generated_name, "()", (is_global ? nullptr : " = default"), ";"};
}

Emitter Interface::destructor_impl() const
{
    if (is_global)
    {
        return Lines{
            {nmspace, "~", generated_name, "()"},
            Block{
                "wl_global_destroy(global);"
            }
        };
    }
    else
    {
        return nullptr;
    }
}

Emitter Interface::bind_prototype() const
{
    return "virtual void bind(struct wl_client* client, struct wl_resource* resource) { (void)client; (void)resource; }";
}

Emitter Interface::virtual_method_prototypes() const
{
    std::vector<Emitter> prototypes;
    for (auto const& method : methods)
    {
        prototypes.push_back(method.virtual_mir_prototype());
    }
    return Lines{prototypes};
}

Emitter Interface::member_vars() const
{
    if (is_global)
    {
        return Lines{
            {"struct wl_global* const global;"},
            {"uint32_t const max_version;"},
        };
    }
    else
    {
        return Lines{
            {"struct wl_client* const client;"},
            {"struct wl_resource* const resource;"}
        };
    }
}

Emitter Interface::thunk_bodies() const
{
    std::vector<Emitter> bodies;
    for (auto const& method : methods)
    {
        bodies.push_back(method.thunk_body(generated_name));
    }
    return Lines{bodies};
}

Emitter Interface::bind_thunk() const
{
    return Lines{
        "static void bind_thunk(struct wl_client* client, void* data, uint32_t version, uint32_t id)",
        Block{
            {"auto me = static_cast<", generated_name, "*>(data);"},
            {"auto resource = wl_resource_create(client, &", wl_name, "_interface,"},
            {"                                   std::min(version, me->max_version), id);"},
            "if (resource == nullptr)",
            Block{
                "wl_client_post_no_memory(client);",
                "BOOST_THROW_EXCEPTION((std::bad_alloc{}));",
            },
            (has_vtable ?
            "wl_resource_set_implementation(resource, &vtable, me, nullptr);" :
            Emitter{nullptr}),
            "try",
            Block{
                "me->bind(client, resource);"
            },
            "catch(...)",
            Block{{
                "::mir::log(",
                List{{"::mir::logging::Severity::critical",
                    "\"frontend:Wayland\"",
                    "std::current_exception()",
                    {"\"Exception processing ", generated_name, "::bind() request\""}},
                    Line{{","}, false, true}, "           "},
                    ");"
            }}
        }
    };
}

Emitter Interface::resource_destroyed_thunk() const
{
    return Lines{
        "static void resource_destroyed_thunk(wl_resource* resource)",
        Block{
            {"delete static_cast<", generated_name, "*>(wl_resource_get_user_data(resource));"}
        }
    };
}

Emitter Interface::vtable_declare() const
{
    return Line{"static struct ", wl_name, "_interface const vtable;"};
}


Emitter Interface::vtable_init() const
{
    return Lines{
        {"struct ", wl_name, "_interface const ", nmspace, "vtable = {"},
            {vtable_contents(), "};"}
    };
}

Emitter Interface::vtable_contents() const
{
    std::vector<Emitter> elems;
    for (auto const& method : methods)
    {
        elems.push_back(method.vtable_initialiser());
    }
    return List{elems, Line{{","}, false, true}, Emitter::single_indent};
}

std::vector<Method> Interface::get_methods(xmlpp::Element const& node, bool is_global)
{
    std::vector<Method> methods;
    for (auto method_node : node.get_children("request"))
    {
        auto method = dynamic_cast<xmlpp::Element*>(method_node);
        methods.emplace_back(Method{std::ref(*method), is_global});
    }
    return methods;
}
