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

#ifndef MIR_WAYLAND_GENERATOR_EVENT_H
#define MIR_WAYLAND_GENERATOR_EVENT_H

#include "method.h"

class Event : public Method
{
public:
    Event(xmlpp::Element const& node, std::string const& class_name, bool is_global, int opcode);

    Emitter prototype() const;
    Emitter impl() const;

protected:
    // converts wl input types to mir types
    Emitter mir2wl_converters() const;

    Emitter mir_args() const;

    // arguments to call the virtual mir function call (just names, no types)
    Emitter wl_call_args() const;

    int const opcode;
};

#endif // MIR_WAYLAND_GENERATOR_EVENT_H
