/*
 * Copyright © 2013 Canonical Ltd.
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

#ifndef MIR_TEST_DOUBLES_FAKE_RENDERABLE_H_
#define MIR_TEST_DOUBLES_FAKE_RENDERABLE_H_

#include "stub_buffer.h"
#include "mir/graphics/renderable.h"
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{

class FakeRenderable : public graphics::Renderable
{
public:
    FakeRenderable(int x, int y, int width, int height)
        : FakeRenderable{geometry::Rectangle{{x,y},{width,height}}}
    {
    }
    FakeRenderable(geometry::Rectangle display_area)
        : FakeRenderable{display_area, 1.0f, true, true, true}
    {
    }

    FakeRenderable(geometry::Rectangle display_area,
                   float opacity)
        : FakeRenderable{display_area, opacity, true, true, true}
    {
    }

    FakeRenderable(geometry::Rectangle display_area,
                   float opacity,
                   bool rectangular,
                   bool visible)
        : FakeRenderable{display_area, opacity, rectangular, visible, true}
    {
    }

    FakeRenderable(geometry::Rectangle display_area,
                   float opacity,
                   bool rectangular,
                   bool visible,
                   bool posted)
        : buf{std::make_shared<StubBuffer>()},
          rect(display_area),
          opacity(opacity),
          rectangular(rectangular),
          visible_(visible),
          posted(posted)
    {
    }

    ID id() const override
    {
        return this;
    }

    float alpha() const override
    {
        return opacity;
    }

    glm::mat4 transformation() const override
    {
        return glm::mat4();
    }

    bool visible() const override
    {
        return visible_ && posted;
    }

    bool shaped() const override
    {
        return !rectangular;
    }

    void set_buffer(std::shared_ptr<graphics::Buffer> b)
    {
        buf = b;
    }

    std::shared_ptr<graphics::Buffer> buffer() const override
    {
        return buf;
    }

    bool alpha_enabled() const override
    {
        return shaped() || alpha() < 1.0f;
    }

    geometry::Rectangle screen_position() const override
    {
        return rect;
    }

    int buffers_ready_for_compositor() const override
    {
        return 1;
    }

private:
    std::shared_ptr<graphics::Buffer> buf;
    mir::geometry::Rectangle rect;
    float opacity;
    bool rectangular;
    bool visible_;
    bool posted;
};

} // namespace doubles
} // namespace test
} // namespace mir
#endif // MIR_TEST_DOUBLES_FAKE_RENDERABLE_H_
