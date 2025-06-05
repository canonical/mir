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

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_SCENE_H
#define MIR_TEST_DOUBLES_MOCK_INPUT_SCENE_H
#include "stub_input_scene.h"

namespace mir
{
namespace test
{
namespace doubles
{
struct MockInputScene : StubInputScene
{
    MOCK_METHOD1(add_input_visualization,
                 void(std::shared_ptr<graphics::Renderable> const&));

    MOCK_METHOD1(remove_input_visualization,
                 void(std::weak_ptr<graphics::Renderable> const&));

    MOCK_METHOD0(emit_scene_changed, void());

    MOCK_CONST_METHOD0(screen_is_locked, bool());
};
}
}
}

#endif //MIR_TEST_DOUBLES_
