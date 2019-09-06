/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "compositing_screencast.h"
#include "screencast_display_buffer.h"
#include "queueing_schedule.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/display.h"
#include "mir/graphics/virtual_output.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/graphics/transformation.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/geometry/rectangles.h"
#include "mir/raii.h"

#include <boost/throw_exception.hpp>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
uint32_t const max_screencast_sessions{100};

bool needs_virtual_output(mg::DisplayConfiguration const& conf, geom::Rectangle const& region)
{
    geom::Rectangles disp_rects;
    conf.for_each_output([&disp_rects](mg::DisplayConfigurationOutput const& disp_conf)
    {
        if (disp_conf.connected)
            disp_rects.add(disp_conf.extents());
    });

    geom::Rectangle empty{};

    return empty == disp_rects.bounding_rectangle().intersection_with(region);
}

std::unique_ptr<mg::VirtualOutput> make_virtual_output(mg::Display& display, geom::Rectangle const& rect)
{
    if (needs_virtual_output(*display.configuration(), rect))
    {
        return display.create_virtual_output(rect.size.width.as_int(), rect.size.height.as_int());
    }
    return nullptr;
}
}

class mc::detail::ScreencastSessionContext
{
public:
    ScreencastSessionContext(
        std::shared_ptr<Scene> const& scene,
        mg::Display& display,
        DisplayBufferCompositorFactory& db_compositor_factory,
        std::vector<std::shared_ptr<mg::Buffer>> const& buffers,
        geom::Rectangle const& capture_region,
        geom::Size const& capture_size,
        MirMirrorMode mirror_mode)
    : scene{scene},
      display_buffer{std::make_unique<ScreencastDisplayBuffer>(capture_region, capture_size, mirror_mode, free_queue, ready_queue, display)},
      display_buffer_compositor{db_compositor_factory.create_compositor_for(*display_buffer)},
      virtual_output{make_virtual_output(display, capture_region)},
      queue_size(capture_size),
      mirror_mode(mirror_mode)
    {
        for (auto buffer : buffers)
            free_queue.schedule(buffer);

        scene->register_compositor(this);
        if (virtual_output)
            virtual_output->enable();
    }
    ~ScreencastSessionContext()
    {
        scene->unregister_compositor(this);
    }

    std::shared_ptr<mg::Buffer> capture()
    {
        std::lock_guard<decltype(mutex)> lk(mutex);
        if (queue_size != display_buffer->renderbuffer_size())
            display_buffer->set_renderbuffer_size(queue_size);

        //FIXME:: the client needs a better way to express it is no longer
        //using the last captured buffer
        if (last_captured_buffer)
            free_queue.schedule(last_captured_buffer);

        display_buffer_compositor->composite(scene->scene_elements_for(this, display_buffer->view_area()));

        last_captured_buffer = ready_queue.next_buffer();
        return last_captured_buffer;
    }

    void capture(std::shared_ptr<mg::Buffer> const& buffer)
    {
        std::lock_guard<decltype(mutex)> lk(mutex);
        if (buffer->size() != display_buffer->renderbuffer_size())
            display_buffer->set_renderbuffer_size(buffer->size());
       
        //a bit confusingly, the old way of screencasting had the mirror_mode_none
        //produce upside down buffers.
        if (mirror_mode == mir_mirror_mode_none)
            display_buffer->set_transformation(mg::transformation(mir_mirror_mode_vertical));
        if (mirror_mode == mir_mirror_mode_vertical)
            display_buffer->set_transformation(mg::transformation(mir_mirror_mode_none));
        if (mirror_mode == mir_mirror_mode_horizontal)
        {
            glm::mat2 mat;
            mat[0][0] = -1;
            mat[1][1] = -1;
            display_buffer->set_transformation(mat);
        }
 
        auto scheduled = free_queue.num_scheduled();
        free_queue.schedule(buffer);
        for(auto i = 0u; i < scheduled; i++)
            free_queue.schedule(free_queue.next_buffer());

        display_buffer_compositor->composite(scene->scene_elements_for(this, display_buffer->view_area()));
        if (buffer != ready_queue.next_buffer())
            throw std::runtime_error("unable to capture to buffer");

        display_buffer->set_transformation(mg::transformation(mirror_mode));
        display_buffer->commit();
    }

private:
    std::mutex mutex;
    std::shared_ptr<Scene> const scene;
    QueueingSchedule free_queue;
    QueueingSchedule ready_queue;
    std::unique_ptr<ScreencastDisplayBuffer> display_buffer;

    std::unique_ptr<compositor::DisplayBufferCompositor> display_buffer_compositor;
    std::unique_ptr<graphics::VirtualOutput> virtual_output;
    std::shared_ptr<mg::Buffer> last_captured_buffer;
    geom::Size queue_size;
    MirMirrorMode mirror_mode;
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
    MirPixelFormat const pixel_format,
    int nbuffers,
    MirMirrorMode mirror_mode)
{
#ifdef MIR_EGL_SUPPORTED
    static auto const buffer_usage = mg::BufferUsage::hardware;
#else
    static auto const buffer_usage = mg::BufferUsage::software;
#endif
    
    if (size.width.as_int() == 0 ||
        size.height.as_int() == 0 ||
        region.size.width.as_int() == 0 ||
        region.size.height.as_int() == 0 ||
        ((nbuffers > 1) && (pixel_format == mir_pixel_format_invalid)))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Invalid parameters"));
    }
    std::lock_guard<decltype(session_mutex)> lock{session_mutex};
    auto const id = next_available_session_id();

    std::vector<std::shared_ptr<mg::Buffer>> buffers(nbuffers);
    for (auto& buffer : buffers)
    {
        buffer = buffer_allocator->alloc_buffer(mg::BufferProperties{size, pixel_format, buffer_usage});
    }
    session_contexts[id] = create_session_context(region, size, buffers, mirror_mode);

    return id;
}

void mc::CompositingScreencast::destroy_session(mf::ScreencastSessionId id)
{
    std::lock_guard<decltype(session_mutex)> lock{session_mutex};
    session_contexts.erase(id);
}

std::shared_ptr<mc::detail::ScreencastSessionContext> mc::CompositingScreencast::session(mf::ScreencastSessionId id)
{
    std::lock_guard<decltype(session_mutex)> lock{session_mutex};
    return session_contexts.at(id);
}

std::shared_ptr<mg::Buffer> mc::CompositingScreencast::capture(mf::ScreencastSessionId id)
{
    return session(id)->capture();
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
    std::vector<std::shared_ptr<mg::Buffer>> const& buffers,
    MirMirrorMode mirror_mode)
{
    return std::make_shared<detail::ScreencastSessionContext>(
        scene, *display, *db_compositor_factory, buffers, rect, size, mirror_mode);
}

void mc::CompositingScreencast::capture(
    mf::ScreencastSessionId id, std::shared_ptr<mg::Buffer> const& b)
{
    session(id)->capture(b);
}
