/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#include "display.h"
#include "nested_display_configuration.h"
#include "display_buffer.h"
#include "host_connection.h"

#include "mir/geometry/rectangle.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/surfaceless_egl_context.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/overlapping_output_grouping.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

EGLint const mgn::detail::nested_egl_context_attribs[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
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
      gl_config{gl_config}
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

    egl_context_ = eglCreateContext(egl_display, choose_windowed_es_config(format), EGL_NO_CONTEXT, detail::nested_egl_context_attribs);

    if (egl_context_ == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create shared EGL context"));
}

EGLConfig mgn::detail::EGLDisplayHandle::choose_windowed_es_config(MirPixelFormat format) const
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
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
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

mgn::Display::Display(
    std::shared_ptr<mg::Platform> const& platform,
    std::shared_ptr<HostConnection> const& connection,
    std::shared_ptr<input::InputDispatcher> const& dispatcher,
    std::shared_ptr<mg::DisplayReport> const& display_report,
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<mg::GLConfig> const& gl_config,
    std::shared_ptr<mi::CursorListener> const& cursor_listener) :
    platform{platform},
    connection{connection},
    dispatcher{dispatcher},
    display_report{display_report},
    egl_display{connection->egl_native_display(), gl_config},
    cursor_listener{cursor_listener},
    outputs{}
{
    std::shared_ptr<DisplayConfiguration> conf(configuration());
    initial_conf_policy->apply_to(*conf);
    create_surfaces(*conf);
}

mgn::Display::~Display() noexcept
{
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
    return std::make_unique<NestedDisplayConfiguration>(
        connection->create_display_config());
}

void mgn::Display::complete_display_initialization(MirPixelFormat format)
{
    if (egl_display.egl_context() != EGL_NO_CONTEXT)  return;

    egl_display.initialize(format);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_display.egl_context());
}

void mgn::Display::configure(mg::DisplayConfiguration const& configuration)
{
    create_surfaces(configuration);
    apply_to_connection(configuration);
}

void mgn::Display::create_surfaces(mg::DisplayConfiguration const& configuration)
{
    if (!configuration.valid())
    {
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid or inconsistent display configuration"));
    }

    decltype(outputs) result;
    OverlappingOutputGrouping unique_outputs{configuration};

    unique_outputs.for_each_group(
        [&](mg::OverlappingOutputGroup const& group)
        {
            bool have_output_for_group = false;
            geometry::Rectangle const& area = group.bounding_rectangle();
            group.for_each_output([&](mg::DisplayConfigurationOutput output)
                {
                    if (!have_output_for_group)
                    {
                        auto const& egl_config_format = output.current_format;

                        complete_display_initialization(egl_config_format);

                        auto const host_surface = connection->create_surface(
                            area.size.width.as_int(),
                            area.size.height.as_int(),
                            egl_config_format,
                            "Mir nested display",
                            mir_buffer_usage_hardware,
                            static_cast<uint32_t>(output.id.as_value()));

                        result[output.id] = std::make_shared<mgn::detail::DisplaySyncGroup>( 
                            std::make_shared<mgn::detail::DisplayBuffer>(
                                egl_display,
                                host_surface,
                                area,
                                dispatcher,
                                cursor_listener,
                                output.current_format));
                        have_output_for_group = true;
                    }
                });
        });

    {
        std::unique_lock<std::mutex> lock(outputs_mutex);
        outputs.swap(result);
    }
}

void mgn::Display::apply_to_connection(mg::DisplayConfiguration const& configuration)
{
    auto const& conf = dynamic_cast<NestedDisplayConfiguration const&>(configuration);

    connection->apply_display_config(*conf);
}

void mgn::Display::register_configuration_change_handler(
        EventHandlerRegister& /*handlers*/,
        DisplayConfigurationChangeHandler const& conf_change_handler)
{
    connection->set_display_config_change_callback(conf_change_handler);
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
    // TODO Do we "own" the cursor or does the host mir?
    // If we "own" the cursor then we need to hide it
}

void mgn::Display::resume()
{
    // TODO Do we "own" the cursor or does the host mir?
    // TODO If we "own" the cursor then we need to restore it
}

auto mgn::Display::create_hardware_cursor(std::shared_ptr<mg::CursorImage> const& /* initial image */)->std::shared_ptr<Cursor>
{
    // TODO Do we "own" the cursor or does the host mir?
    return std::shared_ptr<Cursor>();
}

std::unique_ptr<mg::GLContext> mgn::Display::create_gl_context()
{
    return std::make_unique<SurfacelessEGLContext>(egl_display, EGL_NO_CONTEXT);
}
