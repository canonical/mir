/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display_buffer.h"
#include "kms_output.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/transformation.h"
#include "bypass.h"
#include "gbm_buffer.h"
#include "mir/fatal.h"
#include "mir/log.h"
#include "native_buffer.h"
#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include MIR_SERVER_GL_H

#include <stdexcept>
#include <chrono>
#include <thread>
#include <algorithm>

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace geom = mir::geometry;

mgm::GBMOutputSurface::FrontBuffer::FrontBuffer()
    : surf{nullptr},
      bo{nullptr}
{
}

mgm::GBMOutputSurface::FrontBuffer::FrontBuffer(gbm_surface* surface)
    : surf{surface},
      bo{gbm_surface_lock_front_buffer(surface)}
{
    if (!bo)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to acquire front buffer of gbm_surface"));
    }
}

mgm::GBMOutputSurface::FrontBuffer::~FrontBuffer()
{
    if (surf)
    {
        gbm_surface_release_buffer(surf, bo);
    }
}

mgm::GBMOutputSurface::FrontBuffer::FrontBuffer(FrontBuffer&& from)
    : surf{from.surf},
      bo{from.bo}
{
    const_cast<gbm_surface*&>(from.surf) = nullptr;
    const_cast<gbm_bo*&>(from.bo) = nullptr;
}

auto mgm::GBMOutputSurface::FrontBuffer::operator=(FrontBuffer&& from) -> FrontBuffer&
{
    if (surf)
    {
        gbm_surface_release_buffer(surf, bo);
    }

    const_cast<gbm_surface*&>(surf) = from.surf;
    const_cast<gbm_bo*&>(bo) = from.bo;

    const_cast<gbm_surface*&>(from.surf) = nullptr;
    const_cast<gbm_bo*&>(from.bo) = nullptr;

    return *this;
}

auto mgm::GBMOutputSurface::FrontBuffer::operator=(std::nullptr_t) -> FrontBuffer&
{
    return *this = FrontBuffer{};
}

mgm::GBMOutputSurface::FrontBuffer::operator gbm_bo*()
{
    return bo;
}

mgm::GBMOutputSurface::FrontBuffer::operator bool() const
{
    return (surf != nullptr) && (bo != nullptr);
}

namespace
{

void ensure_egl_image_extensions()
{
    std::string ext_string;
    const char* exts = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (exts)
        ext_string = exts;

    if (ext_string.find("GL_OES_EGL_image") == std::string::npos)
        BOOST_THROW_EXCEPTION(std::runtime_error("GLES2 implementation doesn't support GL_OES_EGL_image extension"));
}

}

mgm::DisplayBuffer::DisplayBuffer(
    mgm::BypassOption option,
    std::shared_ptr<DisplayReport> const& listener,
    std::vector<std::shared_ptr<KMSOutput>> const& outputs,
    GBMOutputSurface&& surface_gbm,
    geom::Rectangle const& area,
    MirOrientation rot)
    : listener(listener),
      bypass_option(option),
      outputs(outputs),
      surface{std::move(surface_gbm)},
      area(area),
      transform{mg::transformation(rot)},
      needs_set_crtc{false},
      page_flips_pending{false}
{
    uint32_t area_width = area.size.width.as_uint32_t();
    uint32_t area_height = area.size.height.as_uint32_t();
    if (rot == mir_orientation_left || rot == mir_orientation_right)
    {
        fb_width = area_height;
        fb_height = area_width;
    }
    else
    {
        fb_width = area_width;
        fb_height = area_height;
    }

    listener->report_successful_setup_of_native_resources();

    make_current();

    listener->report_successful_egl_make_current_on_construction();

    ensure_egl_image_extensions();

    glClear(GL_COLOR_BUFFER_BIT);

    surface.swap_buffers();

    listener->report_successful_egl_buffer_swap_on_construction();

    visible_composite_frame = surface.lock_front();
    if (!visible_composite_frame)
        fatal_error("Failed to get frontbuffer");

    set_crtc(*outputs.front()->fb_for(visible_composite_frame));

    release_current();

    listener->report_successful_drm_mode_set_crtc_on_construction();
    listener->report_successful_display_construction();
    surface.report_egl_configuration(
        [&listener] (EGLDisplay disp, EGLConfig cfg)
        {
            listener->report_egl_configuration(disp, cfg);
        });
}

