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

#ifndef QTMIR_MIRCURSORIMAGES_H_
#define QTMIR_MIRCURSORIMAGES_H_

#include <mir/input/cursor_images.h>

namespace qtmir {

class MirCursorImages : public mir::input::CursorImages
{
public:
    std::shared_ptr<mir::graphics::CursorImage> image(const std::string &cursor_name,
            const mir::geometry::Size &size) override;
};

}

#endif // QTMIR_MIRCURSORIMAGES_H_
