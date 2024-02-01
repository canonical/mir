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

#include <chrono>
#include <epoxy/egl.h>
#include <thread>

#include "display.h"
#include "egl_output.h"
#include "kms_framebuffer.h"
#include "kms_cpu_addressable_display_provider.h"

#include "kms-utils/drm_mode_resources.h"
#include "mir/graphics/platform.h"
#include "threaded_drm_event_handler.h"

#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/overlapping_output_grouping.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/display_sink.h"
#include "mir/graphics/transformation.h"
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
namespace geom = mir::geometry;

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

class DisplaySink
    : public mg::DisplaySyncGroup,
      public mg::DisplaySink,
      public mg::EGLStreamDisplayAllocator
{
public:
    DisplaySink(
        mir::Fd drm_node,
        EGLDisplay dpy,
        EGLContext ctx,
        EGLConfig config,
        std::shared_ptr<mge::DRMEventHandler> event_handler,
        std::shared_ptr<mge::kms::EGLOutput> output,
        std::shared_ptr<mg::DisplayReport> display_report)
        : dpy{dpy},
          ctx{create_context(dpy, config, ctx)},
          output{output},
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
        if (eglQueryOutputLayerAttribEXT(dpy, output->output_layer(), EGL_SWAP_INTERVAL_EXT, &swap_interval) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query swap interval"));
        }

        if (eglOutputLayerAttribEXT(dpy, output->output_layer(), EGL_SWAP_INTERVAL_EXT, 1) != EGL_TRUE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to set swap interval"));
        }

        if (eglStreamConsumerOutputEXT(dpy, output_stream, output->output_layer()) == EGL_FALSE)
        {
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to attach EGLStream to output"));
        };

        this->display_report->report_successful_setup_of_native_resources();
        this->display_report->report_successful_display_construction();
        this->display_report->report_egl_configuration(dpy, config);

        // The first flip needs no wait!
        std::promise<void> satisfied_promise;
        satisfied_promise.set_value();
        pending_flip = satisfied_promise.get_future();
    }

    ~DisplaySink()
    {
        if (output_stream != EGL_NO_STREAM_KHR)
        {
            eglDestroyStreamKHR(dpy, output_stream);
            output_stream = nullptr;
        }

        if (ctx != EGL_NO_CONTEXT)
        {
            eglDestroyContext(dpy, ctx);
            ctx = nullptr;
        }
    }

    mir::geometry::Rectangle view_area() const override
    {
        return output->extents();
    }

    auto output_size() const -> geom::Size override
    {
        return output->size();
    }

    glm::mat2 transformation() const override
    {
        return output->transformation();
    }

    void for_each_display_sink(const std::function<void(mir::graphics::DisplaySink&)>& f) override
    {
        f(*this);
    }

    void post() override
    {
        // Wait for the last flip to finish, if it hasn't already.
        pending_flip.get();
        pending_flip = event_handler->expect_flip_event(
            output->crtc_id(),
            [this](unsigned frame_count, std::chrono::milliseconds frame_time)
            {
                // TODO: Um, why does NVIDIA always call this with 0, 0ms?
                display_report->report_vsync(
                    output->crtc_id(),
                    mg::Frame {
                        frame_count,
                        mir::time::PosixTimestamp(CLOCK_MONOTONIC, frame_time)});
            });

        if (next_swap) // This is the KMS route
        {
            visible_fb = std::move(scheduled_fb);
            scheduled_fb = nullptr;

            scheduled_fb = std::move(next_swap);
            next_swap = nullptr;

            // Arbitrarily pick a 10s deadline for modesetting
            auto deadline  = std::chrono::steady_clock::now() + std::chrono::seconds{10};
            while (auto error = output->queue_atomic_flip(*scheduled_fb, event_handler->drm_event_data()))
            {
                if (error->value() == EPERM || error->value() == EACCES)
                {
                    /* We don't have modesetting permissions
                     * This is presumably because we've just been VT-switched-away, and so
                     * will pause soon.
                     *
                     * We'll assume that this is transitory, log in case it's an acutal issue, and continue.
                     *
                     * We've failed to submit the flip, though, so cancel the pending flip event
                     */
                    mir::log_info("Failed to submit page flip (%s (%i))",
                        error->message().c_str(),
                        error->value());
                    event_handler->cancel_flip_events(output->crtc_id());
                    return;
                }
                if (error->value() == EAGAIN)
                {
                    /* Sigh.
                     * So, if we try and schedule a flip when something is already in progress, we'll
                     * get EAGAIN.
                     *
                     * *Mostly* we shouldn't, because we're waiting for the page flip event before trying
                     * to submit a new one, but it's possible for the KMS driver to cause this for opaque
                     * (to us) reasons even if we're doing everything correctly.
                     *
                     * So, in this case, wait a bit(!) and try again.
                     */
                    if (std::chrono::steady_clock::now() > deadline)
                    {
                        BOOST_THROW_EXCEPTION((std::runtime_error{"Timeout waiting for modeset"}));
                    }
                    /* Sleep for 5ms; this would correspond to ~200Hz, which is fast enough that we
                     * won't notice it, while being long enough to make it likely the previous work is
                     * done
                     */
                    std::this_thread::sleep_for(std::chrono::milliseconds{5});
                }
                if (error->value() == EINVAL)
                {
                    /* This *should* be just for us doing the wrong thing, but it *also*
                     * occurs when VT switching has disabled the damn output behind our back
                     * so we can't do a non-ALLOW_MODESET commit (and we don't want to do that)
                     * anyway.
                     *
                     * Assume this is a transient VT-switch failure :(
                     */
                    mir::log_info("Failed to submit page flip (%s (%i))",
                        error->message().c_str(),
                        error->value());
                    event_handler->cancel_flip_events(output->crtc_id());
                    return;
                }
            }
            return;
        }

        EGLAttrib const acquire_attribs[] = {
            EGL_DRM_FLIP_EVENT_DATA_NV, reinterpret_cast<EGLAttrib>(event_handler->drm_event_data()),
            EGL_NONE
        };
        if (nv_stream(dpy).eglStreamConsumerAcquireAttribNV(dpy, output_stream, acquire_attribs) != EGL_TRUE)
        {
            auto error = eglGetError();
            /* TODO: Handle error 0x3353 (EGL_RESOURCE_BUSY_EXT) here.
             * That can be triggered by VT switch, but the naive approach direct KMS uses above doesn't work:
             * We can't return to the compositor without *really* consuming the buffer because then
             * eglSwapBuffers in EGLStreamOutputSurface::commit() blocks (because it can't push a new frame
             * into the EGLStream).
             */
            EGLAttrib stream_state{0};
            eglQueryStreamAttribKHR(dpy, output_stream, EGL_STREAM_STATE_KHR, &stream_state);
            std::string state;
            switch (stream_state)
            {
            case EGL_STREAM_STATE_CREATED_KHR:
                state = "EGL_STREAM_STATE_CREATED_KHR";
                break;
            case EGL_STREAM_STATE_CONNECTING_KHR:
                state = "EGL_STREAM_STATE_CONNECTING_KHR";
                break;
            case EGL_STREAM_STATE_EMPTY_KHR:
                state = "EGL_STREAM_STATE_EMPTY_KHR";
                break;
            case EGL_STREAM_STATE_NEW_FRAME_AVAILABLE_KHR:
                state = "EGL_STREAM_STATE_NEW_FRAME_AVAILABLE_KHR";
                break;
            case EGL_STREAM_STATE_OLD_FRAME_AVAILABLE_KHR:
                state = "EGL_STREAM_STATE_OLD_FRAME_AVAILABLE_KHR";
                break;
            case EGL_STREAM_STATE_DISCONNECTED_KHR:
                state = "EGL_STREAM_STATE_DISCONNECTED_KHR";
                break;
            default:
                state = "<ERROR ACQUIRING STATE>";
            }
            BOOST_THROW_EXCEPTION((
                std::system_error{
                    error,
                    mg::egl_category(),
                    std::string{"Failed to submit frame from EGLStream for display. (Stream in state: "} + state + ")"}));
        }
    }

    std::chrono::milliseconds recommended_sleep() const override
    {
        return std::chrono::milliseconds{0};
    }

    auto claim_stream() -> EGLStreamKHR override
    {
        return output_stream;
    }

    auto describe_output() const -> std::string override
    {
        return "FIXME: actually implement";
    }

    auto maybe_create_allocator(mg::DisplayAllocator::Tag const& type_tag) -> mg::DisplayAllocator* override
    {
        if (dynamic_cast<mg::EGLStreamDisplayAllocator::Tag const*>(&type_tag))
        {
            return this;
        }
        if (dynamic_cast<mg::CPUAddressableDisplayAllocator::Tag const*>(&type_tag))
        {
            kms_allocator = mg::kms::CPUAddressableDisplayAllocator::create_if_supported(drm_node, output->size());
            return kms_allocator.get();
        }
        return nullptr;
    }

    void set_next_image(std::unique_ptr<mg::Framebuffer> content) override
    {
        std::vector<mg::DisplayElement> const single_buffer = {
            mg::DisplayElement {
                view_area(),
                mir::geometry::RectangleF{
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

    bool overlay(std::vector<mg::DisplayElement> const& renderable_list) override
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

        if (auto fb = std::dynamic_pointer_cast<mg::FBHandle>(renderable_list[0].buffer))
        {
            next_swap = std::move(fb);
            return true;
        }
        // TODO: Validate that the submitted "framebuffer" is *actually* our EGLStream
        return true;
    }

    void resume()
    {
        output->set_flags_for_next_flip(DRM_MODE_ATOMIC_ALLOW_MODESET);
    }
private:

    EGLDisplay dpy;
    EGLContext ctx;
    std::shared_ptr<mge::kms::EGLOutput> output;
    EGLStreamKHR output_stream;
    mir::Fd const drm_node;
    std::shared_ptr<mge::DRMEventHandler> const event_handler;
    std::future<void> pending_flip;
    mg::EGLExtensions::LazyDisplayExtensions<mg::EGLExtensions::NVStreamAttribExtensions> nv_stream;
    std::shared_ptr<mg::DisplayReport> const display_report;
    std::shared_ptr<mg::kms::CPUAddressableDisplayAllocator> kms_allocator;

    /// Used only for the KMS case
    std::shared_ptr<mg::FBHandle const> next_swap{nullptr};
    std::shared_ptr<mg::FBHandle const> scheduled_fb{nullptr};
    std::shared_ptr<mg::FBHandle const> visible_fb{nullptr};
};

mge::KMSDisplayConfiguration create_display_configuration(
    mir::Fd const& drm_node,
    EGLDisplay dpy,
    EGLContext context)
{
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, context) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to make EGL context current for display construction")));
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
    kms_conf.for_each_output([this](std::shared_ptr<kms::EGLOutput> const& output)
         {
             if (output->used)
             {
                 output->configure(output->current_mode_index);
                 active_sync_groups.emplace_back(
                     std::make_unique<::DisplaySink>(
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
    for (auto& group : active_sync_groups)
    {
        dynamic_cast<::DisplaySink*>(group.get())->resume();
    }
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

