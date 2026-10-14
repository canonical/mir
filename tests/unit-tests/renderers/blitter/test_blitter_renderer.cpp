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

#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/drm_formats.h>
#include <mir/test/doubles/mock_blitter_rendering_provider.h>
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
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mrb = mir::renderer::blitter;
namespace mrs = mir::renderer::software;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
auto const surfaceless_display{reinterpret_cast<EGLDisplay>(0x5e7f)};
auto const other_context{reinterpret_cast<EGLContext>(0x07e4)};
geom::Size const output_size{100, 100};
geom::Rectangle const default_renderable_rect{{10, 20}, {30, 40}};

auto whole_of(geom::Rectangle const& rect) -> geom::RectangleD
{
    return {{0.0, 0.0}, {rect.size.width.as_value(), rect.size.height.as_value()}};
}

MATCHER_P(IsSameBufferAs, buffer, "")
{
    return &arg == buffer.get();
}

class FakeDMABufBuffer : public mg::DMABufBuffer
{
public:
    explicit FakeDMABufBuffer(geom::Size size) : size_{size} {}

    auto format() const -> mg::DRMFormat override { return mg::DRMFormat{DRM_FORMAT_XRGB8888}; }
    auto modifier() const -> std::optional<uint64_t> override { return std::nullopt; }
    auto planes() const -> std::vector<PlaneDescriptor> const& override { return planes_; }
    auto layout() const -> mg::gl::Texture::Layout override { return mg::gl::Texture::Layout::TopRowFirst; }
    auto size() const -> geom::Size override { return size_; }

private:
    geom::Size const size_;
    std::vector<PlaneDescriptor> const planes_;
};

class FakeMappableFB : public mg::CPUAddressableDisplayAllocator::MappableFB
{
public:
    explicit FakeMappableFB(geom::Size size) : size_{size}, dmabuf{size} {}

    auto as_dmabuf() -> mg::DMABufBuffer const* override { return &dmabuf; }
    auto map_writeable() -> std::unique_ptr<mrs::Mapping<std::byte>> override { return nullptr; }
    auto format() const -> MirPixelFormat override { return mir_pixel_format_xrgb_8888; }
    auto stride() const -> geom::Stride override { return geom::Stride{size_.width.as_int() * 4}; }
    auto size() const -> geom::Size override { return size_; }

private:
    geom::Size const size_;
    FakeDMABufBuffer const dmabuf;
};

class MockCPUAddressableDisplayAllocator : public mg::CPUAddressableDisplayAllocator
{
public:
    MockCPUAddressableDisplayAllocator()
    {
        ON_CALL(*this, supported_formats())
            .WillByDefault(Return(std::vector{mg::DRMFormat{DRM_FORMAT_XRGB8888}}));
        ON_CALL(*this, output_size()).WillByDefault(Return(::output_size));
        ON_CALL(*this, alloc_fb(_)).WillByDefault(
            [](auto) -> std::unique_ptr<MappableFB> { return std::make_unique<FakeMappableFB>(::output_size); });
    }

    MOCK_METHOD(std::vector<mg::DRMFormat>, supported_formats, (), (const, override));
    MOCK_METHOD(std::unique_ptr<MappableFB>, alloc_fb, (mg::DRMFormat), (override));
    MOCK_METHOD(geom::Size, output_size, (), (const, override));
};

/// A Task that, like a real one, holds its Surface until it is complete
class HoldingTask : public mg::BlitterRenderingProvider::Task
{
public:
    explicit HoldingTask(std::unique_ptr<mg::BlitterRenderingProvider::Surface> surface) :
        surface{std::move(surface)}
    {
    }

    std::unique_ptr<mg::BlitterRenderingProvider::Surface> surface;
};

/// A glGen* action that allocates sequential non-zero names
auto generate_names()
{
    return [next = GLuint{1}](GLsizei n, GLuint* out) mutable
    {
        for (auto i = 0; i < n; ++i)
        {
            out[i] = next++;
        }
    };
}

struct RenderableWithBuffer
{
    std::shared_ptr<NiceMock<mtd::MockRenderable>> renderable;
    std::shared_ptr<mg::Buffer> buffer;
};

class BlitterRenderer : public Test
{
public:
    BlitterRenderer()
    {
        ON_CALL(mock_egl, eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS))
            .WillByDefault(Return("EGL_EXT_platform_base EGL_MESA_platform_surfaceless"));
        ON_CALL(mock_egl, eglQueryString(Ne(EGL_NO_DISPLAY), EGL_EXTENSIONS))
            .WillByDefault(Return(
                "EGL_KHR_no_config_context EGL_KHR_surfaceless_context EGL_EXT_image_dma_buf_import "
                "EGL_KHR_image_base"));
        ON_CALL(mock_egl, eglGetPlatformDisplayEXT(EGL_PLATFORM_SURFACELESS_MESA, _, _))
            .WillByDefault(Return(surfaceless_display));

