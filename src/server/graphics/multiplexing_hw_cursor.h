/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <vector>
#include <span>

#include "mir/graphics/cursor.h"

namespace mir::graphics
{
class Display;

class MultiplexingCursor : public Cursor
{
public:
    explicit MultiplexingCursor(std::span<Display*> platform_displays);

    void show(CursorImage const& image) override;
    void hide() override;
    void move_to(geometry::Point position) override;

    void set_scale(float) override
    {
    }

private:
    std::vector<std::shared_ptr<Cursor>> const platform_cursors;
};
}
