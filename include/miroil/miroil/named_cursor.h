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

#ifndef MIROIL_NAMED_CURSOR_H
#define MIROIL_NAMED_CURSOR_H
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
    NamedCursor(const char *name);
    ~NamedCursor();

    auto as_argb_8888() const -> void const* override;
    auto hotspot() const -> mir::geometry::Displacement override;
    auto name() const -> std::string const&;
    auto size() const -> mir::geometry::Size override;

private:
    std::string cursor_name;
};

} // namespace miroil

#endif // MIROIL_NAMED_CURSOR_H
