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

#include <mir/renderers/blitter/renderer.h>

#include <mir/graphics/buffer.h>
#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/drm_formats.h>
#include <mir/graphics/gl_config.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/ptr_backed_mapping.h>
#include <mir/graphics/rendering_providers.h>
#include <mir/test/doubles/mock_egl.h>
#include <mir/test/doubles/mock_gl.h>
#include <mir/test/doubles/mock_renderable.h>
#include <mir/test/doubles/stub_buffer.h>
#include <mir/test/doubles/stub_gl_config.h>

#include <drm_fourcc.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mrb = mir::renderer::blitter;
namespace mtd = mir::test::doubles;

namespace
{
auto output_size() -> geom::Size
{
    return {100, 100};
}

auto viewport() -> geom::Rectangle
{
    return {{0, 0}, output_size()};
}

auto default_renderable_rect() -> geom::Rectangle
{
    return {{10, 20}, {30, 40}};
}

auto to_src_bounds(geom::Rectangle const& rect) -> geom::RectangleD
{
    return {{0.0, 0.0}, {rect.size.width.as_value(), rect.size.height.as_value()}};
}

MATCHER_P(IsSameBufferAs, buffer, "")
{
    return &arg == buffer.get();
}

class StubDMABufBuffer : public mg::DMABufBuffer
{
public:
    explicit StubDMABufBuffer(geom::Size size)
        : buffer_size{size},
          buffer_planes{{mir::Fd{}, static_cast<uint32_t>(size.width.as_int() * 4), 0}}
    {
    }

    auto format() const -> mg::DRMFormat override
    {
        return mg::DRMFormat{DRM_FORMAT_ARGB8888};
    }

    auto modifier() const -> std::optional<uint64_t> override
    {
        return std::nullopt;
    }

    auto planes() const -> std::vector<PlaneDescriptor> const& override
    {
        return buffer_planes;
    }

    auto layout() const -> mg::gl::Texture::Layout override
    {
        return mg::gl::Texture::Layout::GL;
    }

    auto size() const -> geom::Size override
    {
        return buffer_size;
    }

private:
    geom::Size const buffer_size;
    std::vector<PlaneDescriptor> const buffer_planes;
};

class StubMappableFB : public mg::CPUAddressableDisplayAllocator::MappableFB
{
public:
    explicit StubMappableFB(geom::Size size)
        : buffer_size{size},
          stride_{size.width.as_int() * 4},
          pixels(static_cast<size_t>(size.height.as_int() * stride_.as_int())),
          dmabuf{size}
    {
    }

    auto size() const -> geom::Size override
    {
        return buffer_size;
    }

    auto as_dmabuf() -> mg::DMABufBuffer const* override
    {
        return &dmabuf;
    }

    auto map_writeable() -> std::unique_ptr<mir::renderer::software::Mapping<std::byte>> override
    {
        return std::make_unique<mg::PtrBackedMapping<std::byte>>(
            pixels.data(),
            mir_pixel_format_argb_8888,
            buffer_size,
            stride_);
    }

    auto format() const -> MirPixelFormat override
    {
        return mir_pixel_format_argb_8888;
    }

    auto stride() const -> geom::Stride override
    {
        return stride_;
    }

private:
    geom::Size const buffer_size;
    geom::Stride const stride_;
    std::vector<std::byte> pixels;
    StubDMABufBuffer const dmabuf;
};

class StubCPUAddressableDisplayAllocator : public mg::CPUAddressableDisplayAllocator
{
public:
    StubCPUAddressableDisplayAllocator()
    {
        using testing::_;
        using testing::Return;

        ON_CALL(*this, supported_formats())
            .WillByDefault(Return(std::vector<mg::DRMFormat>{mg::DRMFormat{DRM_FORMAT_ARGB8888}}));
        ON_CALL(*this, output_size())
            .WillByDefault(Return(::output_size()));
        ON_CALL(*this, alloc_fb(_))
            .WillByDefault([](mg::DRMFormat)
                {
                    return std::make_unique<StubMappableFB>(::output_size());
                });
    }

