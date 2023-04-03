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

#ifndef MIR_TEST_DOUBLES_STUB_RENDERER_H_
#define MIR_TEST_DOUBLES_STUB_RENDERER_H_

#include "mir/renderer/renderer.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer.h"
#include <thread>

namespace mir
{
namespace test
{
namespace doubles
{

class StubRenderer : public renderer::Renderer
{
public:
    void set_viewport(geometry::Rectangle const&) override {}
    void set_output_transform(glm::mat2 const&) override {}
    void suspend() override {}

    auto render(graphics::RenderableList const& renderables) const -> std::unique_ptr<graphics::Framebuffer> override
    {
        for (auto const& r : renderables)
            r->buffer(); // We need to consume a buffer to unblock client tests
        // Yield to reduce runtime under valgrind
        std::this_thread::yield();

        return {};
    }
};


} // namespace doubles
} // namespace test
} // namespace mir

#endif // MIR_TEST_DOUBLES_STUB_RENDERER_H_
