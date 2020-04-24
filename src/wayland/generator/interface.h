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

#ifndef MIR_WAYLAND_GENERATOR_INTERFACE_H
#define MIR_WAYLAND_GENERATOR_INTERFACE_H

#include "emitter.h"
#include "request.h"
#include "event.h"
#include "enum.h"
#include "global.h"

#include <experimental/optional>
#include <functional>
#include <map>
#include <unordered_set>

class Method;

namespace xmlpp
{
class Element;
}

class Interface
{
public:
    Interface(xmlpp::Element const& node,
              std::function<std::string(std::string)> const& name_transform,
              std::unordered_set<std::string> const& constructible_interfaces,
              std::unordered_multimap<std::string, std::string> const& event_constructable_interfaces);

    std::string class_name() const;
    Emitter declaration() const;
    Emitter implementation() const;
    Emitter wl_interface_init() const;
    void populate_required_interfaces(std::set<std::string>& interfaces) const; // fills the set with interfaces used

private:
    Emitter constructor_prototypes() const;
    Emitter constructor_impl() const;
    Emitter constructor_impl(std::string const& parent_interface) const;
    Emitter constructor_args() const;
    Emitter constructor_args(std::string const& parent_interface) const;
    Emitter destructor_prototype() const;
    Emitter destructor_impl() const;
    Emitter virtual_request_prototypes() const;
    Emitter event_prototypes() const;
    Emitter event_impls() const;
    Emitter member_vars() const;
    Emitter enum_declarations() const;
    Emitter event_opcodes() const;
    Emitter thunks_impl() const;
    Emitter thunks_impl_contents() const;
    Emitter resource_destroyed_thunk() const;
    Emitter types_init() const;
    Emitter is_instance_prototype() const;
    Emitter is_instance_impl() const;

    static std::vector<Request> get_requests(xmlpp::Element const& node, std::string generated_name);
    static std::vector<Event> get_events(xmlpp::Element const& node, std::string generated_name);
    static std::vector<Enum> get_enums(xmlpp::Element const& node);

    std::string const wl_name;
    int const version;
    std::string const generated_name;
    std::string const nmspace;
    bool const has_server_constructor;
    bool const has_client_constructor;
    std::experimental::optional<Global> const global;
    std::vector<Request> const requests;
    std::vector<Event> const events;
    std::vector<Enum> const enums;
    std::vector<std::string> const parent_interfaces;
    bool const has_vtable;
};

#endif // MIR_WAYLAND_GENERATOR_INTERFACE_H
