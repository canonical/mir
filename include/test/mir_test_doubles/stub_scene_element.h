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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SCENE_ELEMENT_H_
#define MIR_TEST_DOUBLES_STUB_SCENE_ELEMENT_H_

#include "mir/compositor/scene_element.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubSceneElement : public compositor::SceneElement
{
public:
    StubSceneElement(std::shared_ptr<graphics::Renderable> const& renderable)
        : renderable_{renderable}
    {
    }

    std::shared_ptr<graphics::Renderable> renderable() const
    {
        return renderable_;
    }

    void rendered_in(compositor::CompositorID) override
    {
    }

    void occluded_in(compositor::CompositorID) override
    {
    }

private:
    std::shared_ptr<graphics::Renderable> const renderable_;
};

}
}
}

#endif
