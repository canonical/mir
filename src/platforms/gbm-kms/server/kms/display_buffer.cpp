/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "display_sink.h"
#include <mir/graphics/kms_cpu_addressable_display_provider.h>
#include "kms_output.h"
#include <mir/graphics/cpu_addressable_fb.h>
#include "gbm_display_allocator.h"
#include <mir/fd.h>
#include <mir/graphics/display_report.h>
#include <mir/graphics/platform.h>
#include <mir/graphics/transformation.h>
#include "bypass.h"
#include <mir/fatal.h>
#include <mir/log.h>
#include "display_helpers.h"
#include "egl_helper.h"
#include <mir/graphics/egl_error.h>
#include <mir/graphics/gl_config.h>
#include <mir/graphics/dmabuf_buffer.h>
#include <mir/debug_frame_trace.h>

#include <boost/throw_exception.hpp>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <drm_fourcc.h>

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <algorithm>

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace geom = mir::geometry;

mgg::DisplaySink::DisplaySink(
    mir::Fd drm_fd,
    std::shared_ptr<struct gbm_device> gbm,
    mgg::BypassOption,
    std::shared_ptr<DisplayReport> const& listener,
    std::shared_ptr<KMSOutput> const& output,
    geom::Rectangle const& area,
    glm::mat2 const& transformation)
    : gbm{std::move(gbm)},
      listener(listener),
      output(output),
      area(area),
      transform{transformation},
      needs_set_crtc{false},
      page_flips_pending{false}
{
    listener->report_successful_setup_of_native_resources();

    // If any of the outputs have a CRTC mismatch, we will want to set all of them
    // so that they're all showing the same buffer.
    bool has_crtc_mismatch = output->has_crtc_mismatch();

    if (has_crtc_mismatch)
    {
        mir::log_info("Clearing screen due to differing encountered and target modes");
        // TODO: Pull a supported format out of KMS rather than assuming XRGB8888
        auto initial_fb = std::make_shared<graphics::CPUAddressableFB>(
            std::move(drm_fd),
            false,
            DRMFormat{DRM_FORMAT_XRGB8888},
            area.size);

        auto mapping = initial_fb->map_writeable();
        ::memset(mapping->data(), 24, mapping->len());

        visible_fb = std::move(initial_fb);
        output->set_crtc(*visible_fb);
        listener->report_successful_drm_mode_set_crtc_on_construction();
    }
    listener->report_successful_display_construction();
}

mgg::DisplaySink::~DisplaySink() = default;

geom::Rectangle mgg::DisplaySink::view_area() const
{
    return area;
}

glm::mat2 mgg::DisplaySink::transformation() const
{
    return transform;
}

void mgg::DisplaySink::set_transformation(glm::mat2 const& t, geometry::Rectangle const& a)
{
    transform = t;
    area = a;
}

bool mgg::DisplaySink::overlay(std::vector<DisplayElement> const& renderable_list)
{
    // TODO: implement more than the most basic case.
    if (renderable_list.size() != 1)
    {
        return false;
    }

    if (renderable_list[0].screen_positon != view_area())
    {
        return false;
    }

    if (renderable_list[0].source_position.top_left != geom::PointF {0,0} ||
        renderable_list[0].source_position.size.width.as_value() != view_area().size.width.as_int() ||
        renderable_list[0].source_position.size.height.as_value() != view_area().size.height.as_int())
    {
        return false;
    }

    if (auto fb = std::dynamic_pointer_cast<graphics::FBHandle>(renderable_list[0].buffer))
    {
        next_swap = std::move(fb);
        return true;
    }
    return false;
}

auto mgg::DisplaySink::plan_presentation(std::vector<std::shared_ptr<Renderable>> const&)
    -> std::optional<LogicalStacking>
{ return std::nullopt; }

void mgg::DisplaySink::for_each_display_sink(std::function<void(graphics::DisplaySink&)> const& f)
{
    // When an output is disconnected, its sink will have a size of 0x0.
    // This may cause us to allocate a buffer of zero size, which leads
    // to all sorts of problems (e.g. an error during eglSwapBuffers
    // because the back buffer is empty). To avoid this, we don't iterate
    // over display sinks that refer to disconnected outputs.
    if (!output->connected())
        return;

    f(*this);
}

bool mgg::DisplaySink::set_crtc(FBHandle const& forced_frame)
{
    /*
     * Note that failure to set the CRTC is not a fatal error. This can
     * happen under normal conditions when resizing VirtualBox (which
     * actually removes and replaces the virtual output each time so
     * sometimes it's really not there). Xorg often reports similar
     * errors, and it's not fatal.
     */
    if (!output->set_crtc(forced_frame))
    {
        mir::log_error("Failed to set DRM CRTC. "
            "Screen contents may be incomplete. "
            "Try plugging the monitor in again.");
        return false;
    }
    return true;
}

