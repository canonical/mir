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
Emitter const Emitter::null{std::shared_ptr<Emitter::Impl>(nullptr)};

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
    SeqEmitter(std::vector<Emitter> const& children, Emitter const& delimiter = nullptr, bool at_start = false, bool at_end = false)
        : children(children),
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
            if (delimiter.is_valid() && !is_start)
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
    LayoutEmitter(Emitter const& child, bool clear_line_before = true, bool clear_line_after = true, std::string indent="")
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

std::shared_ptr<Emitter::Impl const> Emitter::string(std::string text)
{
    if (text.empty())
        return {};
    else
        return std::make_shared<StringEmitter>(text);
}

std::shared_ptr<Emitter::Impl const> Emitter::seq(std::vector<Emitter> const& children,
                                                  Emitter const& delimiter,
                                                  bool at_start,
                                                  bool at_end)
{
    std::vector<Emitter> valid;
    for (auto const& i : children)
    {
        if (i.is_valid())
            valid.push_back(i);
    }
    if (valid.empty())
        return {};
    else
        return std::make_shared<SeqEmitter>(move(valid), delimiter, at_start, at_end);
}

std::shared_ptr<Emitter::Impl const> Emitter::layout(Emitter const& child,
                                                     bool clear_line_before,
                                                     bool clear_line_after,
                                                     std::string indent)
{
    if (clear_line_before == false && clear_line_after == false && indent == "")
        return child.impl;
    else if (child.is_valid())
        return std::make_shared<LayoutEmitter>(child, clear_line_before, clear_line_after, indent);
    else
        return {};
}

Emitter::Emitter(std::string const& text) : impl{string(text)} {}
Emitter::Emitter(const char* text) : impl{text == nullptr ? nullptr : string(text)} {}
Emitter::Emitter(std::initializer_list<Emitter> emitters) : impl{seq(emitters)} {}
Emitter::Emitter(std::vector<Emitter> const& emitters) : impl{seq(emitters)} {}

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
