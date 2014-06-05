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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_RENDERABLE_H_
#define MIR_TEST_DOUBLES_STUB_RENDERABLE_H_

#include "mir_test_doubles/stub_buffer.h"
#include <mir/graphics/renderable.h>
#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

class StubRenderable : public graphics::Renderable
{
public:
    StubRenderable(geometry::Rectangle const& rect)
        : rect(rect)
    {}
    StubRenderable() :
        StubRenderable({{},{}})
    {}
    ID id() const override
    {
        return this;
    }
    std::shared_ptr<graphics::Buffer> buffer() const override
    {
        graphics::BufferProperties prop{rect.size, mir_pixel_format_abgr_8888, graphics::BufferUsage::hardware};
        return std::make_shared<StubBuffer>(prop);
    }
    bool alpha_enabled() const
    {
        return false;
    }
    geometry::Rectangle screen_position() const
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
    bool visible() const
    {
        return true;
    }
    bool shaped() const override
    {
        return false;
    }

    int buffers_ready_for_compositor() const override
    {
        return 1;
    }

private:
    glm::mat4 trans;
    geometry::Rectangle const rect;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_RENDERABLE_H_ */
