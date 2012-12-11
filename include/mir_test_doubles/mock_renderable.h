/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/graphics/renderable.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockRenderable :  public graphics::Renderable
{
public:
    MOCK_CONST_METHOD0(top_left, geometry::Point());
    MOCK_CONST_METHOD0(size, geometry::Size());
    MOCK_CONST_METHOD0(texture, std::shared_ptr<compositor::GraphicRegion>());
    MOCK_CONST_METHOD0(transformation, glm::mat4());
    MOCK_CONST_METHOD0(alpha, float());
    MOCK_CONST_METHOD0(hidden, bool());
};

}
}
}
#endif /* MIR_TEST_DOUBLES_MOCK_RENDERABLE_H_ */