mgm::DisplayBuffer::~DisplayBuffer()
{
}

geom::Rectangle mgm::DisplayBuffer::view_area() const
{
    return area;
}

glm::mat2 mgm::DisplayBuffer::transformation() const
{
    return transform;
}

void mgm::DisplayBuffer::set_orientation(MirOrientation const rot, geometry::Rectangle const& a)
{
    transform = mg::transformation(rot);
    area = a;
}

bool mgm::DisplayBuffer::overlay(RenderableList const& renderable_list)
{
    glm::mat2 static const no_transformation;
    if (transform == no_transformation &&
       (bypass_option == mgm::BypassOption::allowed))
    {
        mgm::BypassMatch bypass_match(area);
        auto bypass_it = std::find_if(renderable_list.rbegin(), renderable_list.rend(), bypass_match);
        if (bypass_it != renderable_list.rend())
        {
            auto bypass_buffer = (*bypass_it)->buffer();
            auto native = std::dynamic_pointer_cast<mgm::NativeBuffer>(bypass_buffer->native_buffer_handle());
            if (!native)
                BOOST_THROW_EXCEPTION(std::invalid_argument("could not convert NativeBuffer"));
            if (native->flags & mir_buffer_flag_can_scanout &&
                bypass_buffer->size() == geom::Size{fb_width,fb_height})
            {
                if (auto bufobj = outputs.front()->fb_for(native->bo))
                {
                    bypass_buf = bypass_buffer;
                    bypass_bufobj = bufobj;
                    return true;
                }
            }
        }
    }

    bypass_buf = nullptr;
    bypass_bufobj = nullptr;
    return false;
}

void mgm::DisplayBuffer::for_each_display_buffer(
    std::function<void(graphics::DisplayBuffer&)> const& f)
{
    f(*this);
}

void mgm::DisplayBuffer::swap_buffers()
{
    surface.swap_buffers();
    bypass_buf = nullptr;
    bypass_bufobj = nullptr;
}

void mgm::DisplayBuffer::set_crtc(FBHandle const& forced_frame)
{
    for (auto& output : outputs)
    {
        /*
         * Note that failure to set the CRTC is not a fatal error. This can
         * happen under normal conditions when resizing VirtualBox (which
         * actually removes and replaces the virtual output each time so
         * sometimes it's really not there). Xorg often reports similar
         * errors, and it's not fatal.
         */
        if (!output->set_crtc(forced_frame))
            mir::log_error("Failed to set DRM CRTC. "
                "Screen contents may be incomplete. "
                "Try plugging the monitor in again.");
    }
}

void mgm::DisplayBuffer::post()
{
    /*
     * We might not have waited for the previous frame to page flip yet.
     * This is good because it maximizes the time available to spend rendering
     * each frame. Just remember wait_for_page_flip() must be called at some
     * point before the next schedule_page_flip().
     */
    wait_for_page_flip();

    mgm::FBHandle *bufobj;
    if (bypass_buf)
    {
        bufobj = bypass_bufobj;
    }
    else
    {
        scheduled_composite_frame = surface.lock_front();
        bufobj = outputs.front()->fb_for(scheduled_composite_frame);
        if (!bufobj)
            fatal_error("Failed to get front buffer object");
    }

    /*
     * Try to schedule a page flip as first preference to avoid tearing.
     * [will complete in a background thread]
     */
    if (!needs_set_crtc && !schedule_page_flip(*bufobj))
        needs_set_crtc = true;

    /*
     * Fallback blitting: Not pretty, since it may tear. VirtualBox seems
     * to need to do this on every frame. [will complete in this thread]
     */
    if (needs_set_crtc)
    {
        set_crtc(*bufobj);
        needs_set_crtc = false;
    }

    using namespace std;  // For operator""ms()

    // Predicted worst case render time for the next frame...
    auto predicted_render_time = 50ms;

    if (bypass_buf)
    {
        /*
         * For composited frames we defer wait_for_page_flip till just before
         * the next frame, but not for bypass frames. Deferring the flip of
         * bypass frames would increase the time we held
         * visible_bypass_frame unacceptably, resulting in client stuttering
         * unless we allocate more buffers (which I'm trying to avoid).
         * Also, bypass does not need the deferred page flip because it has
         * no compositing/rendering step for which to save time for.
         */
        scheduled_bypass_frame = bypass_buf;
        wait_for_page_flip();

        // It's very likely the next frame will be bypassed like this one so
        // we only need time for kernel page flip scheduling...
        predicted_render_time = 5ms;
    }
    else
    {
        /*
         * Not in clone mode? We can afford to wait for the page flip then,
         * making us double-buffered (noticeably less laggy than the triple
         * buffering that clone mode requires).
         */
        if (outputs.size() == 1)
            wait_for_page_flip();

        /*
         * TODO: If you're optimistic about your GPU performance and/or
         *       measure it carefully you may wish to set predicted_render_time
         *       to a lower value here for lower latency.
         *
         *predicted_render_time = 9ms; // e.g. about the same as Weston
         */
    }

    // Buffer lifetimes are managed exclusively by scheduled*/visible* now
    bypass_buf = nullptr;
    bypass_bufobj = nullptr;

    recommend_sleep = 0ms;
    if (outputs.size() == 1)
    {
        auto const& output = outputs.front();
        auto const min_frame_interval = 1000ms / output->max_refresh_rate();
        if (predicted_render_time < min_frame_interval)
            recommend_sleep = min_frame_interval - predicted_render_time;
    }
}