bool mgg::DisplaySink::post()
{
    /* THROWAWAY (#5325): Tier C — the paths below used to be silent, which made
     * "composited but never presented" indistinguishable from "never composited".
     */
    auto const trace_key = debug_trace_key();

    if (!next_swap)
    {
        // Hey! No one has given us a next frame yet, so we don't have to change what's onscreen.
        // Sweet! We can just bail.
        if (mir::debug::tracing())
        {
            mir::debug::trace("post: no new content for output {} (connector {})", trace_key, output->id());
            mir::debug::update_output_trace(
                trace_key,
                [this](mir::debug::OutputTrace& trace)
                {
                    ++trace.skipped_posts;
                    trace.connector = std::to_string(output->id());
                });
        }
        return true;
    }
    /*
     * Otherwise, pull the next frame into the pending slot
     */
    scheduled_fb = std::move(next_swap);
    next_swap = nullptr;

    /*
     * Try to schedule a page flip as first preference to avoid tearing.
     * [will complete in a background thread]
     */
    auto const scheduled_fb_id = static_cast<uint32_t>(*scheduled_fb);
    if (!needs_set_crtc && !schedule_page_flip(*scheduled_fb))
    {
        needs_set_crtc = true;
        mir::debug::trace_warning(
            "post: page flip NOT scheduled for output {} (connector {}), fb={}; falling back to set_crtc",
            trace_key, output->id(), scheduled_fb_id);
        mir::debug::update_output_trace(
            trace_key, [](mir::debug::OutputTrace& trace) { ++trace.failed_flips; });
    }

    /*
     * Fallback blitting: Not pretty, since it may tear. VirtualBox seems
     * to need to do this on every frame. [will complete in this thread]
     */
    auto presentation_succeeded = true;
    if (needs_set_crtc)
    {
        if (mir::debug::tracing())
        {
            mir::debug::trace_warning(
                "post: set_crtc fallback for output {} (connector {}), fb={}",
                trace_key, output->id(), scheduled_fb_id);
            mir::debug::update_output_trace(
                trace_key, [](mir::debug::OutputTrace& trace) { ++trace.set_crtc_fallbacks; });
        }

        presentation_succeeded = set_crtc(*scheduled_fb);
        // SetCrtc is immediate, so the FB is now visible and we have nothing pending
        visible_fb = std::move(scheduled_fb);
        scheduled_fb = nullptr;

        needs_set_crtc = false;
    }

    using namespace std::chrono_literals;  // For operator""ms()

    // Predicted worst case render time for the next frame...
    auto predicted_render_time = 50ms;

    recommend_sleep = 0ms;

    // Wait for the frame to be displayed
    // This makes us double-buffered; assuming we can reliably render within the frame deadline
    // that minimises latency.
    wait_for_page_flip();

    if (mir::debug::tracing())
    {
        mir::debug::trace(
            "post: output {} (connector {}) presented fb={} ok={}",
            trace_key, output->id(), scheduled_fb_id, presentation_succeeded);
        mir::debug::update_output_trace(
            trace_key,
            [this, scheduled_fb_id](mir::debug::OutputTrace& trace)
            {
                ++trace.posts;
                trace.connector = std::to_string(output->id());
                trace.last_scheduled_fb = scheduled_fb_id;
                trace.last_visible_fb = visible_fb ? static_cast<uint32_t>(*visible_fb) : 0;
                trace.last_flip = mir::debug::now_seconds();
                trace.needs_set_crtc = needs_set_crtc;
                trace.page_flips_pending = page_flips_pending;
            });
    }

    auto const min_frame_interval = 1000ms / output->max_refresh_rate();
    if (predicted_render_time < min_frame_interval)
        recommend_sleep = min_frame_interval - predicted_render_time;

    return presentation_succeeded;
}

auto mgg::DisplaySink::debug_trace_key() const -> std::string
{
    auto const area = view_area();
    return mir::debug::output_key(
        area.size.width.as_int(), area.size.height.as_int(),
        area.top_left.x.as_int(), area.top_left.y.as_int());
}

std::chrono::milliseconds mgg::DisplaySink::recommended_sleep() const
{
    return recommend_sleep;
}

bool mgg::DisplaySink::schedule_page_flip(FBHandle const& bufobj)
{
    /*
     * Schedule the current front buffer object for display. Note that
     * the page flip is asynchronous and synchronized with vertical refresh.
     */
    if (output->schedule_page_flip(bufobj))
        page_flips_pending = true;

    return page_flips_pending;
}

void mgg::DisplaySink::wait_for_page_flip()
{
    if (page_flips_pending)
    {
        output->wait_for_page_flip();

        // The previously-scheduled FB has been page-flipped, and is now visible
        visible_fb = std::move(scheduled_fb);
        scheduled_fb = nullptr;

        page_flips_pending = false;
    }
}

void mgg::DisplaySink::schedule_set_crtc()
{
    needs_set_crtc = true;
}

auto mgg::DisplaySink::drm_fd() const -> mir::Fd
{
    return mir::Fd{mir::IntOwnedFd{output->drm_fd()}};
}

auto mgg::DisplaySink::gbm_device() const -> std::shared_ptr<struct gbm_device>
{
    return gbm;
}

void mir::graphics::gbm::DisplaySink::set_next_image(std::unique_ptr<Framebuffer> content)
{
    std::vector<DisplayElement> const single_buffer = {
        DisplayElement {
            view_area(),
            geom::RectangleF{
                {0, 0},
                {view_area().size.width.as_value(), view_area().size.height.as_value()}},
            std::move(content)
        }
    };
    if (!overlay(single_buffer))
    {
        // Oh, oh! We should be *guaranteed* to “overlay” a single Framebuffer; this is likely a programming error
        BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to post buffer to display"}));
    }
}

auto mgg::DisplaySink::maybe_create_allocator(DisplayAllocator::Tag const& type_tag)
    -> DisplayAllocator*
{
    if (dynamic_cast<mg::CPUAddressableDisplayAllocator::Tag const*>(&type_tag))
    {
        if (!kms_allocator)
        {
            kms_allocator = kms::CPUAddressableDisplayAllocator::create_if_supported(drm_fd(), output->size());
        }
        return kms_allocator.get();
    }
    if (dynamic_cast<mg::GBMDisplayAllocator::Tag const*>(&type_tag))
    {
        if (!gbm_allocator)
        {
            gbm_allocator = std::make_unique<GBMDisplayAllocator>(drm_fd(), gbm, output->size());
        }
        return gbm_allocator.get();
    }
    return nullptr;
}
