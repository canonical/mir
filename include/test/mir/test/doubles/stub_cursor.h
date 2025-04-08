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

#ifndef MIR_TEST_DOUBLES_STUB_CURSOR_H_
#define MIR_TEST_DOUBLES_STUB_CURSOR_H_

#include "mir/graphics/cursor.h"
#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubCursor : public graphics::Cursor
{
    void show(std::shared_ptr<graphics::CursorImage> const&) override {}
    void hide() override {}
    void move_to(geometry::Point) override {}
    void scale(float) override {}
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_CURSOR_H_ */
