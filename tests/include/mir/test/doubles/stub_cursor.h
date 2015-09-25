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

#ifndef MIR_TEST_DOUBLES_STUB_CURSOR_H_
#define MIR_TEST_DOUBLES_STUB_CURSOR_H_

#include "mir/graphics/cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/input/cursor_images.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubCursor : public graphics::Cursor
{
    void show() override {}
    void show(graphics::CursorImage const&) override {}
    void hide() override {}
    void move_to(geometry::Point) override {}
};

struct StubCursorImage : public graphics::CursorImage
{
    StubCursorImage(std::string const& /* name */)
    { }

    void const* as_argb_8888() const override { return this; }
    geometry::Size size() const  override { return geometry::Size{}; }
    geometry::Displacement hotspot() const  override{ return geometry::Displacement{0, 0}; }
};

struct StubCursorImages : public input::CursorImages
{
   std::shared_ptr<graphics::CursorImage>
       image(std::string const& name, geometry::Size const& /* size */) override
   {
       return std::make_shared<StubCursorImage>(name);
   }
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_CURSOR_H_ */
