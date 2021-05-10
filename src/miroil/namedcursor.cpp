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

#include "miroil/namedcursor.h"

miroil::NamedCursor::NamedCursor(char const* name)
: name(name) 
{
}

std::string const& miroil::NamedCursor::name() const 
{ 
    return name; 
}
    
void const* miroil::NamedCursor::as_argb_8888() const
{ 
    return nullptr; 
}

mir::geometry::Size miroil::NamedCursor::size() const
{ 
    return {0,0}; 
}

mir::geometry::Displacement miroil::NamedCursor::hotspot() const
{ 
    return {0,0}; 
}

