/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "display_configuration.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/egl_resources.h"
#include "display.h"

#include <boost/throw_exception.hpp>
#include <fcntl.h>
#include <mutex>

namespace mg=mir::graphics;
namespace mgx=mg::X;
namespace geom=mir::geometry;

mgx::Display::Display()
{
#if 0
    display = XOpenDisplay(0);
    // TODO: check for error
    int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
    int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

    Window mir_display = XCreateSimpleWindow(display, DefaultRootWindow(display), 0, 0,
				     200, 100, 0, blackColor, blackColor);

    XSelectInput(dpy, w, StructureNotifyMask);

    // "Map" the window (that is, make it appear on the screen)

    XMapWindow(dpy, w);

    // Create a "Graphics Context"

    GC gc = XCreateGC(dpy, w, 0, NIL);

    // Tell the GC we draw using the white color

    XSetForeground(dpy, gc, whiteColor);

    // Wait for the MapNotify event

    for(;;) {
	    XEvent e;
	    XNextEvent(dpy, &e);
	    if (e.type == MapNotify)
		  break;
    }
#endif
}

mgx::Display::~Display() noexcept
{
}

void mgx::Display::for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& /*f*/)
{
#if 0
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    f(displays);
#endif
}

std::unique_ptr<mg::DisplayConfiguration> mgx::Display::configuration() const
{
#if 0
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};
    return std::unique_ptr<mg::DisplayConfiguration>(new mgx::DisplayConfiguration(config));
#else
    return std::make_unique<mgx::DisplayConfiguration>();
#endif
}

void mgx::Display::configure(mg::DisplayConfiguration const& /*new_configuration*/)
{
#if 0
    if (!new_configuration.valid())
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid or inconsistent display configuration"));

    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    new_configuration.for_each_output([this](mg::DisplayConfigurationOutput const& output)
    {
        if (output.current_format != config[output.id].current_format)
            BOOST_THROW_EXCEPTION(std::logic_error("could not change display buffer format"));

        config[output.id].orientation = output.orientation;
        if (config.primary().id == output.id)
        {
            power_mode(mga::DisplayName::primary, *hwc_config, config.primary(), output.power_mode);
            displays.configure(mga::DisplayName::primary, output.power_mode, output.orientation);
        }
        else if (config.external().connected)
        {
            power_mode(mga::DisplayName::external, *hwc_config, config.external(), output.power_mode);
            displays.configure(mga::DisplayName::external, output.power_mode, output.orientation);
        }
    });
#endif
}

void mgx::Display::register_configuration_change_handler(
    EventHandlerRegister& /* event_handler */,
    DisplayConfigurationChangeHandler const& /* change_handler */)
{
#if 0
    event_handler.register_fd_handler({display_change_pipe->read_pipe}, this, [change_handler, this](int){
        change_handler();
        display_change_pipe->ack_change();
    });
#endif
}

void mgx::Display::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mgx::Display::pause()
{
}

void mgx::Display::resume()
{
}

auto mgx::Display::create_hardware_cursor(std::shared_ptr<mg::CursorImage> const& /* initial_image */) -> std::shared_ptr<Cursor>
{
    return nullptr;
}

std::unique_ptr<mg::GLContext> mgx::Display::create_gl_context()
{
#if 0
    return std::unique_ptr<mg::GLContext>{new mga::PbufferGLContext(gl_context)};
#else
    return nullptr;
#endif
}
