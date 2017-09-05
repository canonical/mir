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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_SCENE_ELEMENT_H_
#define MIR_TEST_DOUBLES_STUB_SCENE_ELEMENT_H_

#include "mir/compositor/scene_element.h"
#include "stub_renderable.h"

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

    StubSceneElement()
        : StubSceneElement(std::make_shared<StubRenderable>())
    {
    }

    std::shared_ptr<graphics::Renderable> renderable() const override
    {
        return renderable_;
    }

    void rendered() override
    {
    }

    void occluded() override
    {
    }

private:
    std::shared_ptr<graphics::Renderable> const renderable_;
};

}
}
}

#endif
