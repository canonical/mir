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

#include "src/platform/renderers/blitter/framebuffer_pool.h"
#include "src/platform/renderers/blitter/software_egl_context.h"

#include <mir/graphics/dmabuf_buffer.h>
#include <mir/graphics/drm_formats.h>
#include <mir/test/doubles/mock_egl.h>
#include <mir/test/doubles/mock_gl.h>
#include <mir/test/doubles/mock_gl_config.h>

#include <drm_fourcc.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <stdexcept>
#include <string>
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
geom::Size const default_output_size{640, 480};

using Events = std::vector<std::string>;

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
    FakeMappableFB(geom::Size size, bool exportable, Events* events) :
        size_{size},
        dmabuf{size},
        exportable{exportable},
        events{events}
    {
    }

    ~FakeMappableFB() override
    {
        if (events)
        {
            events->push_back("framebuffer destroyed");
        }
    }

    auto as_dmabuf() -> mg::DMABufBuffer const* override { return exportable ? &dmabuf : nullptr; }
    auto map_writeable() -> std::unique_ptr<mrs::Mapping<std::byte>> override { return nullptr; }
    auto format() const -> MirPixelFormat override { return mir_pixel_format_xrgb_8888; }
    auto stride() const -> geom::Stride override { return geom::Stride{size_.width.as_int() * 4}; }
    auto size() const -> geom::Size override { return size_; }

private:
    geom::Size const size_;
    FakeDMABufBuffer const dmabuf;
    bool const exportable;
    Events* const events;
};

class StubSurface : public mg::BlitterRenderingProvider::Surface
{
public:
    explicit StubSurface(Events* events = nullptr) : events{events} {}

    ~StubSurface() override
    {
        if (events)
        {
            events->push_back("surface destroyed");
        }
    }

private:
    Events* const events;
};

class MockCPUAddressableDisplayAllocator : public mg::CPUAddressableDisplayAllocator
{
public:
    MOCK_METHOD(std::vector<mg::DRMFormat>, supported_formats, (), (const, override));
    MOCK_METHOD(std::unique_ptr<MappableFB>, alloc_fb, (mg::DRMFormat), (override));
    MOCK_METHOD(geom::Size, output_size, (), (const, override));
};

class MockBlitterRenderingProvider : public mg::BlitterRenderingProvider
{
public:
    MOCK_METHOD(mg::probe::Result, suitability_for_display, (mg::DisplaySink&), (override));
    MOCK_METHOD(
        mg::probe::Result,
        suitability_for_allocator,
        (std::shared_ptr<mg::GraphicBufferAllocator> const&),
        (override));
    MOCK_METHOD(std::unique_ptr<FramebufferProvider>, make_framebuffer_provider, (mg::DisplaySink&), (override));
    MOCK_METHOD(std::unique_ptr<Task>, create_task, (std::unique_ptr<Surface>), (override));
    MOCK_METHOD(std::unique_ptr<Surface>, wait_complete, (std::unique_ptr<Task>), (override));
    MOCK_METHOD(
        bool,
        blit,
        (Task&, mg::Buffer const&, geom::Rectangle const&, geom::Rectangle const&, MirOrientation, MirMirrorMode),
        (override));
    MOCK_METHOD(bool, fill, (Task&, geom::Rectangle const&, uint8_t, uint8_t, uint8_t, uint8_t), (override));
    MOCK_METHOD(
        std::unique_ptr<Surface>,
        surface_for_fb,
        (mg::CPUAddressableDisplayAllocator::MappableFB const&),
        (override));
    MOCK_METHOD(
        std::unique_ptr<mrs::Mapping<std::byte const>>,
        map_buffer,
        (mg::Buffer const&),
        (override));
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

/// A glGen* action that allocates sequential non-zero names, recording them in \p ids
auto generate_into(std::vector<GLuint>& ids)
{
    return [&ids](GLsizei n, GLuint* out)
    {
        for (auto i = 0; i < n; ++i)
        {
            out[i] = static_cast<GLuint>(ids.size() + 1);
            ids.push_back(out[i]);
        }
    };
}

/// An alloc_fb action that allocates fake framebuffers of the allocator's current output size
auto allocate_fakes_from(mg::CPUAddressableDisplayAllocator const& allocator, bool exportable, Events* events)
{
    return [&allocator, exportable, events](auto) -> std::unique_ptr<mg::CPUAddressableDisplayAllocator::MappableFB>
    {
        return std::make_unique<FakeMappableFB>(allocator.output_size(), exportable, events);
    };
}

/// A surface_for_fb action that creates stub surfaces
auto make_stub_surfaces(Events* events)
{
    return [events](auto const&) -> std::unique_ptr<mg::BlitterRenderingProvider::Surface>
    {
        return std::make_unique<StubSurface>(events);
    };
}

class FramebufferPoolTest : public Test
{
public:
    FramebufferPoolTest()
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

