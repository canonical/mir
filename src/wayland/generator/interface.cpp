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

#include "interface.h"

#include <libxml++/libxml++.h>

#include "method.h"

namespace
{
class AsRange
{
public:
    using const_iterator = std::unordered_multimap<std::string, std::string>::const_iterator;
    explicit AsRange(std::pair<const_iterator, const_iterator> stupid_api)
        : range{std::move(stupid_api)}
    {
    }

    const_iterator begin()
    {
        return range.first;
    }

    const_iterator end()
    {
        return range.second;
    }

private:
    std::pair<const_iterator, const_iterator> const range;
};

auto matching_keys_to_vector(
    std::unordered_multimap<std::string, std::string> const& map,
    std::function<std::string(std::string)> const& name_transform,
    std::string key)
{
    std::vector<std::string> interfaces;
    for (auto const& i : AsRange{map.equal_range(key)})
    {
        interfaces.push_back(name_transform(i.second));
    }
    return interfaces;
}

auto vec_has_destroy_request(std::vector<Request> const& requests) -> bool
{
    int count = 0;
    for (auto const& request : requests)
    {
        if (request.is_destroy())
        {
            count++;
        }
    }
    if (count > 1)
    {
        throw std::runtime_error{"interface has multiple destroy requests"};
    }
    else
    {
        return count;
    }
}
}

Interface::Interface(xmlpp::Element const& node,
                     std::function<std::string(std::string)> const& name_transform,
                     std::unordered_set<std::string> const& constructable_interfaces,
                     std::unordered_multimap<std::string, std::string> const& event_constructable_interfaces)
    : wl_name{node.get_attribute_value("name")},
      version{std::stoi(node.get_attribute_value("version"))},
      generated_name{name_transform(wl_name)},
      nmspace{"mw::" + generated_name + "::"},
      has_server_constructor{event_constructable_interfaces.count(wl_name) != 0},
      has_client_constructor{constructable_interfaces.count(wl_name) != 0},
      global{!(has_server_constructor || has_client_constructor) ?
          std::make_optional(Global{wl_name, generated_name, version, nmspace}) :
          std::nullopt},
      requests{get_requests(node, generated_name)},
      events{get_events(node, generated_name)},
      enums{get_enums(node, generated_name)},
      parent_interfaces{matching_keys_to_vector(event_constructable_interfaces, name_transform, wl_name)},
      has_destroy_request{vec_has_destroy_request(requests)}
{
}

std::string Interface::class_name() const
{
    return generated_name;
}

