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

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_BUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_BUFFER_H_

#include <mir/graphics/display_buffer.h>

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockDisplayBuffer : public graphics::DisplayBuffer
{
public:
    MockDisplayBuffer()
    {
        using namespace testing;
        ON_CALL(*this, view_area())
            .WillByDefault(Return(geometry::Rectangle{{0,0},{0,0}}));
    }
    MOCK_CONST_METHOD0(view_area, geometry::Rectangle());
    MOCK_METHOD0(make_current, void());
    MOCK_METHOD0(release_current, void());
    MOCK_METHOD0(gl_swap_buffers, void());
    MOCK_METHOD1(post_renderables_if_optimizable, bool(graphics::RenderableList const&));
    MOCK_CONST_METHOD0(orientation, MirOrientation());
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_BUFFER_H_ */
