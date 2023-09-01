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

#include <epoxy/egl.h>

#include "display.h"
#include "egl_output.h"

#include "kms-utils/drm_mode_resources.h"
#include "threaded_drm_event_handler.h"

#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/overlapping_output_grouping.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/transformation.h"
#include "mir/renderer/gl/render_target.h"
#include "mir/renderer/gl/context.h"
#include "mir/graphics/egl_extensions.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/event_handler_register.h"

#include "mir/udev/wrapper.h"

#define MIR_LOG_COMPONENT "platform-eglstream-kms"
#include "mir/log.h"

#include <xf86drmMode.h>
#include <system_error>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mge = mir::graphics::eglstream;
namespace mgk = mir::graphics::kms;

#ifndef EGL_NV_output_drm_flip_event
#define EGL_NV_output_drm_flip_event 1
#define EGL_DRM_FLIP_EVENT_DATA_NV            0x333E
#endif /* EGL_NV_output_drm_flip_event */

namespace
{
EGLConfig choose_config(EGLDisplay display, mg::GLConfig const& requested_config)
{
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_STREAM_BIT_KHR,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, requested_config.depth_buffer_bits(),
        EGL_STENCIL_SIZE, requested_config.stencil_buffer_bits(),
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLint num_egl_configs;
    EGLConfig egl_config;
    if (eglChooseConfig(display, config_attr, &egl_config, 1, &num_egl_configs) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to chose EGL config"));
    }
    else if (num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error{"Failed to find compatible EGL config"});
    }

    return egl_config;
}

EGLContext create_context(EGLDisplay display, EGLConfig config)
{
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint const context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, context_attr);
    if (context == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }

    return context;
}

EGLContext create_context(EGLDisplay display, EGLConfig config, EGLContext shared_context)
{
    eglBindAPI(EGL_OPENGL_ES_API);

    EGLint const context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLContext context = eglCreateContext(display, config, shared_context, context_attr);
    if (context == EGL_NO_CONTEXT)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    }

    return context;
}

class DisplayBuffer
    : public mg::DisplaySyncGroup,
      public mg::DisplayBuffer
{
public:
    DisplayBuffer(
        std::shared_ptr<mg::DisplayInterfaceProvider> owner,
        mir::Fd drm_node,
        EGLDisplay dpy,
        EGLContext ctx,
        EGLConfig config,
        std::shared_ptr<mge::DRMEventHandler> event_handler,
        mge::kms::EGLOutput const& output,
        std::shared_ptr<mg::DisplayReport> display_report)
        : owner{std::move(owner)},
          dpy{dpy},
          ctx{create_context(dpy, config, ctx)},
          layer{output.output_layer()},
          crtc_id{output.crtc_id()},
          view_area_{output.extents()},
          output_size{output.size()},
          transform{output.transformation()},
          drm_node{std::move(drm_node)},
          event_handler{std::move(event_handler)},
          display_report{std::move(display_report)}
    {
        EGLint const stream_attribs[] = {
            EGL_STREAM_FIFO_LENGTH_KHR, 1,
            EGL_CONSUMER_AUTO_ACQUIRE_EXT, EGL_FALSE,
            EGL_NONE
        };
        output_stream = eglCreateStreamKHR(dpy, stream_attribs);
        if (output_stream == EGL_NO_STREAM_KHR)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGLStream"));
        }

        EGLAttrib swap_interval;
        if (eglQueryOutputLayerAttribEXT(dpy, layer, EGL_SWAP_INTERVAL_EXT, &swap_interval) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query swap interval"));
        }

        if (eglOutputLayerAttribEXT(dpy, layer, EGL_SWAP_INTERVAL_EXT, 1) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to set swap interval"));
        }

        if (eglStreamConsumerOutputEXT(dpy, output_stream, output.output_layer()) == EGL_FALSE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to attach EGLStream to output"));
        };

        EGLint const surface_attribs[] = {
            EGL_WIDTH, output_size.width.as_int(),
            EGL_HEIGHT, output_size.height.as_int(),
            EGL_NONE,
        };
        surface = eglCreateStreamProducerSurfaceKHR(dpy, config, output_stream, surface_attribs);
        if (surface == EGL_NO_SURFACE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create StreamProducerSurface"));
        }

        this->display_report->report_successful_setup_of_native_resources();
        this->display_report->report_successful_display_construction();
        this->display_report->report_egl_configuration(dpy, config);

        // The first flip needs no wait!
        std::promise<void> satisfied_promise;
        satisfied_promise.set_value();
        pending_flip = satisfied_promise.get_future();
    }

    /* gl::RenderTarget */
    auto size() const -> mir::geometry::Size override
    {
        return output_size;
    }

    void make_current() override
    {
        if (eglMakeCurrent(dpy, surface, surface, ctx) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
        }
    }

    void release_current() override
    {
        if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release context"));
        }
    }

    void swap_buffers() override
    {
        if (eglSwapBuffers(dpy, surface) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("eglSwapBuffers failed"));
        }
    }

    mir::geometry::Rectangle view_area() const override
    {
        return view_area_;
    }

    bool overlay(std::vector<mg::DisplayElement> const&) override
    {
        return false;
    }

    glm::mat2 transformation() const override
    {
        return transform;
    }

    void for_each_display_buffer(const std::function<void(mir::graphics::DisplayBuffer&)>& f) override
    {
        f(*this);
    }

    void post() override
    {
        // Wait for the last flip to finish, if it hasn't already.
        pending_flip.get();

        pending_flip = event_handler->expect_flip_event(
            crtc_id,
            [this](unsigned frame_count, std::chrono::milliseconds frame_time)
            {
                // TODO: Um, why does NVIDIA always call this with 0, 0ms?
                display_report->report_vsync(
                    crtc_id,
                    mg::Frame {
                        frame_count,
                        mir::time::PosixTimestamp(CLOCK_MONOTONIC, frame_time)});
            });

        EGLAttrib const acquire_attribs[] = {
            EGL_DRM_FLIP_EVENT_DATA_NV, reinterpret_cast<EGLAttrib>(event_handler->drm_event_data()),
            EGL_NONE
        };
        if (nv_stream(dpy).eglStreamConsumerAcquireAttribNV(dpy, output_stream, acquire_attribs) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to submit frame from EGLStream for display"));
        }
    }

    void bind() override
    {
    }

    std::chrono::milliseconds recommended_sleep() const override
    {
        return std::chrono::milliseconds{0};
    }

