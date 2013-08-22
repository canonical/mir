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
#include "nested_gl_context.h"

#include "mir/geometry/rectangle.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mgn = mir::graphics::nested;
namespace geom = mir::geometry;

namespace
{
class MirDisplayConfigHandle
{
public:
    explicit MirDisplayConfigHandle(MirConnection* connection) :
    display_config{mir_connection_create_display_config(connection)}
    {
    }

    ~MirDisplayConfigHandle() noexcept
    {
        mir_display_config_destroy(display_config);
    }

    MirDisplayConfiguration* operator->() const { return display_config; }

private:
    MirDisplayConfiguration* const display_config;

    MirDisplayConfigHandle(MirDisplayConfigHandle const&) = delete;
    MirDisplayConfigHandle operator=(MirDisplayConfigHandle const&) = delete;
};

auto configure_outputs(MirConnection* connection)
-> std::unordered_map<uint32_t, std::shared_ptr<mgn::detail::NestedOutput>>
{
    MirDisplayConfigHandle display_config{connection};

    std::unordered_map<uint32_t, std::shared_ptr<mgn::detail::NestedOutput>> result;

    for (decltype(display_config->num_outputs) i = 0; i != display_config->num_outputs; ++i)
    {
        auto const egl_display_info = display_config->outputs+i;

        if (egl_display_info->used)
        {
            result[egl_display_info->output_id] =
                std::make_shared<mgn::detail::NestedOutput>(connection, egl_display_info);
        }
    }

    return result;
}

EGLint const egl_attribs[] = {
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_ALPHA_SIZE, 8,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE
};

EGLint const egl_context_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};
}


mgn::detail::MirSurfaceHandle::MirSurfaceHandle(MirConnection* connection, MirDisplayOutput* const egl_display_info)
{
    auto const egl_display_mode = egl_display_info->modes + egl_display_info->current_mode;
    auto const egl_display_format = egl_display_info->output_formats[egl_display_info->current_output_format];

    MirSurfaceParameters const request_params =
        {
            "Mir nested display",
            int(egl_display_mode->horizontal_resolution),
            int(egl_display_mode->vertical_resolution),
            egl_display_format,
            mir_buffer_usage_hardware,
            egl_display_info->output_id
        };

    mir_surface = mir_connection_create_surface_sync(connection, &request_params);

    if (!mir_surface_is_valid(mir_surface))
        BOOST_THROW_EXCEPTION(std::runtime_error(mir_surface_get_error_message(mir_surface)));
}

mgn::detail::MirSurfaceHandle::~MirSurfaceHandle() noexcept
{
    mir_surface_release_sync(mir_surface);
}

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

mgn::detail::NestedOutput::NestedOutput(MirConnection* connection, MirDisplayOutput* const egl_display_info) :
    mir_surface(connection, egl_display_info),
    egl_display{connection},
    egl_config{(egl_display.initialize(), egl_display.choose_config(egl_attribs))},
    egl_surface{egl_display, egl_display.egl_surface(egl_config, mir_surface)},
    egl_context{egl_display, eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, egl_context_attribs)}
{
    if (eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context) != EGL_TRUE)
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir Display Error: Failed to update EGL surface.\n"));
}

mgn::detail::NestedOutput::~NestedOutput() noexcept
{
}


mgn::NestedDisplay::NestedDisplay(MirConnection* connection, std::shared_ptr<mg::DisplayReport> const& display_report) :
    display_report{display_report},
    outputs{configure_outputs(connection)}
{
    if (outputs.empty())
        BOOST_THROW_EXCEPTION(std::runtime_error("Nested Mir needs at least one output for display"));
}

mgn::NestedDisplay::~NestedDisplay() noexcept
{
}

void mgn::NestedDisplay::post_update()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
    //mir_surface_swap_buffers_sync(mir_surface);
    //eglSwapBuffers(egl_display, egl_surface);
}

void mgn::NestedDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& /*f*/)
{
    // TODO
}

std::shared_ptr<mg::DisplayConfiguration> mgn::NestedDisplay::configuration()
{
    return std::make_shared<NestedDisplayConfiguration>();
}

void mgn::NestedDisplay::configure(mg::DisplayConfiguration const& /*configuration*/)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}

void mgn::NestedDisplay::register_configuration_change_handler(
        EventHandlerRegister& /*handlers*/,
        DisplayConfigurationChangeHandler const& /*conf_change_handler*/)
{
    // TODO
}

void mgn::NestedDisplay::register_pause_resume_handlers(
        EventHandlerRegister& /*handlers*/,
        DisplayPauseHandler const& /*pause_handler*/,
        DisplayResumeHandler const& /*resume_handler*/)
{
    // TODO
}

void mgn::NestedDisplay::pause()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}

void mgn::NestedDisplay::resume()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}

auto mgn::NestedDisplay::the_cursor()->std::weak_ptr<Cursor>
{
    return std::weak_ptr<Cursor>();
}

std::unique_ptr<mg::GLContext> mgn::NestedDisplay::create_gl_context()
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Not implemented yet!"));
}
