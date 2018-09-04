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
        std::string wl_type_abbr; // abbreviated type (i for int, etc), used by libwayland
        std::experimental::optional<std::function<Emitter(std::string)>> converter;
    };

    Argument(xmlpp::Element const& node, bool is_event);

    Emitter wl_prototype() const;
    Emitter mir_prototype() const;
    Emitter call_fragment() const;
    Emitter object_type_interface_declare() const;
    Emitter object_type_fragment() const;
    Emitter type_str_fragment() const;
    std::experimental::optional<Emitter> converter() const;

private:
    static TypeDescriptor get_type(xmlpp::Element const& node, bool is_event);
    static std::experimental::optional<std::string> get_interface(xmlpp::Element const& node);

    std::string const name;
    TypeDescriptor descriptor;
    std::experimental::optional<std::string> const interface;
};

#endif // MIR_WAYLAND_GENERATOR_ARGUMENT_H
