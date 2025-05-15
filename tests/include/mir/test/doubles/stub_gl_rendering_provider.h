/*
 * Copyright © Canonical Ltd.
 *
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

#ifndef MIR_TEST_DOUBLES_STUB_GL_RENDERING_PROVIDER_H_
#define MIR_TEST_DOUBLES_STUB_GL_RENDERING_PROVIDER_H_

#include "mir/graphics/platform.h"
#include "mir/test/doubles/mock_gl_buffer.h"
#include "mir/test/doubles/mock_output_surface.h"

namespace mir::test::doubles
{
class StubGlRenderingProvider : public graphics::GLRenderingProvider
{
public:
    auto as_texture(std::shared_ptr<graphics::Buffer> buffer) -> std::shared_ptr<graphics::gl::Texture> override
    {
        std::shared_ptr<graphics::NativeBufferBase> native_buffer{buffer, buffer->native_buffer_base()};
        if (auto existing_buf = std::dynamic_pointer_cast<graphics::gl::Texture>(std::move(native_buffer)))
        {
            return existing_buf;
        }
        auto tex_buf = std::make_shared<testing::NiceMock<MockTextureBuffer>>(
            geometry::Size{800, 500},
            geometry::Stride{-1},
            mir_pixel_format_argb_8888);

        ON_CALL(*tex_buf, shader(testing::_))
            .WillByDefault(testing::Invoke(
                [](auto& factory) -> mir::graphics::gl::Program&
                {
                    static int unused = 1;
                    return factory.compile_fragment_shader(&unused, "extension code", "fragment code");
                }));

        return tex_buf;
    }

    auto surface_for_sink(
        graphics::DisplaySink&,
        graphics::GLConfig const&) -> std::unique_ptr<graphics::gl::OutputSurface> override
    {
        return std::make_unique<testing::NiceMock<MockOutputSurface>>();
    }

    auto suitability_for_allocator(std::shared_ptr<graphics::GraphicBufferAllocator> const& /*sink*/)
        -> graphics::probe::Result override
    {
        return graphics::probe::supported;
    }

    auto suitability_for_display(graphics::DisplaySink& /*sink*/)
        -> graphics::probe::Result override
    {
        return graphics::probe::dummy; 
    }

    auto make_framebuffer_provider(graphics::DisplaySink& /*sink*/)
        -> std::unique_ptr<FramebufferProvider> override
    {
        class NullFramebufferProvider : public FramebufferProvider
        {
        public:
            auto buffer_to_framebuffer(std::shared_ptr<graphics::Buffer>)
                -> std::unique_ptr<graphics::Framebuffer> override
            {
                return {};
            }
        };
        return std::make_unique<NullFramebufferProvider>();
    }
};
}

#endif //MIR_TEST_DOUBLES_STUB_GL_RENDERING_PROVIDER_H_
