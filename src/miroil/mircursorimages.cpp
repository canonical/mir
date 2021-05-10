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

#include "mircursorimages.h"
#include "namedcursor.h"

using namespace qtmir;

std::shared_ptr<mir::graphics::CursorImage> MirCursorImages::image(const std::string &cursor_name,
        const mir::geometry::Size &)
{
    // We are not responsible for loading cursors. This is left for shell to do as it's drawing its own QML cursor.
    // So here we work around Mir API by storing just the cursor name in the CursorImage.
    return std::make_shared<NamedCursor>(cursor_name.c_str());
}
