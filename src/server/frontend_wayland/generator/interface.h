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
#include "method.h"

#include <experimental/optional>
#include <functional>
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
              std::unordered_set<std::string> const& constructable_interfaces);

    Emitter declaration() const;
    Emitter implementation() const;

private:
    Emitter constructor_prototype() const;
    Emitter constructor_impl() const;
    Emitter constructor_args() const;
    Emitter destructor_prototype() const;
    Emitter destructor_impl() const;
    Emitter bind_prototype() const;
    Emitter virtual_method_prototypes() const;
    Emitter member_vars() const;
    Emitter thunks_impl() const;
    Emitter bind_thunk() const;
    Emitter resource_destroyed_thunk() const;
    Emitter vtable_declare() const;
    Emitter vtable_init() const;
    Emitter vtable_contents() const;

    std::vector<Method> get_methods(xmlpp::Element const& node, bool is_global);

    std::string const wl_name;
    std::string const generated_name;
    std::string const nmspace;
    bool const is_global;
    std::vector<Method> const methods;
    bool const has_vtable;
};

#endif // MIR_WAYLAND_GENERATOR_INTERFACE_H
