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

namespace xmlpp
{
class Element;
}

struct ArgumentTypeDescriptor
{
    std::string cpp_type;
    std::string c_type;
    std::experimental::optional<std::vector<std::string>> converter;
};

class Argument
{
public:
    Argument(xmlpp::Element const& node);

    Emitter emit_c_prototype() const;
    Emitter emit_cpp_prototype() const;
    Emitter emit_thunk_call_fragment() const;
    Emitter emit_thunk_converter() const;

private:
    std::string const name;
    ArgumentTypeDescriptor const& descriptor;
};

#endif // MIR_WAYLAND_GENERATOR_ARGUMENT_H
