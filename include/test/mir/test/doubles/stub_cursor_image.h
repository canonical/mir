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

#ifndef MIR_TEST_DOUBLES_STUB_CURSOR_IMAGE_H_
#define MIR_TEST_DOUBLES_STUB_CURSOR_IMAGE_H_

#include <mir/graphics/cursor_image.h>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubCursorImage : public mir::graphics::CursorImage
{
    void const* as_argb_8888() const override { return nullptr; }
    geometry::Size size() const override { return geometry::Size{16, 16}; }
    geometry::Displacement hotspot() const override { return geometry::Displacement{0, 0}; }
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_CURSOR_H_ */