        ON_CALL(mock_gl, glGenTextures(_, _)).WillByDefault(generate_names());
        ON_CALL(mock_gl, glGenFramebuffers(_, _)).WillByDefault(generate_names());
        ON_CALL(mock_gl, glGenRenderbuffers(_, _)).WillByDefault(generate_names());
        ON_CALL(mock_gl, glCheckFramebufferStatus(GL_FRAMEBUFFER)).WillByDefault(Return(GL_FRAMEBUFFER_COMPLETE));
        ON_CALL(mock_gl, glCreateShader(_)).WillByDefault(Return(1));
        ON_CALL(mock_gl, glCreateProgram()).WillByDefault(Return(2));
        ON_CALL(mock_gl, glGetProgramiv(_, _, _)).WillByDefault(SetArgPointee<2>(GL_TRUE));
        ON_CALL(mock_gl, glGetShaderiv(_, _, _)).WillByDefault(SetArgPointee<2>(GL_TRUE));

        ON_CALL(*blitter, surface_for_fb(_)).WillByDefault(
            [](auto const&) { return std::make_unique<mg::BlitterRenderingProvider::Surface>(); });
        ON_CALL(*blitter, create_task(_)).WillByDefault(
            [](auto surface) -> std::unique_ptr<mg::BlitterRenderingProvider::Task>
            {
                return std::make_unique<HoldingTask>(std::move(surface));
            });
        ON_CALL(*blitter, wait_complete(_)).WillByDefault(
            [](auto task) { return std::move(dynamic_cast<HoldingTask&>(*task).surface); });
        ON_CALL(*blitter, blit(_, _, _, _, _, _)).WillByDefault(Return(true));
        ON_CALL(*blitter, fill(_, _, _, _, _, _)).WillByDefault(Return(true));
        ON_CALL(*blitter, map_buffer(_)).WillByDefault(
            [](mg::Buffer const& buffer) { return dynamic_cast<mtd::StubBuffer const&>(buffer).map_readable(); });
    }

    auto make_renderer(geom::Rectangle const& viewport = {{0, 0}, output_size}) -> std::unique_ptr<mrb::Renderer>
    {
        auto renderer = std::make_unique<mrb::Renderer>(blitter, allocator, std::make_shared<mtd::StubGLConfig>());
        renderer->set_viewport(viewport);
        return renderer;
    }

    auto make_renderable(geom::Rectangle const& rect = default_renderable_rect) -> RenderableWithBuffer
    {
        auto buffer = std::make_shared<mtd::StubBuffer>(rect.size, mir_pixel_format_argb_8888);
        auto renderable = std::make_shared<NiceMock<mtd::MockRenderable>>();

        ON_CALL(*renderable, buffer()).WillByDefault(Return(buffer));
        ON_CALL(*renderable, screen_position()).WillByDefault(Return(rect));
        ON_CALL(*renderable, src_bounds()).WillByDefault(Return(whole_of(rect)));
        ON_CALL(*renderable, clip_area()).WillByDefault(Return(std::nullopt));
        ON_CALL(*renderable, alpha()).WillByDefault(Return(1.0f));
        ON_CALL(*renderable, shaped()).WillByDefault(Return(false));
        ON_CALL(*renderable, opaque_region()).WillByDefault(Return(std::nullopt));
        ON_CALL(*renderable, transformation()).WillByDefault(Return(glm::mat4{1.0f}));
        ON_CALL(*renderable, orientation()).WillByDefault(Return(mir_orientation_normal));
        ON_CALL(*renderable, mirror_mode()).WillByDefault(Return(mir_mirror_mode_none));

        return {renderable, buffer};
    }

    NiceMock<mtd::MockEGL> mock_egl;
    NiceMock<mtd::MockGL> mock_gl;
    std::shared_ptr<NiceMock<mtd::MockBlitterRenderingProvider>> const blitter{
        std::make_shared<NiceMock<mtd::MockBlitterRenderingProvider>>()};
    NiceMock<MockCPUAddressableDisplayAllocator> allocator;
};
}