std::chrono::milliseconds mgm::DisplayBuffer::recommended_sleep() const
{
    return recommend_sleep;
}

bool mgm::DisplayBuffer::schedule_page_flip(FBHandle const& bufobj)
{
    /*
     * Schedule the current front buffer object for display. Note that
     * the page flip is asynchronous and synchronized with vertical refresh.
     */
    for (auto& output : outputs)
    {
        if (output->schedule_page_flip(bufobj))
            page_flips_pending = true;
    }

    return page_flips_pending;
}

void mgm::DisplayBuffer::wait_for_page_flip()
{
    if (page_flips_pending)
    {
        for (auto& output : outputs)
            output->wait_for_page_flip();

        page_flips_pending = false;
    }

    if (scheduled_bypass_frame || scheduled_composite_frame)
    {
        // Why are both of these grouped into a single statement?
        // Because in either case both types of frame need releasing each time.

        visible_bypass_frame = scheduled_bypass_frame;
        scheduled_bypass_frame = nullptr;
    
        visible_composite_frame = std::move(scheduled_composite_frame);
        scheduled_composite_frame = nullptr;
    }
}

void mgm::DisplayBuffer::make_current()
{
    surface.make_current();
}

void mgm::DisplayBuffer::bind()
{
    surface.bind();
}

void mgm::DisplayBuffer::release_current()
{
    surface.release_current();
}

void mgm::DisplayBuffer::schedule_set_crtc()
{
    needs_set_crtc = true;
}

mg::NativeDisplayBuffer* mgm::DisplayBuffer::native_display_buffer()
{
    return this;
}

mgm::GBMOutputSurface::GBMOutputSurface(
    int drm_fd,
    GBMSurfaceUPtr&& surface,
    uint32_t width,
    uint32_t height,
    helpers::EGLHelper&& egl)
    : drm_fd{drm_fd},
      width{width},
      height{height},
      egl{std::move(egl)},
      surface{std::move(surface)}
{
}

mgm::GBMOutputSurface::GBMOutputSurface(GBMOutputSurface&& from)
    : drm_fd{from.drm_fd},
      width{from.width},
      height{from.height},
      egl{std::move(from.egl)},
      surface{std::move(from.surface)}
{
}


void mgm::GBMOutputSurface::make_current()
{
    if (!egl.make_current())
    {
        fatal_error("Failed to make EGL surface current");
    }
}

void mgm::GBMOutputSurface::release_current()
{
    egl.release_current();
}

void mgm::GBMOutputSurface::swap_buffers()
{
    if (!egl.swap_buffers())
        fatal_error("Failed to perform buffer swap");
}

void mgm::GBMOutputSurface::bind()
{

}

auto mgm::GBMOutputSurface::lock_front() -> FrontBuffer
{
    return FrontBuffer{surface.get()};
}

void mgm::GBMOutputSurface::report_egl_configuration(
    std::function<void(EGLDisplay, EGLConfig)> const& to)
{
    egl.report_egl_configuration(to);
}
