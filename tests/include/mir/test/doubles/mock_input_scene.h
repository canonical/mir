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

#include "mir/input/scene.h"
#include <gmock/gmock.h>

namespace mir::test::doubles
{
class MockInputScene : public input::Scene
{
public:
    MOCK_METHOD(std::shared_ptr<input::Surface>, input_surface_at,
        (geometry::Point point), (const, override));

    MOCK_METHOD(void, add_observer,
        (std::shared_ptr<scene::Observer> const& observer), (override));

    MOCK_METHOD(void, remove_observer,
        (std::weak_ptr<scene::Observer> const& observer), (override));

    MOCK_METHOD(void, add_input_visualization,
        (std::shared_ptr<graphics::Renderable> const& overlay), (override));

    MOCK_METHOD(void, prepend_input_visualization,
        (std::shared_ptr<graphics::Renderable> const& overlay), (override));

    MOCK_METHOD(void, remove_input_visualization,
        (std::weak_ptr<graphics::Renderable> const& overlay), (override));

    MOCK_METHOD(void, emit_scene_changed, (), (override));

    MOCK_METHOD(bool, screen_is_locked, (), (const, override));
};
}

#endif //MIR_TEST_DOUBLES_MOCK_INPUT_SCENE_H
