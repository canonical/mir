/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_NESTED_CURSOR_H_
#define MIR_GRAPHICS_NESTED_CURSOR_H_

#include "mir/graphics/cursor.h"

namespace mir
{
namespace graphics
{
namespace nested
{
class HostConnection;
class Cursor : public graphics::Cursor
{
public:
    Cursor(std::shared_ptr<HostConnection> const& host_connection, std::shared_ptr<CursorImage> const& default_image);
    ~Cursor() = default;

    void show(CursorImage const& image) override;
    void show() override;
    
    void hide() override;

    void move_to(geometry::Point position) override;
private:
    std::shared_ptr<HostConnection> const connection;
    std::shared_ptr<CursorImage> const default_image;
};
}
}
}

#endif // MIR_GRAPHICS_NESTED_CURSOR_H_
