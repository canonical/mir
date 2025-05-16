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

#ifndef MIR_TEST_DOUBLES_STUB_INPUT_SCENE_H_
#define MIR_TEST_DOUBLES_STUB_INPUT_SCENE_H_

#include "mir/input/scene.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubInputScene : public input::Scene
{
    auto input_surface_at(geometry::Point) const -> std::shared_ptr<input::Surface> override
    {
        return nullptr;
    }
    void add_observer(std::shared_ptr<scene::Observer> const& /* observer */) override
    {
    }
    void remove_observer(std::weak_ptr<scene::Observer> const& /* observer */) override
    {
    }

    void add_input_visualization(std::shared_ptr<graphics::Renderable> const& /* overlay */) override
    {
    }

    void add_bottom_input_visualization(std::shared_ptr<graphics::Renderable> const& /* overlay */) override
    {
    }

    void remove_input_visualization(std::weak_ptr<graphics::Renderable> const& /* overlay */) override
    {
    }
    
    void emit_scene_changed() override
    {
    }

    bool screen_is_locked() const override
    {
        return false;
    }
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_STUB_INPUT_SCENE_H_ */
