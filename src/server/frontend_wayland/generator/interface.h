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
    Interface(
        xmlpp::Element const& node,
        std::function<std::string(std::string)> const& name_transform,
        std::unordered_set<std::string> const& constructable_interfaces);

    Emitter full_class();

private:
    Emitter constructor();
    Emitter constructor_for_global();
    Emitter constructor_for_regular();
    Emitter destructor();
    Emitter bind_prototype();
    Emitter virtual_method_prototypes();
    Emitter member_vars();
    Emitter thunk_bodies();
    Emitter bind_thunk();
    Emitter resource_destroyed_thunk();
    Emitter vtable_getter();
    Emitter vtable_contents();

    static std::vector<Method> get_methods(xmlpp::Element const& node, bool is_global);

    std::string const wl_name;
    std::string const generated_name;
    bool const is_global;
    std::vector<Method> const methods;
    bool const has_vtable;
};

#endif // MIR_WAYLAND_GENERATOR_INTERFACE_H
