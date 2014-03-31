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


#ifndef MIR_GRAPHICS_BUILTIN_CURSOR_LOADER_H_
#define MIR_GRAPHICS_BUILTIN_CURSOR_LOADER_H_

#include "mir/graphics/cursor_loader.h"

namespace mir
{
namespace graphics
{
class CursorImage;

class BuiltinCursorLoader : public CursorLoader
{
public:
    BuiltinCursorLoader();
    virtual ~BuiltinCursorLoader() = default;

    std::shared_ptr<CursorImage> lookup_cursor(std::string const& theme_name,
                                               std::string const& cursor_name,
                                               geometry::Size const& size);

protected:
    BuiltinCursorLoader(BuiltinCursorLoader const&) = delete;
    BuiltinCursorLoader& operator=(BuiltinCursorLoader const&) = delete;

private:
    std::shared_ptr<CursorImage> const builtin_image;
};
}
}


#endif /* MIR_GRAPHICS_BUILTIN_CURSOR_LOADER_H_ */
