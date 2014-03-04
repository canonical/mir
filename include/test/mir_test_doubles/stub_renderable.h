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
    std::shared_ptr<graphics::Buffer> buffer(unsigned long) const override
    {
        return {};
    }
    bool alpha_enabled() const
    {
        return false;
    }
    geometry::Rectangle screen_position() const
    {
        return {{},{}};
    }
    float alpha() const override
    {
        return 1.0f;
    }
    glm::mat4 const& transformation() const override
    {
        return trans;
    }
    bool should_be_rendered_in(geometry::Rectangle const&) const override
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
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_RENDERABLE_H_ */