    MOCK_METHOD((std::vector<mg::DRMFormat>), supported_formats, (), (const, override));
    MOCK_METHOD(
        (std::unique_ptr<mg::CPUAddressableDisplayAllocator::MappableFB>),
        alloc_fb,
        (mg::DRMFormat format),
        (override));
    MOCK_METHOD(geom::Size, output_size, (), (const, override));
};

class NullFramebufferProvider : public mg::RenderingProvider::FramebufferProvider
{
public:
    auto buffer_to_framebuffer(std::shared_ptr<mg::Buffer>) -> std::unique_ptr<mg::Framebuffer> override
    {
        return nullptr;
    }
};

class MockBlitterRenderingProvider : public mg::BlitterRenderingProvider
{
public:
    class TrackingTask : public Task
    {
    public:
        explicit TrackingTask(std::unique_ptr<Surface> surface)
            : surface{std::move(surface)}
        {
        }

        std::unique_ptr<Surface> surface;
    };

    MockBlitterRenderingProvider()
    {
        using testing::_;
        using testing::Return;

        ON_CALL(*this, suitability_for_display(_))
            .WillByDefault(Return(mg::probe::supported));
        ON_CALL(*this, suitability_for_allocator(_))
            .WillByDefault(Return(mg::probe::supported));
        ON_CALL(*this, make_framebuffer_provider(_))
            .WillByDefault([](mg::DisplaySink&)
                {
                    return std::make_unique<NullFramebufferProvider>();
                });
        ON_CALL(*this, create_task(_))
            .WillByDefault([](std::unique_ptr<Surface> surface) -> std::unique_ptr<Task>
                {
                    return std::make_unique<TrackingTask>(std::move(surface));
                });
        ON_CALL(*this, wait_complete(_))
            .WillByDefault([](std::unique_ptr<Task> task) -> std::unique_ptr<Surface>
                {
                    auto* const tracking_task = dynamic_cast<TrackingTask*>(task.get());
                    if (!tracking_task)
                    {
                        throw std::logic_error{"Unexpected blitter task type"};
                    }
                    return std::move(tracking_task->surface);
                });
        ON_CALL(*this, blit(_, _, _, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, fill(_, _, _, _, _, _))
            .WillByDefault(Return(true));
        ON_CALL(*this, surface_for_fb(_))
            .WillByDefault([](mg::CPUAddressableDisplayAllocator::MappableFB const&)
                {
                    return std::make_unique<Surface>();
                });
        ON_CALL(*this, map_buffer(_))
            .WillByDefault([](mg::Buffer const& buffer)
                {
                    return buffer.map_readable();
                });
    }

    MOCK_METHOD(mg::probe::Result, suitability_for_display, (mg::DisplaySink& sink), (override));
    MOCK_METHOD(
        mg::probe::Result,
        suitability_for_allocator,
        (std::shared_ptr<mg::GraphicBufferAllocator> const& target),
        (override));
    MOCK_METHOD(
        (std::unique_ptr<mg::RenderingProvider::FramebufferProvider>),
        make_framebuffer_provider,
        (mg::DisplaySink& sink),
        (override));
    MOCK_METHOD((std::unique_ptr<Task>), create_task, (std::unique_ptr<Surface> surface), (override));
    MOCK_METHOD((std::unique_ptr<Surface>), wait_complete, (std::unique_ptr<Task> task), (override));
    MOCK_METHOD(
        bool,
        blit,
        (Task& task,
         mg::Buffer const& source,
         geom::Rectangle const& source_rect,
         geom::Rectangle const& target_rect,
         MirOrientation rotation,
         MirMirrorMode mirror_mode),
        (override));
    MOCK_METHOD(
        bool,
        fill,
        (Task& task, geom::Rectangle const& target_rect, uint8_t r, uint8_t g, uint8_t b, uint8_t a),
        (override));
    MOCK_METHOD(
        (std::unique_ptr<Surface>),
        surface_for_fb,
        (mg::CPUAddressableDisplayAllocator::MappableFB const& fb),
        (override));
    MOCK_METHOD(
        (std::unique_ptr<mir::renderer::software::Mapping<std::byte const>>),
        map_buffer,
        (mg::Buffer const& buffer),
        (override));
};

struct RenderableWithBuffer
{
    std::shared_ptr<testing::NiceMock<mtd::MockRenderable>> renderable;
    std::shared_ptr<mg::Buffer> buffer;
};

class BlitterRenderer : public testing::Test
{
public:
    BlitterRenderer()
    {
        using testing::_;
        using testing::AnyNumber;
        using testing::Return;
        using testing::SetArgPointee;

        static char const* const egl_extensions =
            "EGL_EXT_platform_base "
            "EGL_MESA_platform_surfaceless "
            "EGL_KHR_image_base "
            "EGL_KHR_no_config_context "
            "EGL_KHR_surfaceless_context "
            "EGL_EXT_image_dma_buf_import";

        ON_CALL(mock_egl, eglQueryString(_, EGL_EXTENSIONS))
            .WillByDefault(Return(egl_extensions));
        ON_CALL(mock_egl, eglGetProcAddress(testing::StrEq("eglQueryDevicesEXT")))
            .WillByDefault(Return(nullptr));

        ON_CALL(mock_gl, glCreateShader(GL_VERTEX_SHADER))
            .WillByDefault(Return(vertex_shader));
        ON_CALL(mock_gl, glCreateShader(GL_FRAGMENT_SHADER))
            .WillByDefault(Return(fragment_shader));
        ON_CALL(mock_gl, glCreateProgram())
            .WillByDefault(Return(gl_program));
        ON_CALL(mock_gl, glGetProgramiv(_, _, _))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));
        ON_CALL(mock_gl, glGetShaderiv(_, _, _))
            .WillByDefault(SetArgPointee<2>(GL_TRUE));
        ON_CALL(mock_gl, glGetAttribLocation(_, _))
            .WillByDefault(Return(attribute_location));
        ON_CALL(mock_gl, glGetUniformLocation(_, _))
            .WillByDefault(Return(uniform_location));
        ON_CALL(mock_gl, glCheckFramebufferStatus(GL_FRAMEBUFFER))
            .WillByDefault(Return(GL_FRAMEBUFFER_COMPLETE));

