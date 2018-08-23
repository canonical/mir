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

#include "emitter.h"

std::string const Emitter::single_indent = "    ";

Emitter::Emitter(std::ostream& out)
    : out{out}
{
}

void Emitter::emit(std::string fragment)
{
    out << fragment;
    fresh_line = false;
}

void Emitter::emit(std::initializer_list<std::string> fragments)
{
    for (auto const& fragment : fragments)
    {
        emit(fragment);
    }
}

void Emitter::emit_newline()
{
    out << "\n";
    fresh_line = true;
}

void Emitter::emit_indent()
{
    if (!fresh_line)
        emit_newline();

    for (int i = 0; i < indent; i++)
    {
        out << single_indent;
        fresh_line = false;
    }
}

void Emitter::emit_lines(std::initializer_list<std::initializer_list<std::string>> lines)
{
    for (auto const& line : lines)
    {
        emit_indent();
        emit(line);
        emit_newline();
    }
}
