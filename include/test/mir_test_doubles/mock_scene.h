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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_SCENE_H_
#define MIR_TEST_DOUBLES_MOCK_SCENE_H_

#include "mir/compositor/scene.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class MockScene : public compositor::Scene
{
public:
    MockScene()
    {
        ON_CALL(*this, renderable_list_for(testing::_))
            .WillByDefault(testing::Return(graphics::RenderableList{}));
    }

    MOCK_CONST_METHOD1(renderable_list_for, graphics::RenderableList(void const*));
    MOCK_METHOD1(set_change_callback, void(std::function<void()> const&));

    MOCK_METHOD1(add_observer, void(std::shared_ptr<scene::Observer> const&));
    MOCK_METHOD1(remove_observer, void(std::weak_ptr<scene::Observer> const&));
};

} // namespace doubles
} // namespace test
} // namespace mir

#endif /* MIR_TEST_DOUBLES_MOCK_SCENE_H_ */