        ON_CALL(allocator, supported_formats())
            .WillByDefault(Return(std::vector{mg::DRMFormat{DRM_FORMAT_XRGB8888}}));
        ON_CALL(allocator, output_size()).WillByDefault(Return(default_output_size));
        ON_CALL(allocator, alloc_fb(_)).WillByDefault(allocate_fakes_from(allocator, true, nullptr));

        ON_CALL(*blitter, surface_for_fb(_)).WillByDefault(make_stub_surfaces(nullptr));

        context = std::make_shared<mrb::SoftwareEGLContext>();
    }

    auto make_pool() -> std::unique_ptr<mrb::FramebufferPool>
    {
        return std::make_unique<mrb::FramebufferPool>(context, blitter, allocator, config);
    }

    void make_other_context_current()
    {
        eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, other_context);
    }

    /// Record the destruction of subsequently created framebuffers and blitter surfaces in \p events
    void record_destruction_in(Events& events)
    {
        ON_CALL(allocator, alloc_fb(_)).WillByDefault(allocate_fakes_from(allocator, true, &events));
        ON_CALL(*blitter, surface_for_fb(_)).WillByDefault(make_stub_surfaces(&events));
    }

    void allocate_unexportable_framebuffers()
    {
        ON_CALL(allocator, alloc_fb(_)).WillByDefault(allocate_fakes_from(allocator, false, nullptr));
    }

    NiceMock<mtd::MockEGL> mock_egl;
    NiceMock<mtd::MockGL> mock_gl;

    std::shared_ptr<mrb::SoftwareEGLContext> context;
    std::shared_ptr<NiceMock<MockBlitterRenderingProvider>> const blitter{
        std::make_shared<NiceMock<MockBlitterRenderingProvider>>()};
    NiceMock<MockCPUAddressableDisplayAllocator> allocator;
    NiceMock<mtd::MockGLConfig> config;
};
}

TEST_F(FramebufferPoolTest, allocates_a_framebuffer_eagerly_in_the_preferred_format)
{
    ON_CALL(allocator, supported_formats())
        .WillByDefault(Return(std::vector{mg::DRMFormat{DRM_FORMAT_RGBA8888}, mg::DRMFormat{DRM_FORMAT_XRGB8888}}));

    EXPECT_CALL(allocator, alloc_fb(Eq(mg::DRMFormat{DRM_FORMAT_XRGB8888})));

    auto const pool = make_pool();
}

TEST_F(FramebufferPoolTest, throws_when_framebuffer_cannot_be_allocated)
{
    ON_CALL(allocator, alloc_fb(_)).WillByDefault(ReturnNull());

    EXPECT_THROW(make_pool(), std::runtime_error);
}

TEST_F(FramebufferPoolTest, throws_when_blitter_cannot_target_framebuffer)
{
    ON_CALL(*blitter, surface_for_fb(_)).WillByDefault(ReturnNull());

    EXPECT_THROW(make_pool(), std::runtime_error);
}

TEST_F(FramebufferPoolTest, throws_when_framebuffer_cannot_be_exported_as_dmabuf)
{
    allocate_unexportable_framebuffers();

    EXPECT_THROW(make_pool(), std::runtime_error);
}

TEST_F(FramebufferPoolTest, throws_when_dmabuf_cannot_be_imported)
{
    ON_CALL(mock_egl, eglCreateImageKHR(_, _, _, _, _)).WillByDefault(Return(EGL_NO_IMAGE_KHR));

    EXPECT_THROW(make_pool(), std::runtime_error);
}

TEST_F(FramebufferPoolTest, throws_when_framebuffer_is_incomplete)
{
    ON_CALL(mock_gl, glCheckFramebufferStatus(GL_FRAMEBUFFER)).WillByDefault(Return(GL_FRAMEBUFFER_UNSUPPORTED));

    try
    {
        make_pool();
        FAIL() << "Expected FramebufferPool construction to throw";
    }
    catch (std::runtime_error const& error)
    {
        EXPECT_THAT(error.what(), HasSubstr("GL_FRAMEBUFFER_UNSUPPORTED"));
    }
}

