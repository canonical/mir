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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_RENDERER_H_
#define MIR_TEST_DOUBLES_STUB_RENDERER_H_

#include "mir/compositor/renderer.h"

namespace mir
{
namespace test
{
namespace doubles
{

class StubRenderer : public compositor::Renderer
{
public:
    void set_viewport(geometry::Rectangle const&) override
    {
    }

    void set_rotation(float) override
    {
    }

    void begin() const override
    {
    }

    void render(graphics::Renderable const&,
                graphics::Buffer&) const override
    {
    }

    void end() const override
    {
    }

    void suspend() override
    {
    }
};


} // namespace doubles
} // namespace test
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_RENDERER_H_
