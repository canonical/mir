/*
 * Copyright © 2013, 2015 Canonical Ltd.
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
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#include "display.h"
#include "nested_display_configuration.h"
#include "display_buffer.h"
#include "host_connection.h"
#include "host_stream.h"

#include "mir/geometry/rectangle.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/graphics/surfaceless_egl_context.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/overlapping_output_grouping.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/virtual_output.h"
#include "mir/graphics/buffer_properties.h"
#include "mir_toolkit/mir_connection.h"
#include "mir/raii.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <algorithm>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

EGLint const mgn::detail::nested_egl_context_attribs[] =
{
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
    EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
    EGL_NONE
};

mgn::detail::EGLSurfaceHandle::EGLSurfaceHandle(EGLDisplay display, EGLNativeWindowType native_window, EGLConfig cfg)
    : egl_display(display),
      egl_surface(eglCreateWindowSurface(egl_display, cfg, native_window, NULL))
{
    if (egl_surface == EGL_NO_SURFACE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Nested Mir Display Error: Failed to create EGL surface."));
    }
}

mgn::detail::EGLSurfaceHandle::~EGLSurfaceHandle() noexcept
{
    eglDestroySurface(egl_display, egl_surface);
}

mgn::detail::EGLDisplayHandle::EGLDisplayHandle(
    EGLNativeDisplayType native_display,
    std::shared_ptr<GLConfig> const& gl_config)
    : egl_display(EGL_NO_DISPLAY),
      egl_context_(EGL_NO_CONTEXT),
      gl_config{gl_config},
      pixel_format{mir_pixel_format_invalid}
{
    egl_display = eglGetDisplay(native_display);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(mg::egl_error("Nested Mir Display Error: Failed to fetch EGL display."));
}

void mgn::detail::EGLDisplayHandle::initialize(MirPixelFormat format)
{
    int major;
    int minor;

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Nested Mir Display Error: Failed to initialize EGL."));
    }

    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
    pixel_format = format;
    egl_context_ = eglCreateContext(egl_display, choose_windowed_config(format), EGL_NO_CONTEXT, detail::nested_egl_context_attribs);

    if (egl_context_ == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create shared EGL context"));
}

EGLConfig mgn::detail::EGLDisplayHandle::choose_windowed_config(MirPixelFormat format) const
{
    EGLint const nested_egl_config_attribs[] =
    {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, mg::red_channel_depth(format),
        EGL_GREEN_SIZE, mg::green_channel_depth(format),
        EGL_BLUE_SIZE, mg::blue_channel_depth(format),
        EGL_ALPHA_SIZE, mg::alpha_channel_depth(format),
        EGL_DEPTH_SIZE, gl_config->depth_buffer_bits(),
        EGL_STENCIL_SIZE, gl_config->stencil_buffer_bits(),
        EGL_RENDERABLE_TYPE, MIR_SERVER_EGL_OPENGL_BIT,
        EGL_NONE
    };
    EGLConfig result;
    int n;

    int res = eglChooseConfig(egl_display, nested_egl_config_attribs, &result, 1, &n);
    if ((res != EGL_TRUE) || (n != 1))
        BOOST_THROW_EXCEPTION(mg::egl_error("Nested Mir Display Error: Failed to choose EGL configuration."));

    return result;
}

EGLContext mgn::detail::EGLDisplayHandle::egl_context() const
{
    return egl_context_;
}

std::unique_ptr<mir::renderer::gl::Context> mgn::detail::EGLDisplayHandle::create_gl_context() const
{
    EGLint const attribs[] =
    {
       EGL_SURFACE_TYPE, EGL_DONT_CARE,
       EGL_RED_SIZE, mg::red_channel_depth(pixel_format),
       EGL_GREEN_SIZE, mg::green_channel_depth(pixel_format),
       EGL_BLUE_SIZE, mg::blue_channel_depth(pixel_format),
       EGL_ALPHA_SIZE, mg::alpha_channel_depth(pixel_format),
       EGL_DEPTH_SIZE, gl_config->depth_buffer_bits(),
       EGL_STENCIL_SIZE, gl_config->stencil_buffer_bits(),
       EGL_RENDERABLE_TYPE, MIR_SERVER_EGL_OPENGL_BIT,
       EGL_NONE
    };
    return std::make_unique<SurfacelessEGLContext>(egl_display, attribs, EGL_NO_CONTEXT);
}

mgn::detail::EGLDisplayHandle::~EGLDisplayHandle() noexcept
{
    eglTerminate(egl_display);
}

mgn::detail::DisplaySyncGroup::DisplaySyncGroup(
    std::shared_ptr<mgn::detail::DisplayBuffer> const& output) :
    output(output)
{
}

void mgn::detail::DisplaySyncGroup::for_each_display_buffer(
    std::function<void(graphics::DisplayBuffer&)> const& f)
{
    f(*output);
}

void mgn::detail::DisplaySyncGroup::post()
{
}

std::chrono::milliseconds
mgn::detail::DisplaySyncGroup::recommended_sleep() const
{
    // TODO: Might make sense in future with nested bypass. We could save
    //       almost another frame of lag!
    return std::chrono::milliseconds::zero();
}

geom::Rectangle mgn::detail::DisplaySyncGroup::view_area() const
{
    return output->view_area();
}

namespace
{
long area_of(geom::Rectangle const& rect)
{
    return static_cast<long>(rect.size.width.as_int())*rect.size.height.as_int();
}

std::vector<mg::DisplayConfigurationOutput> calculate_best_outputs(
    mg::DisplayConfiguration const& conf)
{
    std::vector<mg::DisplayConfigurationOutput> result;

    mg::OverlappingOutputGrouping unique_outputs{conf};

    unique_outputs.for_each_group(
        [&](mg::OverlappingOutputGroup const& group)
        {
            geom::Rectangle const area = group.bounding_rectangle();

            long max_overlap_area = 0;
            mg::DisplayConfigurationOutput best_output;

            group.for_each_output(
                [&](mg::DisplayConfigurationOutput const& output)
                {
                    if (area_of(area.intersection_with(output.extents())) > max_overlap_area)
                    {
                        max_overlap_area = area_of(area.intersection_with(output.extents()));
                        best_output = output;
                    }
                });
            result.push_back(best_output);
        });

    return result;
}
}

mgn::Display::Display(
    std::shared_ptr<mg::RenderingPlatform> const& platform,
    std::shared_ptr<HostConnection> const& connection,
    std::shared_ptr<mg::DisplayReport> const& display_report,
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<mg::GLConfig> const& gl_config,
    PassthroughOption passthrough_option) :
    platform{platform},
    connection{connection},
    display_report{display_report},
    egl_display{connection->egl_native_display(), gl_config},
    passthrough_option(passthrough_option),
    outputs{},
    current_configuration(std::make_unique<NestedDisplayConfiguration>(connection->create_display_config()))
{
    decltype(current_configuration) conf{dynamic_cast<NestedDisplayConfiguration*>(current_configuration->clone().release())};

    initial_conf_policy->apply_to(*conf);

    if (*current_configuration != *conf)
    {
        connection->apply_display_config(**conf);
        swap(current_configuration, conf);
    }

    create_surfaces(calculate_best_outputs(*current_configuration));
}

mgn::Display::~Display() noexcept
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
    if (eglGetCurrentContext() == egl_display.egl_context())
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void mgn::Display::for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f)
{
    std::unique_lock<std::mutex> lock(outputs_mutex);
    for (auto& i : outputs)
        f(*i.second);
}

std::unique_ptr<mg::DisplayConfiguration> mgn::Display::configuration() const
{
    std::lock_guard<std::mutex> lock(configuration_mutex);
    return current_configuration->clone();
}

void mgn::Display::complete_display_initialization(MirPixelFormat format)
{
    if (egl_display.egl_context() != EGL_NO_CONTEXT)  return;

    egl_display.initialize(format);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_display.egl_context());
}

void mgn::Display::configure(mg::DisplayConfiguration const& configuration)
{
    if (!configuration.valid())
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid or inconsistent display configuration"));
    }

    decltype(current_configuration) new_config{dynamic_cast<NestedDisplayConfiguration*>(configuration.clone().release())};

    {
        std::lock_guard<std::mutex> lock(configuration_mutex);

        swap(current_configuration, new_config);
        create_surfaces(calculate_best_outputs(*current_configuration));
    }

    connection->apply_display_config(**current_configuration);
}

void mgn::Display::create_surfaces(std::vector<mg::DisplayConfigurationOutput> const& output_list)
{
    decltype(outputs) result;
    for (auto const& output : output_list)
    {
        auto const& egl_config_format = output.current_format;
        geometry::Rectangle const& extents = output.extents();

        auto& display_buffer = result[output.id];

        {
            std::unique_lock<std::mutex> lock(outputs_mutex);
            display_buffer = outputs[output.id];
        }

        if (display_buffer)
        {
            if (display_buffer->view_area() != extents)
                display_buffer.reset();
        }

        if (!display_buffer)
        {
            complete_display_initialization(egl_config_format);

            eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
            display_buffer = std::make_shared<mgn::detail::DisplaySyncGroup>(
                std::make_shared<mgn::detail::DisplayBuffer>(
                    egl_display,
                    output,
                    connection,
                    passthrough_option));
        }
    }

    {
        std::unique_lock<std::mutex> lock(outputs_mutex);
        outputs.swap(result);
    }
}

void mgn::Display::register_configuration_change_handler(
        EventHandlerRegister& /*handlers*/,
        DisplayConfigurationChangeHandler const& conf_change_handler)
{
    auto const handler = [this, conf_change_handler] {
        {
            std::lock_guard<std::mutex> lock(configuration_mutex);
            current_configuration = std::make_unique<NestedDisplayConfiguration>(connection->create_display_config());
        }
        conf_change_handler();
    };

    connection->set_display_config_change_callback(handler);
}