Emitter Interface::declaration() const
{
    return Lines{
        {"class ", generated_name, " : public Resource"},
        "{",
        "public:",
        Emitter::layout(EmptyLineList{
            {"static char const constexpr* interface_name = \"", wl_name, "\";"},
            {"static ", generated_name, "* from(struct wl_resource*);"},
            Lines {
                constructor_prototypes(),
                destructor_prototype(),
            },
            event_prototypes(),
            (has_destroy_request ? nullptr : "void destroy_and_delete() const;"),
            enum_declarations(),
            event_opcodes(),
            (thunks_impl_contents().is_valid() ? "struct Thunks;" : nullptr),
            is_instance_prototype(),
            (global ? global.value().declaration() : nullptr),
        }, true, true, Emitter::single_indent),
        empty_line,
        "private:",
        Emitter::layout(EmptyLineList{
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
            thunks_impl(),
            constructor_impl(),
            destructor_impl(),
            event_impls(),
            is_instance_impl(),
            (has_destroy_request ? Emitter{nullptr} :
                Lines{
                    {"void ", nmspace, "destroy_and_delete() const"},
                    Block{
                        {"// Will result in this object being deleted"},
                        {"wl_resource_destroy(resource);"},
                    }
                }
            ),
            enum_impls(),
            (global ? global.value().implementation() : nullptr),
            types_init(),
            Lines{
                {"mw::", generated_name, "* ", nmspace, "from(struct wl_resource* resource)"},
                Block{
                    Lines{
                        {"if (resource &&"},
                        {"    wl_resource_instance_of(resource, &",
                            wl_name,
                            "_interface, ",
                            generated_name,
                            "::Thunks::request_vtable))"},
                        Block{
                            {"return static_cast<", generated_name, "*>(wl_resource_get_user_data(resource));"}
                        },
                        "else",
                        Block{
                            {"return nullptr;"}
                        },
                    },
                }
            },
        },
    };
}

Emitter Interface::wl_interface_init() const
{
    return Lines{
        {"struct wl_interface const ", wl_name, "_interface",
            BraceList{
                {nmspace, "interface_name"},
                {nmspace, "Thunks::supported_version"},
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

auto Interface::is_core() const -> bool
{
    return wl_name.starts_with("wl_");
}

Emitter Interface::constructor_prototypes() const
{
    std::vector<Emitter> prototypes;
    if (has_client_constructor || global)
    {
        prototypes.push_back(Line{generated_name, "(", constructor_args(), ");"});
    }
    if (has_server_constructor)
    {
        for (auto const &parent : parent_interfaces)
        {
            prototypes.push_back(Line{generated_name, "(", constructor_args(parent), ");"});
        }
    }
    return Lines{prototypes};
}

Emitter Interface::constructor_impl() const
{
    std::vector<Emitter> impls;
    if (has_client_constructor || global)
    {
        impls.push_back(Lines{
            {nmspace, generated_name, "(", constructor_args(), ")"},
            {"    : Resource{resource}"},
            Block{
                "wl_resource_set_implementation(resource, Thunks::request_vtable, this, &Thunks::resource_destroyed_thunk);",
            }
        });
    }
    for (auto const& parent : parent_interfaces)
    {
        impls.push_back(constructor_impl(parent));
    }
    return Lines{impls};
}

Emitter Interface::constructor_impl(std::string const& parent_interface) const
{
    return Lines{
        {nmspace, generated_name, "(", constructor_args(parent_interface), ")"},
        {"    : Resource{wl_resource_create("},
        {"          wl_resource_get_client(parent.resource),"},
        {"          &", wl_name, "_interface,"},
        {"          wl_resource_get_version(parent.resource), 0)}"},
        Block{
            "wl_resource_set_implementation(resource, Thunks::request_vtable, this, &Thunks::resource_destroyed_thunk);",
        }
    };
}

Emitter Interface::constructor_args() const
{
    return {"struct wl_resource* resource, Version<", std::to_string(version), ">"};
}

Emitter Interface::constructor_args(std::string const& parent_interface) const
{
    return {parent_interface, " const& parent"};
}

Emitter Interface::destructor_prototype() const
{
    return Line{"virtual ~", generated_name, "();"};
}

Emitter Interface::destructor_impl() const
{
    return Lines{
        {nmspace, "~", generated_name, "()"},
        Block{
            "wl_resource_set_implementation(resource, nullptr, nullptr, nullptr);"
        }
    };
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

Emitter Interface::is_instance_prototype() const
{
    return "static bool is_instance(wl_resource* resource);";
}

Emitter Interface::is_instance_impl() const
{
    return Lines{
        {"bool ", nmspace, "is_instance(wl_resource* resource)"},
        Block{"return wl_resource_instance_of(resource, &" + wl_name + "_interface, Thunks::request_vtable);"}
    };
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

Emitter Interface::enum_impls() const
{
    std::vector<Emitter> declarations;
    for (auto const& i : enums)
    {
        declarations.push_back(i.impl());
    }
    return Lines{declarations};
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
            empty_line,
            {"int const ", nmspace, "Thunks::supported_version = ", std::to_string(version), ";"},
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
    impls.push_back(
        {"static int const supported_version;"});

    for (auto const& request : requests)
        impls.push_back(request.thunk_impl());

    impls.push_back(resource_destroyed_thunk());

    if (global)
        impls.push_back(global.value().bind_thunk_impl());

    std::vector<Emitter> declares;
    for (auto const& request : requests)
        declares.push_back(request.types_declare());
    for (auto const& event : events)
        declares.push_back(event.types_declare());
    if (!requests.empty())
        declares.push_back("static struct wl_message const request_messages[];");
    if (!events.empty())
        declares.push_back("static struct wl_message const event_messages[];");
    declares.push_back({"static void const* request_vtable[];"});
    impls.push_back(Lines{declares});

    return EmptyLineList{impls};
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

    std::vector<Emitter> request_initializers;
    for (auto const& request : requests)
    {
        request_initializers.push_back({"(void*)Thunks::", request.vtable_initialiser()});
    }

    if (request_initializers.empty())
    {
        request_initializers.push_back("nullptr");
    }

    types.push_back(Lines{
        {"void const* ", nmspace, "Thunks::request_vtable[] ",
            BraceList{request_initializers}},
    });

    return EmptyLineList{types};
}

std::vector<Request> Interface::get_requests(xmlpp::Element const& node, std::string generated_name)
{
    std::vector<Request> requests;
    for (auto method_node : node.get_children("request"))
    {
        auto elem = dynamic_cast<xmlpp::Element*>(method_node);
        requests.emplace_back(Request{std::ref(*elem), generated_name});
    }
    return requests;
}

std::vector<Event> Interface::get_events(xmlpp::Element const& node, std::string generated_name)
{
    std::vector<Event> events;
    int opcode = 0;
    for (auto method_node : node.get_children("event"))
    {
        auto elem = dynamic_cast<xmlpp::Element*>(method_node);
        events.emplace_back(Event{std::ref(*elem), generated_name, opcode});
        opcode++;
    }
    return events;
}

std::vector<Enum> Interface::get_enums(xmlpp::Element const& node, std::string generated_name)
{
    std::vector<Enum> enums;
    for (auto method_node : node.get_children("enum"))
    {
        auto elem = dynamic_cast<xmlpp::Element*>(method_node);
        enums.emplace_back(Enum{std::ref(*elem), generated_name});
    }
    return enums;
}
