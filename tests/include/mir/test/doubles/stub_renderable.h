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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_RENDERABLE_H_
#define MIR_TEST_DOUBLES_STUB_RENDERABLE_H_

#include "mir/test/doubles/stub_buffer.h"
#include <mir/graphics/renderable.h>
#include <memory>
#define GLM_FORCE_RADIANS
#define GLM_PRECISION_MEDIUMP_FLOAT
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace mir
{
namespace test
{
namespace doubles
{

class StubRenderable : public graphics::Renderable
{
public:
    StubRenderable(std::shared_ptr<graphics::Buffer> const& buffer, geometry::Rectangle const& rect)
        : trans(1),
          rect(rect),
          stub_buffer(buffer)
    {}

    StubRenderable(std::shared_ptr<graphics::Buffer> const& buffer)
        : StubRenderable(buffer, {{},{}})
    {}


    StubRenderable(geometry::Rectangle const& rect)
        : StubRenderable(make_stub_buffer(rect), rect)
    {}

    StubRenderable() :
        StubRenderable(make_stub_buffer({{},{}}), {{},{}})
    {}

    void set_buffer(std::shared_ptr<graphics::Buffer> const& buffer)
    {
        stub_buffer = buffer;
    }

    ID id() const override
    {
        return this;
    }
    std::shared_ptr<graphics::Buffer> buffer() const override
    {
        return stub_buffer;
    }
    geometry::Rectangle screen_position() const override
    {
        return rect;
    }
    float alpha() const override
    {
        return 1.0f;
    }
    glm::mat4 transformation() const override
    {
        return trans;
    }
    bool shaped() const override
    {
        return false;
    }
    unsigned int swap_interval() const override
    {
        return 1;
    }

private:
    std::shared_ptr<graphics::Buffer> make_stub_buffer(geometry::Rectangle const& rect)
    {
        graphics::BufferProperties prop{
            rect.size, mir_pixel_format_abgr_8888, graphics::BufferUsage::hardware};
        return std::make_shared<StubBuffer>(prop);
    }

    glm::mat4 trans;
    geometry::Rectangle const rect;
    std::shared_ptr<graphics::Buffer> stub_buffer;
};

struct StubTransformedRenderable : public StubRenderable
{
    StubTransformedRenderable()
    {
    }

    StubTransformedRenderable(
        std::shared_ptr<graphics::Buffer> const& buffer,
        geometry::Rectangle const& rect) :
        StubRenderable(buffer, rect)
    {
    }

    glm::mat4 transformation() const override
    {
        glm::mat4 transform(1.0);
        glm::vec3 vec(1.0, 0.0, 0.0);
        transform = glm::rotate(transform, 33.0f, vec);
        return transform;
    }
};

//hopefully the alpha representation gets condensed at some point
struct StubShapedRenderable : public StubRenderable
{
    bool shaped() const override
    {
        return true;
    }
};

struct StubTranslucentRenderable : public StubRenderable
{
    bool shaped() const override
    {
        return true;
    }
};

struct PlaneAlphaRenderable : public StubRenderable
{
    float alpha() const override
    {
        //approx 99% alpha 
        return 1.0f - ( 3.0f / 1024.0f );
    }
};

struct IntervalZeroRenderable : StubRenderable
{
    IntervalZeroRenderable() = default;
    IntervalZeroRenderable(
        std::shared_ptr<graphics::Buffer> const& buffer, geometry::Rectangle const& rect) :
        StubRenderable(buffer, rect)
    {
    }

    unsigned int swap_interval() const override
    {
        return 0;
    }
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_RENDERABLE_H_ */
