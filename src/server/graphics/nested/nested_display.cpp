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

#include "nested_display.h"
#include "nested_display_configuration.h"
#include "nested_output.h"
#include "mir_api_wrappers.h"

#include "mir/geometry/rectangle.h"
#include "mir/graphics/pixel_format_utils.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/surfaceless_egl_context.h"
#include "mir/graphics/display_configuration_policy.h"
#include "host_connection.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mgnw = mir::graphics::nested::mir_api_wrappers;
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
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to create EGL surface."));
    }
}

mgn::detail::EGLSurfaceHandle::~EGLSurfaceHandle() noexcept
{
    eglDestroySurface(egl_display, egl_surface);
}

mgn::detail::EGLDisplayHandle::EGLDisplayHandle(MirConnection* connection)
    : egl_display(EGL_NO_DISPLAY),
    egl_context_(EGL_NO_CONTEXT)
{
    auto const native_display = (EGLNativeDisplayType) mir_connection_get_egl_native_display(connection);
    egl_display = eglGetDisplay(native_display);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to fetch EGL display."));
}

void mgn::detail::EGLDisplayHandle::initialize(MirPixelFormat format)
{
    int major;
    int minor;

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to initialize EGL."));
    }

    egl_context_ = eglCreateContext(egl_display, choose_windowed_es_config(format), EGL_NO_CONTEXT, detail::nested_egl_context_attribs);

    if (egl_context_ == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(std::runtime_error("Failed to create shared EGL context"));
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
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLConfig result;
    int n;

    int res = eglChooseConfig(egl_display, nested_egl_config_attribs, &result, 1, &n);
    if ((res != EGL_TRUE) || (n != 1))
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to choose EGL configuration."));

    return result;
}

EGLNativeWindowType mgn::detail::EGLDisplayHandle::native_window(EGLConfig /*egl_config*/, MirSurface* mir_surface) const
{
    auto const native_window = static_cast<EGLNativeWindowType>(mir_surface_get_egl_native_window(mir_surface));
    if (!native_window)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to fetch EGL native window."));

    return native_window;
}

EGLContext mgn::detail::EGLDisplayHandle::egl_context() const
{
    return egl_context_;
}

mgn::detail::EGLDisplayHandle::~EGLDisplayHandle() noexcept
{
    eglTerminate(egl_display);
}

mgn::NestedDisplay::NestedDisplay(
    std::shared_ptr<HostConnection> const& connection,
    std::shared_ptr<input::EventFilter> const& event_handler,
    std::shared_ptr<mg::DisplayReport> const& display_report,
    std::shared_ptr<mg::DisplayConfigurationPolicy> const& initial_conf_policy) :
    connection{connection},
    event_handler{event_handler},
    display_report{display_report},
    egl_display{*connection},
    outputs{}
{

    std::shared_ptr<DisplayConfiguration> conf(configuration());
    initial_conf_policy->apply_to(*conf);
    configure(*conf);
}

mgn::NestedDisplay::~NestedDisplay() noexcept
{
}

void mgn::NestedDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    std::unique_lock<std::mutex> lock(outputs_mutex);
    for (auto& i : outputs)
        f(*i.second);
}

std::shared_ptr<mg::DisplayConfiguration> mgn::NestedDisplay::configuration()
{
    return std::make_shared<NestedDisplayConfiguration>(mir_connection_create_display_config(*connection));
}

void mgn::NestedDisplay::complete_display_initialization(MirPixelFormat format)
{
    if (egl_display.egl_context() != EGL_NO_CONTEXT)  return;

    egl_display.initialize(format);
    eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_display.egl_context());
}

void mgn::NestedDisplay::configure(mg::DisplayConfiguration const& configuration)
{
    decltype(outputs) result;

    // TODO for proper mirrored mode support we will need to detect overlapping outputs and
    // TODO only use a single surface for them. The OverlappingOutputGrouping utility class
    // TODO used by the Mesa backend for a similar purpose could help with this.
    configuration.for_each_output(
        [&](mg::DisplayConfigurationOutput const& output)
        {
            if (output.used)
            {
                geometry::Rectangle const area{output.top_left, output.modes[output.current_mode_index].size};

                auto const& egl_display_mode = output.modes[output.current_mode_index];
                auto const& egl_config_format = output.current_format;

                complete_display_initialization(egl_config_format);

                MirSurfaceParameters const request_params =
                    {
                        "Mir nested display",
                        egl_display_mode.size.width.as_int(),
                        egl_display_mode.size.height.as_int(),
                        egl_config_format,
                        mir_buffer_usage_hardware,
                        static_cast<uint32_t>(output.id.as_value())
                    };

                auto const mir_surface = mir_connection_create_surface_sync(*connection, &request_params);

                if (!mir_surface_is_valid(mir_surface))
                    BOOST_THROW_EXCEPTION(std::runtime_error(mir_surface_get_error_message(mir_surface)));

                result[output.id] = std::make_shared<mgn::detail::NestedOutput>(
                    egl_display,
                    mir_surface,
                    area,
                    event_handler,
                    output.current_format);
            }
        });

    if (result.empty())
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir needs at least one output for display"));

    auto const& conf = dynamic_cast<NestedDisplayConfiguration const&>(configuration);

    {
        std::unique_lock<std::mutex> lock(outputs_mutex);
        outputs.swap(result);
    }

    mir_connection_apply_display_config(*connection, conf);
}

namespace
{
void display_config_callback_thunk(MirConnection* /*connection*/, void* context)
{
    (*static_cast<mg::DisplayConfigurationChangeHandler*>(context))();
}
}

void mgn::NestedDisplay::register_configuration_change_handler(
        EventHandlerRegister& /*handlers*/,
        DisplayConfigurationChangeHandler const& conf_change_handler)
{
    mir_connection_set_display_config_change_callback(
        *connection,
        &display_config_callback_thunk,
        &(my_conf_change_handler = conf_change_handler));
}

void mgn::NestedDisplay::register_pause_resume_handlers(
        EventHandlerRegister& /*handlers*/,
        DisplayPauseHandler const& /*pause_handler*/,
        DisplayResumeHandler const& /*resume_handler*/)
{
    // No need to do anything
}

void mgn::NestedDisplay::pause()
{
    // TODO Do we "own" the cursor or does the host mir?
    // If we "own" the cursor then we need to hide it
}

void mgn::NestedDisplay::resume()
{
    // TODO Do we "own" the cursor or does the host mir?
    // TODO If we "own" the cursor then we need to restore it
}

auto mgn::NestedDisplay::the_cursor()->std::weak_ptr<Cursor>
{
    // TODO Do we "own" the cursor or does the host mir?
    return std::weak_ptr<Cursor>();
}

std::unique_ptr<mg::GLContext> mgn::NestedDisplay::create_gl_context()
{
    return std::unique_ptr<mg::GLContext>{new SurfacelessEGLContext(egl_display, EGL_NO_CONTEXT)};
}
