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
      version{std::stoi(node.get_attribute_value("version"))},
      generated_name{name_transform(wl_name)},
      nmspace{"mfw::" + generated_name + "::"},
      is_global{constructable_interfaces.count(wl_name) == 0},
      requests{get_requests(node, generated_name, is_global)},
      events{get_events(node, generated_name, is_global)},
      enums{get_enums(node)},
      has_vtable{!requests.empty()}
{
}

Emitter Interface::declaration() const
{
    return Lines{
        {"class ", generated_name},
        "{",
        "public:",
        Emitter::layout(EmptyLineList{
            {"static ", generated_name, "* from(struct wl_resource*);"},
            Lines {
                constructor_prototype(),
                destructor_prototype(),
            },
            event_prototypes(),
            {"void destroy_wayland_object(", is_global ? "struct wl_resource* resource" : nullptr, ") const;"},
            member_vars(),
            enum_declarations(),
            event_opcodes(),
            (thunks_impl_contents().is_valid() ? "struct Thunks;" : nullptr),
        }, true, true, Emitter::single_indent),
        empty_line,
        "private:",
        Emitter::layout(EmptyLineList{
            (is_global ? bind_prototype() : nullptr),
            virtual_request_prototypes(),
        }, true, true, Emitter::single_indent),
        "};"
    };
}

Emitter Interface::implementation() const
{
    return Lines{
        {"// ", generated_name},
        empty_line,
        EmptyLineList{
            Lines{
                {"mfw::", generated_name, "* ", nmspace, "from(struct wl_resource* resource)"},
                Block{
                    {"return static_cast<", generated_name, "*>(wl_resource_get_user_data(resource));"}
                }
            },
            thunks_impl(),
            constructor_impl(),
            destructor_impl(),
            event_impls(),
            Lines{
                {"void ", nmspace, "destroy_wayland_object(", is_global ? "struct wl_resource* resource" : nullptr, ") const"},
                Block{
                    {"wl_resource_destroy(resource);"}
                }
            },
            types_init()
        },
    };
}

Emitter Interface::wl_interface_init() const
{
    return Lines{
        {"struct wl_interface const ", wl_name, "_interface_data ",
            BraceList{
                {"\"", wl_name, "\", ", std::to_string(version)},
                {std::to_string(requests.size()), ", ",  (requests.empty() ? "nullptr" : nmspace + "Thunks::request_messages")},
                {std::to_string(events.size()), ", ",  (events.empty() ? "nullptr" : nmspace + "Thunks::event_messages")}
            }}
    };
}

void Interface::populate_required_interfaces(std::set<std::string>& interfaces) const
{
    interfaces.insert(wl_name);

    for (auto const& request : requests)
    {
        request.populate_required_interfaces(interfaces);
    }

    for (auto const& event : events)
    {
        event.populate_required_interfaces(interfaces);
    }
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
            {"    : global{wl_global_create(display, &", wl_name, "_interface_data, max_version, this, &", "Thunks::bind_thunk)},"},
            {"      max_version{max_version}"},
            Block{
                "if (global == nullptr)",
                Block{
                    {"BOOST_THROW_EXCEPTION((std::runtime_error{\"Failed to export ", wl_name, " interface\"}));"}
                }
            }
        };
    }
    else
    {
        return Lines{
            {nmspace, generated_name, "(", constructor_args(), ")"},
            {"    : client{client},"},
            {"      resource{wl_resource_create(client, &", wl_name, "_interface_data, wl_resource_get_version(parent), id)}"},
            Block{
                "if (resource == nullptr)",
                Block{
                    "wl_resource_post_no_memory(parent);",
                    "BOOST_THROW_EXCEPTION((std::bad_alloc{}));",
                },
                (has_vtable ?
                    "wl_resource_set_implementation(resource, Thunks::request_vtable, this, &Thunks::resource_destroyed_thunk);" :
                    Emitter{nullptr})
            }
        };
    }
}

