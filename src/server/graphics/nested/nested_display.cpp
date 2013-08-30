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
#include "mir/graphics/gl_context.h"
#include "mir/graphics/nested/host_connection.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace mgnw = mir::graphics::nested::mir_api_wrappers;
namespace geom = mir::geometry;

mgn::detail::EGLDisplayHandle::EGLDisplayHandle(MirConnection* connection)
{
    auto const native_display = (EGLNativeDisplayType) mir_connection_get_egl_native_display(connection);
    if (!native_display)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to fetch EGL native display."));

    egl_display = eglGetDisplay(native_display);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to fetch EGL display."));
}

void mgn::detail::EGLDisplayHandle::initialize() const
{
    int major;
    int minor;

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to initialize EGL."));
    }
}

EGLConfig mgn::detail::EGLDisplayHandle::choose_config(const EGLint attrib_list[]) const
{
    EGLConfig result;
    int n;

    int res = eglChooseConfig(egl_display, attrib_list, &result, 1, &n);
    if ((res != EGL_TRUE) || (n != 1))
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to choose EGL configuration."));

    return result;
}

EGLSurface mgn::detail::EGLDisplayHandle::egl_surface(EGLConfig egl_config, MirSurface* mir_surface) const
{
    auto const native_window = static_cast<EGLNativeWindowType>(mir_surface_get_egl_native_window(mir_surface));
    if (!native_window)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to fetch EGL native window."));

    EGLSurface egl_surface = eglCreateWindowSurface(egl_display, egl_config, native_window, NULL);
    if (egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to create EGL surface."));

    return egl_surface;
}

mgn::detail::EGLDisplayHandle::~EGLDisplayHandle() noexcept
{
    eglTerminate(egl_display);
}

mgn::NestedDisplay::NestedDisplay(std::shared_ptr<HostConnection> const& connection, std::shared_ptr<mg::DisplayReport> const& display_report) :
    connection{connection},
    display_report{display_report},
    egl_display{*connection},
    outputs{}
{
    egl_display.initialize();
    configure(*configuration());
}

mgn::NestedDisplay::~NestedDisplay() noexcept
{
}

void mgn::NestedDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    for (auto& i : outputs)
        f(*i.second);
}

std::shared_ptr<mg::DisplayConfiguration> mgn::NestedDisplay::configuration()
{
    return std::make_shared<NestedDisplayConfiguration>(mir_connection_create_display_config(*connection));
}

void mgn::NestedDisplay::configure(mg::DisplayConfiguration const& configuration)
{
    decltype(outputs) result;

    // TODO for proper mirrored mode support we will need to detect overlapping outputs and
    // TODO only use a single surface for them. The OverlappingOutputGrouping utility class
    // TODO used by the GBM backend for a similar purpose could help with this.
    configuration.for_each_output(
        [&](mg::DisplayConfigurationOutput const& output)
        {
            if (output.used)
            {
                geometry::Rectangle const area{output.top_left, output.modes[output.current_mode_index].size};

                auto const& egl_display_mode = output.modes[output.current_mode_index];
                auto const egl_display_format = output.pixel_formats[output.current_format_index];

                MirSurfaceParameters const request_params =
                    {
                        "Mir nested display",
                        egl_display_mode.size.width.as_int(),
                        egl_display_mode.size.height.as_int(),
                        MirPixelFormat(egl_display_format),
                        mir_buffer_usage_hardware,
                        static_cast<uint32_t>(output.id.as_value())
                    };

                auto const mir_surface = mir_connection_create_surface_sync(*connection, &request_params);

                if (!mir_surface_is_valid(mir_surface))
                    BOOST_THROW_EXCEPTION(std::runtime_error(mir_surface_get_error_message(mir_surface)));

                result[output.id] = std::make_shared<mgn::detail::NestedOutput>(egl_display, mir_surface, area);
            }
        });

    if (result.empty())
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir needs at least one output for display"));

    auto const& conf = dynamic_cast<NestedDisplayConfiguration const&>(configuration);

    outputs.swap(result);
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
    class NestedGLContext : public mg::GLContext
    {
    public:
        NestedGLContext(detail::EGLDisplayHandle const& egl_display) :
            egl_display{egl_display},
            egl_config{egl_display.choose_config(detail::egl_attribs)},
            egl_context{egl_display, eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, detail::egl_context_attribs)}
        {
        }

        void make_current() override
        {
            eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, egl_context);
        }

        void release_current() override
        {
            eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }

    private:
        EGLDisplay const egl_display;
        EGLConfig const egl_config;
        EGLContextStore const egl_context;
    };

    return std::unique_ptr<mg::GLContext>{new NestedGLContext(egl_display)};
}