void mgn::Display::register_pause_resume_handlers(
        EventHandlerRegister& /*handlers*/,
        DisplayPauseHandler const& /*pause_handler*/,
        DisplayResumeHandler const& /*resume_handler*/)
{
    // No need to do anything
}

void mgn::Display::pause()
{
    // No need to do anything
}

void mgn::Display::resume()
{
    // No need to do anything
}

auto mgn::Display::create_hardware_cursor() -> std::shared_ptr<mg::Cursor>
{
    BOOST_THROW_EXCEPTION(std::logic_error("Initialization loop: we already need the Cursor when creating the Display"));
    // So we can't do this: return std::make_shared<Cursor>(connection);
}

std::unique_ptr<mg::VirtualOutput> mgn::Display::create_virtual_output(int /*width*/, int /*height*/)
{
    return nullptr;
}

mg::NativeDisplay* mgn::Display::native_display()
{
    return this;
}

std::unique_ptr<mir::renderer::gl::Context> mgn::Display::create_gl_context() const
{
    return egl_display.create_gl_context();
}

bool mgn::Display::apply_if_configuration_preserves_display_buffers(
    mg::DisplayConfiguration const& conf)
{
    auto new_outputs = calculate_best_outputs(conf);
    {
        std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

        {
            std::lock_guard<decltype(outputs_mutex)> outputs_lock{outputs_mutex};
            for (auto const existing_output : outputs)
            {
                // O(n²) here, but n < 10 and this is isn't a hot path.
                if (!std::any_of(
                    new_outputs.begin(),
                    new_outputs.end(),
                [&existing_output](auto const& output) { return output.id == existing_output.first; }))
                {
                    // At least one of the existing outputs isn't used in the ne
                    return false;
                }
            }

            for (auto const& output : new_outputs)
            {
                auto existing_output = outputs.find(output.id);

                /*
                 * If there's no existing output associated with this ID then we can't be
                 * invalidating its DisplayBuffer.
                 *
                 * If there *is* an existing output associated with this ID then
                 * create_surfaces() will invalidate its DisplayBuffer if and only if
                 * the viewport doesn't exactly match.
                 */
                if (existing_output != outputs.end() &&
                    existing_output->second->view_area() != output.extents())
                {
                    return false;
                }
            }
        }

        current_configuration =
            decltype(current_configuration){dynamic_cast<NestedDisplayConfiguration*>(conf.clone().release())};

        create_surfaces(new_outputs);
    }
    connection->apply_display_config(**current_configuration);

    return true;
}

mg::Frame mgn::Display::last_frame_on(unsigned) const
{
    return {}; // TODO after the client API exists for us to get it
}