TEST_F(FramebufferPoolTest, imports_dmabuf_as_colour_attachment_of_framebuffer)
{
    std::vector<GLuint> textures;
    ON_CALL(mock_gl, glGenTextures(_, _)).WillByDefault(generate_into(textures));
    auto const image = mock_egl.fake_egl_image;

    EXPECT_CALL(mock_egl, eglCreateImageKHR(surfaceless_display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, _, _));
    EXPECT_CALL(mock_egl, glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, image));
    EXPECT_CALL(mock_egl, eglDestroyImageKHR(surfaceless_display, image));
    EXPECT_CALL(mock_gl, glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 1, 0));

    auto const pool = make_pool();

    EXPECT_THAT(textures, ElementsAre(1));
}

TEST_F(FramebufferPoolTest, has_no_depth_stencil_buffer_when_not_requested)
{
    EXPECT_CALL(mock_gl, glGenRenderbuffers(_, _)).Times(0);
    EXPECT_CALL(mock_gl, glFramebufferRenderbuffer(_, _, _, _)).Times(0);

    auto const pool = make_pool();
}

TEST_F(FramebufferPoolTest, attaches_depth_stencil_buffer_when_depth_requested)
{
    ON_CALL(config, depth_buffer_bits()).WillByDefault(Return(24));

    EXPECT_CALL(
        mock_gl,
        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8_OES,
            default_output_size.width.as_int(),
            default_output_size.height.as_int()));
    EXPECT_CALL(mock_gl, glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, 1));
    EXPECT_CALL(mock_gl, glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 1));

    auto const pool = make_pool();
}

TEST_F(FramebufferPoolTest, attaches_depth_stencil_buffer_when_stencil_requested)
{
    std::vector<GLuint> renderbuffers;
    ON_CALL(mock_gl, glGenRenderbuffers(_, _)).WillByDefault(generate_into(renderbuffers));
    ON_CALL(config, stencil_buffer_bits()).WillByDefault(Return(8));

    EXPECT_CALL(
        mock_gl,
        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8_OES,
            default_output_size.width.as_int(),
            default_output_size.height.as_int()));

    auto const pool = make_pool();

    EXPECT_THAT(renderbuffers, ElementsAre(1));
}

TEST_F(FramebufferPoolTest, pooled_framebuffer_reports_size_of_display_framebuffer)
{
    auto const pool = make_pool();
    auto const fb = pool->acquire();

    EXPECT_THAT(fb->size(), Eq(default_output_size));
    EXPECT_THAT(fb->entry().size(), Eq(default_output_size));
}

TEST_F(FramebufferPoolTest, entry_holds_blitter_surface_for_its_framebuffer)
{
    mg::BlitterRenderingProvider::Surface* surface{nullptr};
    EXPECT_CALL(*blitter, surface_for_fb(_))
        .WillOnce(
            [&surface](auto const&) -> std::unique_ptr<mg::BlitterRenderingProvider::Surface>
            {
                auto stub = std::make_unique<StubSurface>();
                surface = stub.get();
                return stub;
            });

    auto const pool = make_pool();
    auto const fb = pool->acquire();

    EXPECT_THAT(fb->entry().surface.get(), Eq(surface));
}

TEST_F(FramebufferPoolTest, bind_makes_context_current_and_binds_entry_framebuffer)
{
    std::vector<GLuint> framebuffers;
    ON_CALL(mock_gl, glGenFramebuffers(_, _)).WillByDefault(generate_into(framebuffers));
    auto const pool = make_pool();
    auto const fb = pool->acquire();
    make_other_context_current();

    EXPECT_CALL(mock_gl, glBindFramebuffer(GL_FRAMEBUFFER, framebuffers.at(0)));

    fb->entry().bind();

    EXPECT_THAT(eglGetCurrentContext(), Eq(mock_egl.fake_egl_context));
}

TEST_F(FramebufferPoolTest, destroying_entry_releases_gl_resources_with_context_current)
{
    std::vector<GLuint> textures;
    std::vector<GLuint> framebuffers;
    std::vector<GLuint> renderbuffers;
    ON_CALL(mock_gl, glGenTextures(_, _)).WillByDefault(generate_into(textures));
    ON_CALL(mock_gl, glGenFramebuffers(_, _)).WillByDefault(generate_into(framebuffers));
    ON_CALL(mock_gl, glGenRenderbuffers(_, _)).WillByDefault(generate_into(renderbuffers));
    ON_CALL(config, depth_buffer_bits()).WillByDefault(Return(24));
    auto pool = make_pool();
    make_other_context_current();

    auto const expect_context_current = [this](auto, auto)
    {
        EXPECT_THAT(eglGetCurrentContext(), Eq(mock_egl.fake_egl_context));
    };
    EXPECT_CALL(mock_gl, glDeleteTextures(1, Pointee(textures.at(0)))).WillOnce(expect_context_current);
    EXPECT_CALL(mock_gl, glDeleteFramebuffers(1, Pointee(framebuffers.at(0)))).WillOnce(expect_context_current);
    EXPECT_CALL(mock_gl, glDeleteRenderbuffers(1, Pointee(renderbuffers.at(0)))).WillOnce(expect_context_current);

    pool.reset();
}

