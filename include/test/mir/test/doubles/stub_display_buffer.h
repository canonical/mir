/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_BUFFER_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_BUFFER_H_

#include "mir/test/doubles/null_display_buffer.h"
#include "mir/geometry/rectangle.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubDisplayBuffer : public NullDisplayBuffer
{
public:
    StubDisplayBuffer(geometry::Rectangle const& view_area_) : view_area_(view_area_) {}
    StubDisplayBuffer(StubDisplayBuffer const& s) : view_area_(s.view_area_) {}
    geometry::Rectangle view_area() const override { return view_area_; }

private:
    geometry::Rectangle view_area_;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_BUFFER_H_ */
