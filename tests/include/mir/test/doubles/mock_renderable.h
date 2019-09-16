/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
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
            .WillByDefault(testing::Return(std::experimental::optional<geometry::Rectangle>()));
        ON_CALL(*this, buffer())
            .WillByDefault(testing::Return(std::make_shared<StubBuffer>()));
        ON_CALL(*this, alpha())
            .WillByDefault(testing::Return(1.0f));
        ON_CALL(*this, transformation())
            .WillByDefault(testing::Return(glm::mat4{}));
        ON_CALL(*this, visible())
            .WillByDefault(testing::Return(true));
    }

    MOCK_CONST_METHOD0(id, ID());
    MOCK_CONST_METHOD0(buffer, std::shared_ptr<graphics::Buffer>());
    MOCK_CONST_METHOD0(screen_position, geometry::Rectangle());
    MOCK_CONST_METHOD0(clip_area, std::experimental::optional<geometry::Rectangle>());
    MOCK_CONST_METHOD0(alpha, float());
    MOCK_CONST_METHOD0(transformation, glm::mat4());
    MOCK_CONST_METHOD0(visible, bool());
    MOCK_CONST_METHOD0(shaped, bool());
    MOCK_CONST_METHOD0(swap_interval, unsigned int());
};
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_RENDERABLE_H_ */
