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

#include "emitter.h"

std::string const Emitter::single_indent = "    ";

class Emitter::Impl
{
public:
    Impl() = default;
    virtual ~Impl() = default;

    virtual void emit(State state) const = 0;

    static bool contains_valid(std::vector<Emitter> const& emitters)
    {
        for (auto const& i: emitters)
        {
            if (i.is_valid())
                return true;
        }
        return false;
    }
};

class EmptyLineEmitter : public Emitter::Impl
{
public:
    EmptyLineEmitter() = default;
    virtual ~EmptyLineEmitter() = default;

    void emit(Emitter::State state) const override
    {
        if (!*state.on_fresh_line)
            state.out << "\n";
        state.out << "\n";
        *state.on_fresh_line = true;
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
        if (*state.on_fresh_line)
            state.out << state.indent;
        state.out << text;
        *state.on_fresh_line = false;
    }

private:
    std::string const text;
};

class SeqEmitter : public Emitter::Impl
{
public:
    SeqEmitter(std::vector<Emitter> const && children, Emitter const && delimiter = nullptr, bool at_start = false, bool at_end = false)
        : children(move(children)),
          delimiter{delimiter},
          at_start{at_start},
          at_end{at_end}
    {
    }

    virtual ~SeqEmitter() = default;

    void emit(Emitter::State state) const override
    {
        if (at_start)
            delimiter.emit(state);
        bool is_start = true;
        for (auto& i: children)
        {
            if (!is_start && delimiter.is_valid())
                delimiter.emit(state);
            is_start = false;
            i.emit(state);
        }
        if (at_end)
            delimiter.emit(state);
    }

private:
    std::vector<Emitter> const children;
    Emitter const delimiter;
    bool const at_start;
    bool const at_end;
};

class LayoutEmitter : public Emitter::Impl
{
public:
    LayoutEmitter(Emitter && child, bool clear_line_before = true, bool clear_line_after = true, std::string indent="")
        : child{std::move(child)},
          clear_line_before{clear_line_before},
          clear_line_after{clear_line_after},
          indent{indent}
    {
    }

    virtual ~LayoutEmitter() = default;

    void emit(Emitter::State state) const override
    {
        state.indent += indent;
        if (clear_line_before && !*state.on_fresh_line)
        {
            state.out << "\n";
            *state.on_fresh_line = true;
        }
        child.emit(state);
        if (clear_line_after && !*state.on_fresh_line)
        {
            state.out << "\n";
            *state.on_fresh_line = true;
        }
    }

private:
    Emitter const child;
    bool const clear_line_before;
    bool const clear_line_after;
    std::string const indent;
};

Emitter::Emitter(std::string const& text)
    : impl{text.empty() ? nullptr : std::make_shared<StringEmitter>(text)}
{
}

Emitter::Emitter(const char* text)
    : impl{text != nullptr && *text!='\0' ?
               std::make_shared<StringEmitter>(std::string{text}) :
               nullptr}
{
}

Emitter::Emitter(std::initializer_list<Emitter> const& emitters)
    : impl{Impl::contains_valid(emitters) ?
               std::make_shared<SeqEmitter>(emitters) :
               nullptr}
{
}

Emitter::Emitter(std::vector<Emitter> emitters)
    : impl{Impl::contains_valid(emitters) ?
               std::make_shared<SeqEmitter>(move(emitters)) :
               nullptr}
{
}

Emitter::Emitter(Line && line)
{
    Emitter e{line.emitters};
    if (e.is_valid())
    {
        impl = std::make_shared<LayoutEmitter>(std::move(e), line.break_before, line.break_after, line.indent);
    }
}

Emitter::Emitter(Lines && lines)
{
    std::vector<Emitter> l;
    for (auto& i : lines.emitters)
    {
        if (i.is_valid())
            l.push_back(Emitter{std::make_shared<LayoutEmitter>(std::move(i))});
    }
    if (!l.empty())
    {
        impl = std::make_shared<SeqEmitter>(move(l));
    }
}

Emitter::Emitter(Block && block)
{
    impl = std::make_shared<LayoutEmitter>(Emitter{"{",
                                                   Emitter{std::make_shared<LayoutEmitter>(
                                                       Lines{block.emitters}, true, true, single_indent)},
                                                   "}"}, true, true);
}

Emitter::Emitter(List && list)
{
    std::vector<Emitter> l;
    for (auto& i : list.items)
    {
        if (i.is_valid())
            l.push_back(std::move(i));
    }
    if (!l.empty())
    {
        impl = std::make_shared<SeqEmitter>(move(l), std::move(list.delimiter));
        if (!list.indent.empty())
            impl = std::make_shared<LayoutEmitter>(Emitter{impl}, false, false, list.indent);
    }
}

void Emitter::emit(State state) const
{
    if (impl)
    {
        impl->emit(state);
    }
}

Emitter::Emitter(std::shared_ptr<Impl const> impl)
    : impl{move(impl)}
{
}

Emitter const empty_line = Emitter{std::make_shared<EmptyLineEmitter>()};
