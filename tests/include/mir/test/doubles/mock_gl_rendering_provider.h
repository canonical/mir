/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_MOCK_GL_RENDERING_PROVIDER_H_
#define MIR_TEST_DOUBLES_MOCK_GL_RENDERING_PROVIDER_H_

#include <gmock/gmock.h>

#include "mir/graphics/gl_config.h"
#include "mir/graphics/platform.h"

namespace mir::test::doubles
{
class MockGlRenderingProvider : public graphics::GLRenderingProvider
{
public:
    MOCK_METHOD(std::shared_ptr<graphics::gl::Texture>, as_texture, (std::shared_ptr<graphics::Buffer>), (override));
    MOCK_METHOD(
        std::unique_ptr<graphics::gl::OutputSurface>,
        surface_for_output,
        (graphics::DisplayBuffer&, geometry::Size, graphics::GLConfig const&),
        (override));
    MOCK_METHOD(
        graphics::probe::Result,
        suitability_for_allocator,
        (std::shared_ptr<graphics::GraphicBufferAllocator> const&),
        (override));
    MOCK_METHOD(
        graphics::probe::Result,
        suitability_for_display,
        (graphics::DisplayBuffer&),
        (override));
    MOCK_METHOD(
        std::unique_ptr<FramebufferProvider>,
        make_framebuffer_provider,
        (graphics::DisplayBuffer&),
        (override));
};
}

#endif //MIR_TEST_DOUBLES_MOCK_GL_RENDERING_PROVIDER_H_