        EXPECT_CALL(mock_gl, glUseProgram(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glActiveTexture(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniform1i(_, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniform1f(_, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniform2f(_, _, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glUniformMatrix4fv(_, _, GL_FALSE, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glBindBuffer(_, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glVertexAttribPointer(_, _, _, _, _, _)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glEnableVertexAttribArray(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glDisableVertexAttribArray(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glDisable(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glEnable(_)).Times(AnyNumber());
        EXPECT_CALL(mock_gl, glBlendFuncSeparate(_, _, _, _)).Times(AnyNumber());
    }

    auto make_renderer() -> std::unique_ptr<mrb::Renderer>
    {
        auto renderer = std::make_unique<mrb::Renderer>(
            blitter,
            allocator,
            std::make_shared<mtd::StubGLConfig>());
        renderer->set_viewport(viewport());
        return renderer;
    }

    auto make_renderable(geom::Rectangle const& rect = default_renderable_rect()) -> RenderableWithBuffer
    {
        auto buffer = std::make_shared<mtd::StubBuffer>(rect.size, mir_pixel_format_argb_8888);
        auto renderable = std::make_shared<testing::NiceMock<mtd::MockRenderable>>();

        ON_CALL(*renderable, id())
            .WillByDefault(testing::Return(renderable.get()));
        ON_CALL(*renderable, buffer())
            .WillByDefault(testing::Return(buffer));
        ON_CALL(*renderable, screen_position())
            .WillByDefault(testing::Return(rect));
        ON_CALL(*renderable, src_bounds())
            .WillByDefault(testing::Return(to_src_bounds(rect)));
        ON_CALL(*renderable, clip_area())
            .WillByDefault(testing::Return(std::optional<geom::Rectangle>{}));
        ON_CALL(*renderable, alpha())
            .WillByDefault(testing::Return(1.0f));
        ON_CALL(*renderable, shaped())
            .WillByDefault(testing::Return(false));
        ON_CALL(*renderable, opaque_region())
            .WillByDefault(testing::Return(std::optional<geom::Rectangles>{}));
        ON_CALL(*renderable, transformation())
            .WillByDefault(testing::Return(glm::mat4{1.0f}));
        ON_CALL(*renderable, orientation())
            .WillByDefault(testing::Return(mir_orientation_normal));
        ON_CALL(*renderable, mirror_mode())
            .WillByDefault(testing::Return(mir_mirror_mode_none));

        return {renderable, buffer};
    }

    testing::NiceMock<mtd::MockGL> mock_gl;
    testing::NiceMock<mtd::MockEGL> mock_egl;
    std::shared_ptr<testing::NiceMock<MockBlitterRenderingProvider>> blitter{
        std::make_shared<testing::NiceMock<MockBlitterRenderingProvider>>()};
    testing::NiceMock<StubCPUAddressableDisplayAllocator> allocator;

private:
    static GLuint const vertex_shader{1};
    static GLuint const fragment_shader{2};
    static GLuint const gl_program{3};
    static GLint const attribute_location{4};
    static GLint const uniform_location{5};
};

}

TEST_F(BlitterRenderer, blits_all_renderables_in_one_task_without_gl_drawing)
{
    auto first = make_renderable();
    auto second = make_renderable({{40, 20}, {30, 40}});
    mg::RenderableList const renderables{first.renderable, second.renderable};

    EXPECT_CALL(*blitter, create_task(testing::_)).Times(1);
    EXPECT_CALL(*blitter, fill(testing::_, testing::_, 0, 0, 0, 255)).Times(1);
    EXPECT_CALL(*blitter, blit(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_)).Times(2);
    EXPECT_CALL(*blitter, wait_complete(testing::_)).Times(1);
    EXPECT_CALL(*blitter, map_buffer(testing::_)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(testing::_, testing::_, testing::_)).Times(0);

    auto renderer = make_renderer();
    auto framebuffer = renderer->render(renderables);

    ASSERT_NE(nullptr, framebuffer);
}

TEST_F(BlitterRenderer, falls_back_to_gl_clear_and_starts_a_new_task_when_fill_fails)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};

    testing::InSequence seq;
    EXPECT_CALL(*blitter, create_task(testing::_));
    EXPECT_CALL(*blitter, fill(testing::_, testing::_, 0, 0, 0, 255))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*blitter, wait_complete(testing::_));
    EXPECT_CALL(mock_gl, glClear(GL_COLOR_BUFFER_BIT));
    EXPECT_CALL(mock_gl, glFinish());
    EXPECT_CALL(*blitter, create_task(testing::_));
    EXPECT_CALL(
        *blitter,
        blit(testing::_, IsSameBufferAs(renderable.buffer), testing::_, testing::_, testing::_, testing::_));
    EXPECT_CALL(*blitter, wait_complete(testing::_));

    auto renderer = make_renderer();
    auto framebuffer = renderer->render(renderables);

    ASSERT_NE(nullptr, framebuffer);
}

TEST_F(BlitterRenderer, falls_back_to_gl_for_failed_mid_list_blit_then_blits_subsequent_renderables)
{
    auto first = make_renderable({{0, 0}, {20, 20}});
    auto fallback = make_renderable({{20, 0}, {20, 20}});
    auto after_fallback = make_renderable({{40, 0}, {20, 20}});
    mg::RenderableList const renderables{first.renderable, fallback.renderable, after_fallback.renderable};

    testing::InSequence seq;
    EXPECT_CALL(*blitter, create_task(testing::_));
    EXPECT_CALL(*blitter, fill(testing::_, testing::_, 0, 0, 0, 255));
    EXPECT_CALL(
        *blitter,
        blit(testing::_, IsSameBufferAs(first.buffer), testing::_, testing::_, testing::_, testing::_));
    EXPECT_CALL(
        *blitter,
        blit(testing::_, IsSameBufferAs(fallback.buffer), testing::_, testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(false));
    EXPECT_CALL(*blitter, wait_complete(testing::_));
    EXPECT_CALL(mock_gl, glDrawArrays(testing::_, testing::_, testing::_));
    EXPECT_CALL(mock_gl, glFinish());
    EXPECT_CALL(*blitter, create_task(testing::_));
    EXPECT_CALL(
        *blitter,
        blit(testing::_, IsSameBufferAs(after_fallback.buffer), testing::_, testing::_, testing::_, testing::_));
    EXPECT_CALL(*blitter, wait_complete(testing::_));

    auto renderer = make_renderer();
    auto framebuffer = renderer->render(renderables);

    ASSERT_NE(nullptr, framebuffer);
}

TEST_F(BlitterRenderer, draws_translucent_renderables_with_gl_without_attempting_blit)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};

    ON_CALL(*renderable.renderable, alpha())
        .WillByDefault(testing::Return(0.5f));

    EXPECT_CALL(*blitter, blit(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(testing::_, testing::_, testing::_)).Times(1);

    auto renderer = make_renderer();
    auto framebuffer = renderer->render(renderables);

    ASSERT_NE(nullptr, framebuffer);
}

TEST_F(BlitterRenderer, draws_shaped_renderables_without_opaque_region_with_gl_without_attempting_blit)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};

    ON_CALL(*renderable.renderable, shaped())
        .WillByDefault(testing::Return(true));
    ON_CALL(*renderable.renderable, opaque_region())
        .WillByDefault(testing::Return(std::optional<geom::Rectangles>{}));

    EXPECT_CALL(*blitter, blit(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(testing::_, testing::_, testing::_)).Times(1);

    auto renderer = make_renderer();
    auto framebuffer = renderer->render(renderables);

    ASSERT_NE(nullptr, framebuffer);
}

TEST_F(BlitterRenderer, blits_shaped_renderables_when_opaque_region_covers_target_rect)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};

    ON_CALL(*renderable.renderable, shaped())
        .WillByDefault(testing::Return(true));
    ON_CALL(*renderable.renderable, opaque_region())
        .WillByDefault(testing::Return(std::optional<geom::Rectangles>{geom::Rectangles{default_renderable_rect()}}));

    EXPECT_CALL(
        *blitter,
        blit(testing::_, IsSameBufferAs(renderable.buffer), testing::_, testing::_, testing::_, testing::_))
        .Times(1);
    EXPECT_CALL(mock_gl, glDrawArrays(testing::_, testing::_, testing::_)).Times(0);

    auto renderer = make_renderer();
    auto framebuffer = renderer->render(renderables);

    ASSERT_NE(nullptr, framebuffer);
}

TEST_F(BlitterRenderer, active_output_filter_draws_whole_frame_with_gl_without_blitting)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};

    EXPECT_CALL(*blitter, create_task(testing::_)).Times(0);
    EXPECT_CALL(*blitter, fill(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(*blitter, blit(testing::_, testing::_, testing::_, testing::_, testing::_, testing::_)).Times(0);
    EXPECT_CALL(*blitter, wait_complete(testing::_)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(testing::_, testing::_, testing::_)).Times(testing::AtLeast(1));

    auto renderer = make_renderer();
    renderer->set_output_filter(mir_output_filter_grayscale);
    auto framebuffer = renderer->render(renderables);

    ASSERT_NE(nullptr, framebuffer);
}

TEST_F(BlitterRenderer, reuses_framebuffer_after_previous_render_result_is_destroyed)
{
    mg::RenderableList const renderables;

    EXPECT_CALL(allocator, alloc_fb(testing::_)).Times(1);

    auto renderer = make_renderer();
    {
        auto framebuffer = renderer->render(renderables);
        ASSERT_NE(nullptr, framebuffer);
    }
    auto framebuffer = renderer->render(renderables);

    ASSERT_NE(nullptr, framebuffer);
}
