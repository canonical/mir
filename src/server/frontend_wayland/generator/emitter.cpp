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

    static std::vector<Emitter> insert_between(std::vector<Emitter> const && emitters,
                                               Emitter const && delimiter,
                                               bool at_start = true,
                                               bool at_end = false)
    {
        std::vector<Emitter> ret;
        for (auto const& i: emitters)
        {
            if (at_start || !ret.empty())
                ret.push_back(delimiter);
            ret.push_back(i);
        }
        if (at_end)
            ret.push_back(delimiter);
        return ret;
    }
};

class NewlineEmitter : public Emitter::Impl
{
public:
    NewlineEmitter() = default;
    virtual ~NewlineEmitter() = default;

    void emit(Emitter::State state) const override
    {
        state.out << "\n";
        state.out << state.indent;
    }
};

class StringEmitter : public Emitter::Impl
{
public:
    StringEmitter(std::string text)
    : text{text}
    {
    }

    virtual ~StringEmitter() = default;

    void emit(Emitter::State state) const override
    {
        state.out << text;
    }

private:
    std::string text;
};

class FragEmitter : public Emitter::Impl
{
public:
    FragEmitter(std::vector<Emitter> && children)
    : children(move(children))
    {
    }

    virtual ~FragEmitter() = default;

    void emit(Emitter::State state) const override
    {
        for (auto& i: children)
        {
            i.emit(state);
        }
    }

private:
    std::vector<Emitter> children;
};

class BlockEmitter : public Emitter::Impl
{
public:
    BlockEmitter(std::vector<Emitter> && children)
        : children{move(children)}
    {
    }

    virtual ~BlockEmitter() = default;

    void emit(Emitter::State state) const override
    {
        StringEmitter{"{"}.emit(state);
        auto child_state = state.indented();
        for (auto& i: children)
        {
            i.emit(state);
        }
        NewlineEmitter{}.emit(state);
        StringEmitter{"}"}.emit(state);
    }

private:
    std::vector<Emitter> const children;
};

Emitter::Emitter(std::string const& text)
    : impl{std::make_shared<StringEmitter>(text)}
{
}

Emitter::Emitter(const char* text)
    : impl{std::make_shared<StringEmitter>(text)}
{
}

Emitter::Emitter(std::initializer_list<Emitter> const& emitters)
    : impl{std::make_shared<FragEmitter>(move(emitters))}
{
}

Emitter::Emitter(std::vector<Emitter> const& emitters)
    : impl{std::make_shared<FragEmitter>(Impl::insert_between(move(emitters), Newline{}))}
{
}

Emitter::Emitter(Newline)
    : impl{std::make_shared<NewlineEmitter>()}
{
}

Emitter::Emitter(Lines && lines)
    : impl{std::make_shared<FragEmitter>(Impl::insert_between(move(lines.emitters), Newline{}))}
{
}

Emitter::Emitter(Block && block)
    : impl{std::make_shared<BlockEmitter>(Impl::insert_between(move(block.emitters), Newline{}))}
{
}

Emitter::Emitter(List && list)
    : impl{std::make_shared<FragEmitter>(Impl::insert_between(move(list.items), std::move(list.delimiter), false, false))}
{
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
