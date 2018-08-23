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

class Emitter
{
public:
    Emitter(std::ostream& out);

    void emit(std::string fragment);
    void emit(std::initializer_list<std::string> fragments);
    void emit_newline();
    void emit_indent();
    void emit_lines(std::initializer_list<std::initializer_list<std::string>> lines);
    void inline increase_indent() { indent++; }
    void inline decrease_indent() { indent--; }

private:
    std::ostream& out;
    int indent{0};
    bool fresh_line{true};
    static std::string const single_indent;
};

#endif // MIR_WAYLAND_GENERATOR_EMITTER_H
