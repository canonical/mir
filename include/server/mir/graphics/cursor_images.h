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


#ifndef MIR_GRAPHICS_CURSOR_LOADER_H_
#define MIR_GRAPHICS_CURSOR_LOADER_H_

#include "mir/geometry/size.h"

#include <memory>
#include <string>

namespace mir
{
namespace graphics
{
class CursorImage;

/// CursorImages is used to lookup cursor images.
class CursorImages
{
public:
    virtual ~CursorImages() = default;

    /// Looks up the image for a named cursor. Cursor names
    /// follow the XCursor naming conventions.
    virtual std::shared_ptr<CursorImage> image(std::string const& cursor_name,
        geometry::Size const& size) = 0;

protected:
    CursorImages() = default;
    CursorImages(CursorImages const&) = delete;
    CursorImages& operator=(CursorImages const&) = delete;
};
}
}

#endif /* MIR_GRAPHICS_CURSOR_LOADER_H_ */
