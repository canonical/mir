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


#ifndef MIR_GRAPHICS_BUILTIN_CURSOR_REPOSITORY_H_
#define MIR_GRAPHICS_BUILTIN_CURSOR_REPOSITORY_H_

#include "mir/graphics/cursor_repository.h"

namespace mir
{
namespace graphics
{
class CursorImage;

class BuiltinCursorRepository : public CursorRepository
{
public:
    BuiltinCursorRepository() = default;
    virtual ~BuiltinCursorRepository() = default;

    // TODO: Document
    std::shared_ptr<CursorImage> lookup_cursor(std::string const& theme_name,
                                               std::string const& cursor_name);

protected:
    BuiltinCursorRepository(BuiltinCursorRepository const&) = delete;
    BuiltinCursorRepository& operator=(BuiltinCursorRepository const&) = delete;
};
}
}


#endif /* MIR_GRAPHICS_BUILTIN_CURSOR_REPOSITORY_H_ */
