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

#ifndef MIR_WAYLAND_GENERATOR_ENUM_H
#define MIR_WAYLAND_GENERATOR_ENUM_H

#include "emitter.h"

namespace xmlpp
{
class Element;
}

class Enum
{
public:
    struct Entry
    {
        std::string name;
        std::string value;
    };

    Enum(xmlpp::Element const& node);

    Emitter declaration() const;

private:
    std::string const name;
    std::vector<Entry> entries;
};

#endif // MIR_WAYLAND_GENERATOR_ARGUMENT_H