Emitter Interface::constructor_args() const
{
    if (is_global)
    {
        return {"struct wl_display* display, uint32_t max_version"};
    }
    else
    {
        return {"struct wl_client* client, struct wl_resource* parent, uint32_t id"};
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

Emitter Interface::virtual_request_prototypes() const
{
    std::vector<Emitter> prototypes;
    for (auto const& request : requests)
    {
        prototypes.push_back(request.virtual_mir_prototype());
    }
    return Lines{prototypes};
}

Emitter Interface::event_prototypes() const
{
    std::vector<Emitter> prototypes;
    for (auto const& event : events)
    {
        prototypes.push_back(event.prototype());
    }
    return Lines{prototypes};
}

Emitter Interface::event_impls() const
{
    std::vector<Emitter> impls;
    for (auto const& event : events)
    {
        impls.push_back(event.impl());
    }
    return EmptyLineList{impls};
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

Emitter Interface::enum_declarations() const
{
    std::vector<Emitter> declarations;
    for (auto const& i : enums)
    {
        declarations.push_back(i.declaration());
    }
    return EmptyLineList{declarations};
}

Emitter Interface::event_opcodes() const
{
    std::vector<Emitter> opcodes;
    for (auto const& i : events)
    {
        opcodes.push_back(i.opcode_declare());
    }
    Emitter body{opcodes};
    if (body.is_valid())
    {
        return Lines{
            {"struct Opcode"},
            {Block{
                body
            }, ";"}
        };
    }
    else
    {
        return nullptr;
    }
}

Emitter Interface::thunks_impl() const
{
    Emitter contents = thunks_impl_contents();

    if (contents.is_valid())
    {
        return Lines{
            {"struct ", nmspace, "Thunks"},
            {Block{
                contents
            }, ";"},
        };
    }
    else
    {
        return nullptr;
    }
}

Emitter Interface::thunks_impl_contents() const
{
    std::vector<Emitter> impls;
    for (auto const& request : requests)
        impls.push_back(request.thunk_impl());

    if (is_global)
        impls.push_back(bind_thunk());

    if (has_vtable && !is_global)
        impls.push_back(resource_destroyed_thunk());

    std::vector<Emitter> declares;
    for (auto const& request : requests)
        declares.push_back(request.types_declare());
    for (auto const& event : events)
        declares.push_back(event.types_declare());
    if (!requests.empty())
        declares.push_back("static struct wl_message const request_messages[];");
    if (!events.empty())
        declares.push_back("static struct wl_message const event_messages[];");
    if (has_vtable)
        declares.push_back({"static void const* request_vtable[];"});
    impls.push_back(Lines{declares});

    return EmptyLineList{impls};
}

Emitter Interface::bind_thunk() const
{
    return Lines{
        "static void bind_thunk(struct wl_client* client, void* data, uint32_t version, uint32_t id)",
        Block{
            {"auto me = static_cast<", generated_name, "*>(data);"},
            {"auto resource = wl_resource_create(client, &", wl_name, "_interface_data,"},
            {"                                   std::min(version, me->max_version), id);"},
            "if (resource == nullptr)",
            Block{
                "wl_client_post_no_memory(client);",
                "BOOST_THROW_EXCEPTION((std::bad_alloc{}));",
            },
            (has_vtable ?
                "wl_resource_set_implementation(resource, Thunks::request_vtable, me, nullptr);" :
                Emitter{nullptr}),
            "try",
            Block{
                "me->bind(client, resource);"
            },
            "catch(...)",
            Block{{
                "::mir::log(",
                Emitter::layout(
                    Emitter::seq({
                            "::mir::logging::Severity::critical",
                            "\"frontend:Wayland\"",
                            "std::current_exception()",
                            {"\"Exception processing ", generated_name, "::bind() request\""}},
                        Emitter::layout(",", false, true)),
                    false,
                    false,
                    "           "),
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

Emitter Interface::types_init() const
{
    std::vector<Emitter> types;
    for (auto const& request : requests)
        types.push_back(request.types_init());
    for (auto const& event : events)
        types.push_back(event.types_init());

    if (!requests.empty())
    {
        std::vector<Emitter> request_messages;
        for (auto const& request : requests)
            request_messages.push_back(request.wl_message_init());

        types.push_back(Lines{
            {"struct wl_message const ", nmspace, "Thunks::request_messages[] ",
                BraceList{request_messages}}
        });
    }

    if (!events.empty())
    {
        std::vector<Emitter> event_messages;
        for (auto const& event : events)
            event_messages.push_back(event.wl_message_init());

        types.push_back(Lines{
            {"struct wl_message const ", nmspace, "Thunks::event_messages[] ",
                BraceList{event_messages}}
        });
    }

    if (has_vtable)
    {
        std::vector<Emitter> elems;
        for (auto const& request : requests)
        {
            elems.push_back({"(void*)Thunks::", request.vtable_initialiser()});
        }

        types.push_back(Lines{
            {"void const* ", nmspace, "Thunks::request_vtable[] ",
                BraceList{elems}},
        });
    }

    return EmptyLineList{types};
}

std::vector<Request> Interface::get_requests(xmlpp::Element const& node, std::string generated_name, bool is_global)
{
    std::vector<Request> requests;
    for (auto method_node : node.get_children("request"))
    {
        auto elem = dynamic_cast<xmlpp::Element*>(method_node);
        requests.emplace_back(Request{std::ref(*elem), generated_name, is_global});
    }
    return requests;
}

std::vector<Event> Interface::get_events(xmlpp::Element const& node, std::string generated_name, bool is_global)
{
    std::vector<Event> events;
    int opcode = 0;
    for (auto method_node : node.get_children("event"))
    {
        auto elem = dynamic_cast<xmlpp::Element*>(method_node);
        events.emplace_back(Event{std::ref(*elem), generated_name, is_global, opcode});
        opcode++;
    }
    return events;
}

std::vector<Enum> Interface::get_enums(xmlpp::Element const& node)
{
    std::vector<Enum> enums;
    for (auto method_node : node.get_children("enum"))
    {
        auto elem = dynamic_cast<xmlpp::Element*>(method_node);
        enums.emplace_back(Enum{std::ref(*elem)});
    }
    return enums;
}
