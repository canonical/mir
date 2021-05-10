/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "miroil/named_cursor.h"

miroil::NamedCursor::NamedCursor(char const* name)
: cursor_name(name) 
{
}

miroil::NamedCursor::~NamedCursor() = default;

auto miroil::NamedCursor::name() const 
-> std::string const&
{ 
    return cursor_name; 
}
    
auto miroil::NamedCursor::as_argb_8888() const
-> void const*
{ 
    return nullptr; 
}

auto miroil::NamedCursor::size() const
-> mir::geometry::Size
{ 
    return {0,0}; 
}

auto miroil::NamedCursor::hotspot() const
-> mir::geometry::Displacement
{ 
    return {0,0}; 
}