TEST_F(FramebufferPoolTest, destroying_entry_tolerates_failure_to_make_context_current)
{
    auto pool = make_pool();
    make_other_context_current();
    ON_CALL(mock_egl, eglMakeCurrent(_, _, _, mock_egl.fake_egl_context)).WillByDefault(Return(EGL_FALSE));

    EXPECT_NO_THROW(pool.reset());
}

TEST_F(FramebufferPoolTest, blitter_surface_is_destroyed_before_its_framebuffer)
{
    Events events;
    record_destruction_in(events);
    auto pool = make_pool();

    pool.reset();

    EXPECT_THAT(events, ElementsAre("surface destroyed", "framebuffer destroyed"));
}

TEST_F(FramebufferPoolTest, blitter_surface_is_destroyed_before_its_framebuffer_when_entry_construction_fails)
{
    Events events;
    record_destruction_in(events);
    ON_CALL(mock_gl, glCheckFramebufferStatus(GL_FRAMEBUFFER)).WillByDefault(Return(GL_FRAMEBUFFER_UNSUPPORTED));

    EXPECT_THROW(make_pool(), std::runtime_error);

    EXPECT_THAT(events, ElementsAre("surface destroyed", "framebuffer destroyed"));
}

TEST_F(FramebufferPoolTest, first_acquire_uses_eagerly_allocated_framebuffer)
{
    EXPECT_CALL(allocator, alloc_fb(_)).Times(1);

    auto const pool = make_pool();
    auto const fb = pool->acquire();
}

TEST_F(FramebufferPoolTest, released_framebuffer_is_recycled)
{
    Events events;
    record_destruction_in(events);
    EXPECT_CALL(allocator, alloc_fb(_)).Times(1);

    auto const pool = make_pool();
    auto fb = pool->acquire();
    auto const* const first_entry = &fb->entry();

    fb.reset();
    fb = pool->acquire();

    EXPECT_THAT(&fb->entry(), Eq(first_entry));
    EXPECT_THAT(events, IsEmpty());
}

TEST_F(FramebufferPoolTest, simultaneously_held_framebuffers_are_distinct)
{
    EXPECT_CALL(allocator, alloc_fb(_)).Times(2);

    auto const pool = make_pool();
    auto const first = pool->acquire();
    auto const second = pool->acquire();

    EXPECT_THAT(&first->entry(), Ne(&second->entry()));
}

TEST_F(FramebufferPoolTest, output_resize_discards_free_framebuffers)
{
    Events events;
    record_destruction_in(events);
    EXPECT_CALL(allocator, alloc_fb(_)).Times(2);
    auto const pool = make_pool();
    geom::Size const new_size{1920, 1080};

    ON_CALL(allocator, output_size()).WillByDefault(Return(new_size));
    auto const fb = pool->acquire();

    EXPECT_THAT(fb->size(), Eq(new_size));
    EXPECT_THAT(events, Contains("framebuffer destroyed"));
}

TEST_F(FramebufferPoolTest, framebuffers_released_after_resize_are_not_recycled)
{
    Events events;
    record_destruction_in(events);
    EXPECT_CALL(allocator, alloc_fb(_)).Times(2);
    auto const pool = make_pool();
    auto stale = pool->acquire();

    ON_CALL(allocator, output_size()).WillByDefault(Return(geom::Size{1920, 1080}));
    auto current = pool->acquire();
    auto const* const current_entry = &current->entry();

    stale.reset();
    EXPECT_THAT(events, Contains("framebuffer destroyed"));

    current.reset();
    auto const recycled = pool->acquire();

    EXPECT_THAT(&recycled->entry(), Eq(current_entry));
}

TEST_F(FramebufferPoolTest, pooled_framebuffer_can_outlive_pool)
{
    Events events;
    record_destruction_in(events);
    auto pool = make_pool();
    auto fb = pool->acquire();

    pool.reset();
    EXPECT_THAT(events, IsEmpty());

    fb.reset();
    EXPECT_THAT(events, ElementsAre("surface destroyed", "framebuffer destroyed"));
}

TEST_F(FramebufferPoolTest, acquire_throws_when_new_framebuffer_cannot_be_built)
{
    auto const pool = make_pool();
    auto const held = pool->acquire();
    ON_CALL(allocator, alloc_fb(_)).WillByDefault(ReturnNull());

    EXPECT_THROW(pool->acquire(), std::runtime_error);
}
