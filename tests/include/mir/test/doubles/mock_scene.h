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
        ON_CALL(*this, scene_elements_for(testing::_))
            .WillByDefault(testing::Return(compositor::SceneElementSequence{}));
    }

    MOCK_METHOD(compositor::SceneElementSequence, scene_elements_for, (compositor::CompositorID), (override));
    MOCK_METHOD(void, register_compositor, (compositor::CompositorID), (override));
    MOCK_METHOD(void, unregister_compositor, (compositor::CompositorID), (override));

    MOCK_METHOD(void, add_observer, (std::shared_ptr<scene::Observer> const&), (override));
    MOCK_METHOD(void, remove_observer, (std::weak_ptr<scene::Observer> const&), (override));
};

} // namespace doubles
} // namespace test
} // namespace mir

#endif /* MIR_TEST_DOUBLES_MOCK_SCENE_H_ */
