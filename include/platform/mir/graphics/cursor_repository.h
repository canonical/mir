/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */


#ifndef MIR_GRAPHICS_CURSOR_REPOSITORY_H_
#define MIR_GRAPHICS_CURSOR_REPOSITORY_H_

#include <memory>
#include <string>

namespace mir
{
namespace graphics
{
class CursorImage;

/// CursorRepository is used to lookup cursor images within cursor themes.
class CursorRepository
{
public:
    /// Looks up the image for a named cursor in a given cursor theme. Cursor names
    /// follow the XCursor naming conventions.
    virtual std::shared_ptr<CursorImage> lookup_cursor(std::string const& theme_name,
                                                       std::string const& cursor_name) = 0;

protected:
    CursorRepository() = default;
    virtual ~CursorRepository() = default;
    CursorRepository(CursorRepository const&) = delete;
    CursorRepository& operator=(CursorRepository const&) = delete;
};
}
}


#endif /* MIR_GRAPHICS_CURSOR_REPOSITORY_H_ */