TEST_F(BlitterRenderer, blits_all_renderables_in_one_task_without_gl_drawing)
{
    auto first = make_renderable();
    auto second = make_renderable({{40, 20}, {30, 40}});
    mg::RenderableList const renderables{first.renderable, second.renderable};
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, create_task(_)).Times(1);
    EXPECT_CALL(*blitter, fill(_, _, 0, 0, 0, 255)).Times(1);
    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(2);
    EXPECT_CALL(*blitter, wait_complete(_)).Times(1);
    EXPECT_CALL(*blitter, map_buffer(_)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(0);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, falls_back_to_gl_clear_and_starts_a_new_task_when_fill_fails)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    auto renderer = make_renderer();

    InSequence seq;
    EXPECT_CALL(*blitter, create_task(_));
    EXPECT_CALL(*blitter, fill(_, _, 0, 0, 0, 255)).WillOnce(Return(false));
    EXPECT_CALL(*blitter, wait_complete(_));
    EXPECT_CALL(mock_gl, glClear(GL_COLOR_BUFFER_BIT));
    EXPECT_CALL(mock_gl, glFinish());
    EXPECT_CALL(*blitter, create_task(_));
    EXPECT_CALL(*blitter, blit(_, IsSameBufferAs(renderable.buffer), _, _, _, _));
    EXPECT_CALL(*blitter, wait_complete(_));

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, falls_back_to_gl_for_failed_mid_list_blit_then_blits_subsequent_renderables)
{
    auto first = make_renderable({{0, 0}, {20, 20}});
    auto fallback = make_renderable({{20, 0}, {20, 20}});
    auto after_fallback = make_renderable({{40, 0}, {20, 20}});
    mg::RenderableList const renderables{first.renderable, fallback.renderable, after_fallback.renderable};
    auto renderer = make_renderer();

    InSequence seq;
    EXPECT_CALL(*blitter, create_task(_));
    EXPECT_CALL(*blitter, fill(_, _, 0, 0, 0, 255));
    EXPECT_CALL(*blitter, blit(_, IsSameBufferAs(first.buffer), _, _, _, _));
    EXPECT_CALL(*blitter, blit(_, IsSameBufferAs(fallback.buffer), _, _, _, _)).WillOnce(Return(false));
    EXPECT_CALL(*blitter, wait_complete(_));
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _));
    EXPECT_CALL(mock_gl, glFinish());
    EXPECT_CALL(*blitter, create_task(_));
    EXPECT_CALL(*blitter, blit(_, IsSameBufferAs(after_fallback.buffer), _, _, _, _));
    EXPECT_CALL(*blitter, wait_complete(_));

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, draws_translucent_renderables_with_gl_without_attempting_blit)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, alpha()).WillByDefault(Return(0.5f));
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(1);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, draws_shaped_renderables_without_opaque_region_with_gl_without_attempting_blit)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, shaped()).WillByDefault(Return(true));
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(1);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, blits_shaped_renderables_when_opaque_region_covers_them)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, shaped()).WillByDefault(Return(true));
    ON_CALL(*renderable.renderable, opaque_region())
        .WillByDefault(Return(geom::Rectangles{default_renderable_rect}));
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, IsSameBufferAs(renderable.buffer), _, _, _, _)).Times(1);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(0);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, opaque_region_is_compared_in_scene_coordinates)
{
    geom::Rectangle const viewport{{1000, 500}, output_size};
    geom::Rectangle const in_scene{{1010, 520}, {30, 40}};
    auto renderable = make_renderable(in_scene);
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, shaped()).WillByDefault(Return(true));
    ON_CALL(*renderable.renderable, opaque_region()).WillByDefault(Return(geom::Rectangles{in_scene}));
    auto renderer = make_renderer(viewport);

    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(1);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(0);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, blits_to_the_renderables_position_relative_to_the_viewport)
{
    geom::Rectangle const viewport{{1000, 500}, output_size};
    auto renderable = make_renderable({{1010, 520}, {30, 40}});
    mg::RenderableList const renderables{renderable.renderable};
    auto renderer = make_renderer(viewport);

    EXPECT_CALL(*blitter, fill(_, geom::Rectangle{{0, 0}, output_size}, _, _, _, _));
    EXPECT_CALL(*blitter, blit(_, _, geom::Rectangle{{0, 0}, {30, 40}}, geom::Rectangle{{10, 20}, {30, 40}}, _, _));

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, blits_only_the_clipped_part_of_a_renderable)
{
    auto renderable = make_renderable({{10, 20}, {30, 40}});
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, clip_area()).WillByDefault(Return(geom::Rectangle{{20, 30}, {100, 100}}));
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, geom::Rectangle{{10, 10}, {20, 30}}, geom::Rectangle{{20, 30}, {20, 30}}, _, _));

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, blits_only_the_part_of_a_renderable_on_the_output)
{
    auto renderable = make_renderable({{-10, 80}, {30, 40}});
    mg::RenderableList const renderables{renderable.renderable};
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, geom::Rectangle{{10, 0}, {20, 20}}, geom::Rectangle{{0, 80}, {20, 20}}, _, _));

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, does_not_draw_renderables_entirely_off_the_output)
{
    auto renderable = make_renderable({{200, 200}, {30, 40}});
    mg::RenderableList const renderables{renderable.renderable};
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(0);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, blits_a_whole_rotated_renderable)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, orientation()).WillByDefault(Return(mir_orientation_left));
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, _, _, mir_orientation_left, mir_mirror_mode_none));
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(0);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, draws_a_clipped_rotated_renderable_with_gl)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, orientation()).WillByDefault(Return(mir_orientation_left));
    ON_CALL(*renderable.renderable, clip_area()).WillByDefault(Return(geom::Rectangle{{20, 30}, {100, 100}}));
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(1);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, blits_a_renderable_sampling_a_fractional_region_from_the_floored_region)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, src_bounds()).WillByDefault(Return(geom::RectangleD{{0.5, 1.7}, {30.9, 40.2}}));
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, Eq(geom::Rectangle{{0, 1}, {30, 40}}), _, _, _));
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(0);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, draws_a_renderable_sampling_less_than_a_pixel_with_gl)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, src_bounds()).WillByDefault(Return(geom::RectangleD{{0, 0}, {0.5, 40}}));
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(1);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, draws_a_transformed_renderable_with_gl)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    ON_CALL(*renderable.renderable, transformation()).WillByDefault(Return(glm::mat4{2.0f}));
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(1);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, active_output_filter_draws_whole_frame_with_gl_without_blitting)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    auto renderer = make_renderer();
    renderer->set_output_filter(mir_output_filter_grayscale);

    EXPECT_CALL(*blitter, create_task(_)).Times(0);
    EXPECT_CALL(*blitter, fill(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(0);
    // Once for the renderable, once to apply the filter
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(2);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, rotated_output_draws_whole_frame_with_gl_without_blitting)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    auto renderer = make_renderer();
    renderer->set_output_transform(glm::mat2{0, 1, -1, 0});

    EXPECT_CALL(*blitter, create_task(_)).Times(0);
    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(1);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, scaled_output_draws_whole_frame_with_gl_without_blitting)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    auto renderer = make_renderer({{0, 0}, {200, 200}});

    EXPECT_CALL(*blitter, create_task(_)).Times(0);
    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _)).Times(0);
    EXPECT_CALL(mock_gl, glDrawArrays(_, _, _)).Times(1);

    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, reuses_framebuffer_after_previous_render_result_is_destroyed)
{
    mg::RenderableList const renderables;
    auto renderer = make_renderer();

    EXPECT_CALL(allocator, alloc_fb(_)).Times(0);

    renderer->render(renderables);
    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, allocates_another_framebuffer_while_the_previous_one_is_in_use)
{
    mg::RenderableList const renderables;
    auto renderer = make_renderer();

    EXPECT_CALL(allocator, alloc_fb(_)).Times(1);

    auto const in_use = renderer->render(renderables);
    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, recovers_the_target_from_the_blitter_when_rendering_throws)
{
    auto renderable = make_renderable();
    mg::RenderableList const renderables{renderable.renderable};
    auto renderer = make_renderer();

    EXPECT_CALL(*blitter, blit(_, _, _, _, _, _))
        .WillOnce(Throw(std::runtime_error{"Blitter hung"}))
        .WillRepeatedly(Return(true));
    EXPECT_CALL(*blitter, wait_complete(_)).Times(2);

    EXPECT_THROW(renderer->render(renderables), std::runtime_error);

    // The pool's only framebuffer is reused, and must still have its Surface
    EXPECT_CALL(allocator, alloc_fb(_)).Times(0);
    EXPECT_CALL(*blitter, create_task(NotNull()));
    EXPECT_THAT(renderer->render(renderables), NotNull());
}

TEST_F(BlitterRenderer, suspend_leaves_other_contexts_current)
{
    mg::RenderableList const renderables;
    auto renderer = make_renderer();
    renderer->render(renderables);

    eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, other_context);
    renderer->suspend();

    EXPECT_THAT(eglGetCurrentContext(), Eq(other_context));
}

TEST_F(BlitterRenderer, suspend_releases_its_own_context)
{
    auto renderable = make_renderable();
    ON_CALL(*renderable.renderable, alpha()).WillByDefault(Return(0.5f));
    mg::RenderableList const renderables{renderable.renderable};
    auto renderer = make_renderer();
    renderer->render(renderables);
    ASSERT_THAT(eglGetCurrentContext(), Eq(mock_egl.fake_egl_context));

    renderer->suspend();

    EXPECT_THAT(eglGetCurrentContext(), Eq(EGL_NO_CONTEXT));
}
