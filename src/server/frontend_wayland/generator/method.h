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

#ifndef MIR_WAYLAND_GENERATOR_METHOD_H
#define MIR_WAYLAND_GENERATOR_METHOD_H

#include "emitter.h"
#include "argument.h"

#include <experimental/optional>
#include <functional>
#include <unordered_map>

namespace xmlpp
{
class Element;
}

// represents an request handler on a Wayland object
class Method
{
public:
    Method(xmlpp::Element const& node, bool is_global);

    // prototype of virtual function that is overridden in Mir
    Emitter virtual_mir_prototype() const;

    // the thunk is the static function that libwayland calls
    // It does some type conversion and calls the virtual method, which should be overridden somewhere in Mir
    Emitter thunk_body(std::string const& interface_type) const;

    // the bit of this objects vtable that holds this method
    Emitter vtable_initialiser() const;

private:
    // arguments from libwayland to the thunk
    Emitter wl_args() const;

    // arguments of the mir virtual function to the thunk (names with types)
    Emitter mir_args() const;

    // converts wl input types to mir types
    Emitter converters() const;

    // arguments to call the virtual mir function call (just names, no types)
    Emitter mir_call_args() const;

    std::string const name;
    bool const is_global;
    std::vector<Argument> arguments;
};

#endif // MIR_WAYLAND_GENERATOR_METHOD_H
