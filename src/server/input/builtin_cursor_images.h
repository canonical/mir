/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */


#ifndef MIR_INPUT_BUILTIN_CURSOR_IMAGES_H_
#define MIR_INPUT_BUILTIN_CURSOR_IMAGES_H_

#include "mir/input/cursor_images.h"

namespace mir
{
namespace input
{
class BuiltinCursorImages : public CursorImages
{
public:
    BuiltinCursorImages();
    virtual ~BuiltinCursorImages() = default;

    std::shared_ptr<graphics::CursorImage> image(std::string const& cursor_name,
        geometry::Size const& size);

protected:
    BuiltinCursorImages(BuiltinCursorImages const&) = delete;
    BuiltinCursorImages& operator=(BuiltinCursorImages const&) = delete;

private:
    std::shared_ptr<graphics::CursorImage> const builtin_image;
};
}
}


#endif /* MIR_INPUT_BUILTIN_CURSOR_IMAGES_H_ */
