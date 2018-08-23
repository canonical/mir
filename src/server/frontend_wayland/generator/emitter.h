/*
 * Copyright © 2017 Canonical Ltd.
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

#ifndef MIR_WAYLAND_GENERATOR_EMITTER_H
#define MIR_WAYLAND_GENERATOR_EMITTER_H

#include <ostream>
#include <initializer_list>
#include <vector>
#include <memory>

class Emitter
{
public:
    struct State
    {
        std::ostream& out;
        std::string indent;

        State indented();
    };

    Emitter(std::string const& text);
    Emitter(const char* text);
    Emitter(std::initializer_list<Emitter> const& emitters);

    static Emitter block(std::initializer_list<Emitter> const& emitters);

    void emit(State state) const;

    class Impl;

private:
    Emitter(std::shared_ptr<Impl const> impl);

    std::shared_ptr<Impl const> const impl;

    static std::string const single_indent;
};



#endif // MIR_WAYLAND_GENERATOR_EMITTER_H
