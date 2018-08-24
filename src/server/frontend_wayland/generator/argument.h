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

#ifndef MIR_WAYLAND_GENERATOR_ARGUMENT_H
#define MIR_WAYLAND_GENERATOR_ARGUMENT_H

#include "emitter.h"

#include <experimental/optional>
#include <functional>
#include <unordered_map>

namespace xmlpp
{
class Element;
}

class Argument
{
public:
    struct TypeDescriptor
    {
        std::string mir_type;
        std::string wl_type;
        // wl to mir converter
        std::experimental::optional<std::function<Emitter(std::string)>> converter;
    };

    Argument(xmlpp::Element const& node);

    Emitter wl_prototype() const;
    Emitter mir_prototype() const;
    Emitter mir_call_fragment() const;
    std::experimental::optional<Emitter> converter() const;

private:
    static bool argument_is_optional(xmlpp::Element const& arg);
    static Emitter fd_converter(std::string name);
    static Emitter optional_object_converter(std::string name);
    static Emitter optional_string_converter(std::string name);

    static std::unordered_map<std::string, Argument::TypeDescriptor const> const type_map;
    static std::unordered_map<std::string, Argument::TypeDescriptor const> const optional_type_map;

    std::string const name;
    TypeDescriptor const& descriptor;
};

#endif // MIR_WAYLAND_GENERATOR_ARGUMENT_H
