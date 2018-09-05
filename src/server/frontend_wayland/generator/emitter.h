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

#ifndef MIR_WAYLAND_GENERATOR_EMITTER_H
#define MIR_WAYLAND_GENERATOR_EMITTER_H

#include <ostream>
#include <initializer_list>
#include <vector>
#include <memory>

class Emitter
{
public:
    // is used by emit method
    struct State
    {
        State(std::ostream& out)
        : out{out},
        on_fresh_line{std::make_shared<bool>(true)},
        indent{""}
        {
        }

        std::ostream& out;
        std::shared_ptr<bool> const on_fresh_line;
        std::string indent;
    };

    Emitter() = delete;

    class Impl;

    // explicit low level constructors
    static std::shared_ptr<Impl const> string(std::string text);
    static std::shared_ptr<Impl const> seq(std::vector<Emitter> const& children,
                                           Emitter const& delimiter = Emitter::null,
                                           bool at_start = false,
                                           bool at_end = false);
    static std::shared_ptr<Impl const> layout(Emitter const& child,
                                              bool clear_line_before,
                                              bool clear_line_after,
                                              std::string indent = "");

    // constructors for simple emitters
    Emitter(std::shared_ptr<Impl const> impl);
    Emitter(std::string const& text);
    Emitter(const char* text);
    Emitter(std::initializer_list<Emitter> emitters);
    explicit Emitter(std::vector<Emitter> const& emitters);

    void emit(State state) const;
    bool is_valid() const { return impl != nullptr; }

    static Emitter const null;
    static std::string const single_indent;

private:

    std::shared_ptr<Impl const> impl;
};

Emitter const extern empty_line;

// a line that is at the same indentation level as surrounding block
// implicitly convertible to an Emitter
struct Line
{
    Line(std::initializer_list<Emitter> emitters) : emitters{emitters} {}
    Line(std::initializer_list<Emitter> emitters, bool break_before, bool break_after, std::string indent = "")
        : emitters{emitters},
          break_before{break_before},
          break_after{break_after},
          indent{indent}
    {
    }

    inline operator Emitter() const
    {
        return Emitter::layout(Emitter{emitters}, break_before, break_after, indent);
    }

private:
    std::vector<Emitter> const emitters;
    bool const break_before{true};
    bool const break_after{true};
    std::string const indent;
};

#include <iostream>
// a series of lines that is at the same indentation level as surrounding block
// implicitly convertible to an Emitter
struct Lines
{
    explicit Lines(std::initializer_list<Emitter> emitters) : emitters{std::vector<Emitter>(emitters)} {}
    explicit Lines(std::vector<Emitter> const& emitters) : emitters{emitters} {}

    inline operator Emitter() const
    {
        std::vector<Emitter> l;
        for (auto const& i : emitters)
        {
            l.push_back(Emitter::layout(i, true, true));
        }
        return Emitter{l};
    }

private:
    std::vector<Emitter> const emitters;
};

// an indented curly brace surrounded block
// implicitly convertible to an Emitter
struct Block
{
    explicit Block(std::initializer_list<Emitter> emitters) : emitters{std::vector<Emitter>(emitters)} {}
    explicit Block(std::vector<Emitter> const& emitters) : emitters{emitters} {}

    inline operator Emitter() const
    {
        return Emitter::layout({
                "{",
                Emitter::layout(
                    Lines{emitters},
                    true,
                    true,
                    Emitter::single_indent),
                "}"},
            true,
            false);
    }

private:
    std::vector<Emitter> const emitters;
};

// a list of items with delimiters (such as commas) between them
// implicitly convertible to an Emitter
struct List
{
    explicit List(std::initializer_list<Emitter> items, Emitter const& delimiter, std::string indent = "")
        : items{std::vector<Emitter>(items)},
          delimiter{delimiter},
          indent{indent}
    {
    }

    explicit List(std::vector<Emitter> const& items, Emitter const& delimiter, std::string indent = "")
        : items{items},
          delimiter{delimiter},
          indent{indent}
    {
    }

    inline operator Emitter() const {
        return Emitter::layout(
            Emitter::seq(
                items,
                delimiter,
                false,
                false),
            false,
            false,
            indent);
    }

private:
    std::vector<Emitter> const items;
    Emitter const& delimiter;
    std::string const indent;
};

#endif // MIR_WAYLAND_GENERATOR_EMITTER_H
