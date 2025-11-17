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

#ifndef MIR_TEST_DOUBLES_MOCK_RENDERABLE_H_
#define MIR_TEST_DOUBLES_MOCK_RENDERABLE_H_

#include "mir/test/doubles/stub_buffer.h"
#include <mir/graphics/renderable.h>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockRenderable : public graphics::Renderable
{
    MockRenderable()
    {
        ON_CALL(*this, screen_position())
            .WillByDefault(testing::Return(geometry::Rectangle{{},{}}));
        ON_CALL(*this, clip_area())
            .WillByDefault(testing::Return(std::optional<geometry::Rectangle>()));
        ON_CALL(*this, buffer())
            .WillByDefault(testing::Return(std::make_shared<StubBuffer>()));
        ON_CALL(*this, alpha())
            .WillByDefault(testing::Return(1.0f));
        ON_CALL(*this, transformation())
            .WillByDefault(testing::Return(glm::mat4{}));
        ON_CALL(*this, visible())
            .WillByDefault(testing::Return(true));
    }

    MOCK_METHOD(ID, id, (), (const, override));
    MOCK_METHOD(std::shared_ptr<graphics::Buffer>, buffer, (), (const, override));
    MOCK_METHOD(geometry::Rectangle, screen_position, (), (const, override));
    MOCK_METHOD(geometry::RectangleD, src_bounds, (), (const, override));
    MOCK_METHOD(std::optional<geometry::Rectangle>, clip_area, (), (const, override));
    MOCK_METHOD(float, alpha, (), (const, override));
    MOCK_METHOD(glm::mat4, transformation, (), (const, override));
    MOCK_METHOD(MirOrientation, orientation, (), (const, override));
    MOCK_METHOD(MirMirrorMode, mirror_mode, (), (const, override));
    MOCK_METHOD(bool, visible, (), (const, override));
    MOCK_METHOD(bool, shaped, (), (const, override));
    MOCK_METHOD(std::optional<mir::scene::Surface const*>, surface_if_any, (), (const, override));
    MOCK_METHOD(std::optional<geometry::Rectangles>, opaque_region, (), (const, override));
};
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_RENDERABLE_H_ */
