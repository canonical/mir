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
        ON_CALL(*this, scene_elements_for(testing::_, testing::_))
            .WillByDefault(testing::Return(compositor::SceneElementSequence{}));
        ON_CALL(*this, frames_pending(testing::_))
            .WillByDefault(testing::Return(0));
    }

    MOCK_METHOD2(scene_elements_for, compositor::SceneElementSequence(compositor::CompositorID, geometry::Rectangle));
    MOCK_CONST_METHOD1(frames_pending, int(compositor::CompositorID));
    MOCK_METHOD1(register_compositor, void(compositor::CompositorID));
    MOCK_METHOD1(unregister_compositor, void(compositor::CompositorID));

    MOCK_METHOD1(add_observer, void(std::shared_ptr<scene::Observer> const&));
    MOCK_METHOD1(remove_observer, void(std::weak_ptr<scene::Observer> const&));
};

} // namespace doubles
} // namespace test
} // namespace mir

#endif /* MIR_TEST_DOUBLES_MOCK_SCENE_H_ */
