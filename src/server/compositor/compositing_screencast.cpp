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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "compositing_screencast.h"
#include "screencast_display_buffer.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/raii.h"

#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
uint32_t const max_screencast_sessions{100};
}

struct mc::detail::ScreencastSessionContext
{
    ScreencastSessionContext(
        std::shared_ptr<Scene> const& scene,
        std::shared_ptr<graphics::Buffer> const& buffer,
        std::unique_ptr<graphics::GLContext> gl_context,
        std::unique_ptr<graphics::DisplayBuffer> display_buffer,
        std::unique_ptr<compositor::DisplayBufferCompositor> display_buffer_compositor) :
    scene{scene},
    buffer{buffer},
    gl_context{std::move(gl_context)},
    display_buffer{std::move(display_buffer)},
    display_buffer_compositor{std::move(display_buffer_compositor)}
    {
        scene->register_compositor(this);
    }
    ~ScreencastSessionContext()
    {
        scene->unregister_compositor(this);
    }

    std::shared_ptr<Scene> const scene;
    std::shared_ptr<graphics::Buffer> buffer;
    std::unique_ptr<graphics::GLContext> gl_context;
    std::unique_ptr<graphics::DisplayBuffer> display_buffer;
    std::unique_ptr<compositor::DisplayBufferCompositor> display_buffer_compositor;
};


mc::CompositingScreencast::CompositingScreencast(
    std::shared_ptr<Scene> const& scene,
    std::shared_ptr<mg::Display> const& display,
    std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
    std::shared_ptr<DisplayBufferCompositorFactory> const& db_compositor_factory)
    : scene{scene},
      display{display},
      buffer_allocator{buffer_allocator},
      db_compositor_factory{db_compositor_factory}
{
}

mf::ScreencastSessionId mc::CompositingScreencast::create_session(
    geom::Rectangle const& region,
    geom::Size const& size,
    MirPixelFormat const pixel_format)
{
    if (size.width.as_int() == 0 ||
        size.height.as_int() == 0 ||
        region.size.width.as_int() == 0 ||
        region.size.height.as_int() == 0 ||
        pixel_format == mir_pixel_format_invalid)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid parameters"));
    }
    std::lock_guard<decltype(session_mutex)> lock{session_mutex};
    auto const id = next_available_session_id();
    session_contexts[id] = create_session_context(region, size, pixel_format);

    return id;
}

void mc::CompositingScreencast::destroy_session(mf::ScreencastSessionId id)
{
    std::lock_guard<decltype(session_mutex)> lock{session_mutex};
    auto gl_context = std::move(session_contexts.at(id)->gl_context);

    auto using_gl_context = mir::raii::paired_calls(
        [&] { gl_context->make_current(); },
        [&] { gl_context->release_current(); });

    session_contexts.erase(id);
}

std::shared_ptr<mg::Buffer> mc::CompositingScreencast::capture(mf::ScreencastSessionId id)
{
    std::shared_ptr<detail::ScreencastSessionContext> session_context;

    {
        std::lock_guard<decltype(session_mutex)> lock{session_mutex};
        session_context = session_contexts.at(id);
    }

    auto using_gl_context = mir::raii::paired_calls(
        [&] { session_context->gl_context->make_current(); },
        [&] { session_context->gl_context->release_current(); });

    session_context->display_buffer_compositor->composite(
        session_context->scene->scene_elements_for(session_context.get()));

    return session_context->buffer;
}

mf::ScreencastSessionId mc::CompositingScreencast::next_available_session_id()
{
    for (uint32_t i = 1; i <= max_screencast_sessions; ++i)
    {
        mf::ScreencastSessionId const id{i};
        if (session_contexts.find(id) == session_contexts.end())
            return id;
    }

    BOOST_THROW_EXCEPTION(std::runtime_error("Too many screencast sessions!"));
}

std::shared_ptr<mc::detail::ScreencastSessionContext>
mc::CompositingScreencast::create_session_context(
    geometry::Rectangle const& rect,
    geometry::Size const& size,
    MirPixelFormat pixel_format)
{
    mg::BufferProperties buffer_properties{
        size,
        pixel_format,
        mg::BufferUsage::hardware};

    auto gl_context = display->create_gl_context();
    auto gl_context_raw = gl_context.get();

    auto using_gl_context = mir::raii::paired_calls(
        [&] { gl_context_raw->make_current(); },
        [&] { gl_context_raw->release_current(); });

    auto buffer = buffer_allocator->alloc_buffer(buffer_properties);
    auto display_buffer = std::make_unique<ScreencastDisplayBuffer>(rect, *buffer);
    auto db_compositor = db_compositor_factory->create_compositor_for(*display_buffer);

    return std::shared_ptr<detail::ScreencastSessionContext>(
        new detail::ScreencastSessionContext{
            scene,
            buffer,
            std::move(gl_context),
            std::move(display_buffer),
            std::move(db_compositor)});
}