private:

    std::shared_ptr<mg::DisplayInterfaceProvider> const owner;
    EGLDisplay dpy;
    EGLContext ctx;
    EGLOutputLayerEXT layer;
    uint32_t crtc_id;
    mir::geometry::Rectangle const view_area_;
    mir::geometry::Size const output_size;
    glm::mat2 const transform;
    EGLStreamKHR output_stream;
    EGLSurface surface;
    mir::Fd const drm_node;
    std::shared_ptr<mge::DRMEventHandler> const event_handler;
    std::future<void> pending_flip;
    mg::EGLExtensions::LazyDisplayExtensions<mg::EGLExtensions::NVStreamAttribExtensions> nv_stream;
    std::shared_ptr<mg::DisplayReport> const display_report;
};

mge::KMSDisplayConfiguration create_display_configuration(
    mir::Fd const& drm_node,
    EGLDisplay dpy,
    EGLContext context)
{
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, context) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION((
            mg::egl_error("Failed to make EGL context current for display construction")));
    }
    return mge::KMSDisplayConfiguration{drm_node, dpy};
}
}

mge::Display::Display(
    mir::Fd drm_node,
    EGLDisplay display,
    std::shared_ptr<DisplayConfigurationPolicy> const& configuration_policy,
    GLConfig const& gl_conf,
    std::shared_ptr<DisplayReport> display_report)
    : drm_node{drm_node},
      display{display},
      config{choose_config(display, gl_conf)},
      context{create_context(display, config)},
      display_configuration{create_display_configuration(this->drm_node, display, context)},
      event_handler{std::make_shared<ThreadedDRMEventHandler>(drm_node)},
      display_report{std::move(display_report)}
{
    auto ret = drmSetClientCap(drm_node, DRM_CLIENT_CAP_UNIVERSAL_PLANES, 1);
    if (ret != 0)
    {
        BOOST_THROW_EXCEPTION(std::system_error(-ret, std::system_category(), "Request for Universal Planes support failed"));
    }

    ret = drmSetClientCap(drm_node, DRM_CLIENT_CAP_ATOMIC, 1);
    if (ret != 0)
    {
        BOOST_THROW_EXCEPTION(std::system_error(-ret, std::system_category(), "Request for Atomic Modesetting support failed"));
    }

    configuration_policy->apply_to(display_configuration);

    configure(display_configuration);
}

void mge::Display::for_each_display_sync_group(const std::function<void(DisplaySyncGroup&)>& f)
{
    for (auto& group : active_sync_groups)
    {
        f(*group);
    }
}

std::unique_ptr<mg::DisplayConfiguration> mge::Display::configuration() const
{
    std::lock_guard lock{configuration_mutex};
    return display_configuration.clone();
}

void mge::Display::configure(DisplayConfiguration const& conf)
{
    auto kms_conf = dynamic_cast<KMSDisplayConfiguration const&>(conf);
    active_sync_groups.clear();
    kms_conf.for_each_output([this](kms::EGLOutput const& output)
         {
             if (output.used)
             {
                 const_cast<kms::EGLOutput&>(output).configure(output.current_mode_index);
                 active_sync_groups.emplace_back(
                     std::make_unique<::DisplayBuffer>(
                         drm_node,
                         display,
                         context,
                         config,
                         event_handler,
                         output,
                         display_report));
             }
         });
}

namespace
{
std::unique_ptr<mir::udev::Monitor> create_drm_monitor()
{
    auto monitor = std::make_unique<mir::udev::Monitor>(mir::udev::Context());
    monitor->filter_by_subsystem_and_type("drm", "drm_minor");
    monitor->enable();
    return monitor;
}
}

void mge::Display::register_configuration_change_handler(
    EventHandlerRegister& handlers,
    DisplayConfigurationChangeHandler const& conf_change_handler)
{
    auto monitor = create_drm_monitor();
    auto const fd = monitor->fd();
    handlers.register_fd_handler(
        { fd },
        this,
        make_module_ptr<std::function<void(int)>>(
            [conf_change_handler, this, monitor = std::shared_ptr<mir::udev::Monitor>{std::move(monitor)}](int)
            {
                monitor->process_events(
                    [conf_change_handler, this](mir::udev::Monitor::EventType, mir::udev::Device const&)
                    {
                        std::lock_guard lock{configuration_mutex};
                        display_configuration.update();
                        conf_change_handler();
                    });
            }));
}

void mge::Display::pause()
{

}

void mge::Display::resume()
{

}

std::shared_ptr<mg::Cursor> mge::Display::create_hardware_cursor()
{
    // TODO: Find the cursor plane, and use it.
    return nullptr;
}

bool mge::Display::apply_if_configuration_preserves_display_buffers(
    mg::DisplayConfiguration const& /*conf*/)
{
    return false;
}

std::shared_ptr<mg::InitialRender> mge::Display::create_initial_render()
{
    return nullptr;
}

