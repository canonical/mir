/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_DISPLAY_BUFFER_H_
#define MIR_TEST_DOUBLES_MOCK_DISPLAY_BUFFER_H_

#include <mir/graphics/display_sink.h>
#include <mir/graphics/platform.h>

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockDisplaySink : public graphics::DisplaySink
{
public:
    MockDisplaySink()
    {
        using namespace testing;
        ON_CALL(*this, view_area())
            .WillByDefault(Return(geometry::Rectangle{{0,0},{0,0}}));
    }
    MOCK_METHOD(geometry::Rectangle, view_area, (), (const override));
    MOCK_METHOD(geometry::Size, pixel_size, (), (const override));
    MOCK_METHOD(bool, overlay, (std::vector<graphics::DisplayElement> const&), (override));
    MOCK_METHOD(void, set_next_image, (std::unique_ptr<graphics::Framebuffer>), (override));
    MOCK_METHOD(glm::mat2, transformation, (), (const override));
    MOCK_METHOD(graphics::DisplayAllocator*, maybe_create_allocator, (graphics::DisplayAllocator::Tag const&), (override));
};

}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_DISPLAY_BUFFER_H_ */
