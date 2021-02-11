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

#ifndef MIROIL_NAMEDCURSOR_H
#define MIROIL_NAMEDCURSOR_H
#include <string>
#include <mir/geometry/size.h>
#include <mir/graphics/cursor_image.h>

namespace miroil {

/*
    A hack for storing a named cursor inside a CursorImage, as a way to delegate cursor loading to shell code.
*/
class NamedCursor : public mir::graphics::CursorImage
{
public:
    NamedCursor(const char *name)
        : m_name(name) {}

    const std::string &name() const { return m_name; }
    
    const void *as_argb_8888() const override { return nullptr; }
    mir::geometry::Size size() const override { return {0,0}; }
    mir::geometry::Displacement hotspot() const override { return {0,0}; }

private:
    std::string m_name;
};

} // namespace miroil

#endif // MIROIL_NAMEDCURSOR_H
