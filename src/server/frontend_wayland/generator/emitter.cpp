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

Emitter::State Emitter::State::indented()
{
    auto copy = *this;
    copy.indent += single_indent;
    return copy;
}

class Emitter::Impl
{
public:
    Impl() = default;
    virtual ~Impl() = default;

    virtual void emit(State state) const = 0;

    void emit_newline(State state) const
    {
        state.out << "\n";
        state.out << state.indent;
    }
};

class BlockEmitter : public Emitter::Impl
{
public:
    BlockEmitter(std::initializer_list<Emitter> const& children)
        : children{move(children)}
    {
    }

    virtual ~BlockEmitter() = default;

    void emit(Emitter::State state) const override
    {
        emit_newline(state);
        state.out << "{";
        auto child_state = state.indented();
        for (auto& i: children)
        {
            i.emit(state);
        }
        emit_newline(state);
        state.out << "}";
    }

private:
    std::vector<Emitter> children;
};

class LineEmitter : public Emitter::Impl
{
public:
    LineEmitter(std::initializer_list<Emitter> const& children)
        : children(std::make_move_iterator(std::begin(children)), std::make_move_iterator(std::end(children)))
    {
    }

    virtual ~LineEmitter() = default;

    void emit(Emitter::State state) const override
    {
        emit_newline(state);
        for (auto& i: children)
        {
            i.emit(state);
        }
    }

private:
    std::vector<Emitter> children;
};

class FragmentEmitter : public Emitter::Impl
{
public:
    FragmentEmitter(std::string text)
        : text{text}
    {
    }

    virtual ~FragmentEmitter() = default;

    void emit(Emitter::State state) const override
    {
        state.out << text;
    }

private:
    std::string text;
};

Emitter::Emitter(std::string const& text)
    : impl{std::make_shared<FragmentEmitter>(text)}
{
}

Emitter::Emitter(const char* text)
: impl{std::make_shared<FragmentEmitter>(text)}
{
}

Emitter::Emitter(std::initializer_list<Emitter> const& emitters)
    : impl{std::make_shared<LineEmitter>(move(emitters))}
{
}

Emitter Emitter::block(std::initializer_list<Emitter> const& emitters)
{
    return Emitter{std::make_shared<BlockEmitter>(move(emitters))};
}

void Emitter::emit(State state) const
{
    impl->emit(state);
}

Emitter::Emitter(std::shared_ptr<Impl const> impl)
    : impl{move(impl)}
{
}

/*
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
*/
