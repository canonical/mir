/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_RENDERABLE_H_
#define MIR_TEST_DOUBLES_MOCK_RENDERABLE_H_

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
    MOCK_CONST_METHOD1(buffer, std::shared_ptr<graphics::Buffer>(unsigned long));
    MOCK_CONST_METHOD0(alpha_enabled, bool());
    MOCK_CONST_METHOD0(screen_position, geometry::Rectangle());
    MOCK_CONST_METHOD0(alpha, float());
    MOCK_CONST_METHOD0(transformation, glm::mat4());
    MOCK_CONST_METHOD1(should_be_rendered_in, bool(geometry::Rectangle const& rect));
    MOCK_CONST_METHOD0(shaped, bool());
    int buffers_ready_for_compositor() const override { return 1; }
};
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_RENDERABLE_H_ */
