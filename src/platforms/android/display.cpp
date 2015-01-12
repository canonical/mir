/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "display_configuration.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/egl_resources.h"
#include "display.h"
#include "display_buffer.h"
#include "display_buffer_builder.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/event_handler_register.h"

#include <boost/throw_exception.hpp>
#include <fcntl.h>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

struct mga::DisplayChangePipe
{
    DisplayChangePipe()
    {
        int pipes_raw[2] {-1, -1};
        if(::pipe2(pipes_raw, O_CLOEXEC | O_NONBLOCK))
            BOOST_THROW_EXCEPTION(std::runtime_error("failed to create display change pipe"));
        read_pipe = mir::Fd{pipes_raw[0]}; 
        write_pipe = mir::Fd{pipes_raw[1]}; 
    }
    void notify_change() { ::write(write_pipe, &data, sizeof(data)); }
    void ack_change()
    {
        char tmp{'b'};
        ::read(read_pipe, &tmp, sizeof(tmp));
    }
    mir::Fd read_pipe;
    mir::Fd write_pipe;
    char const data{'a'};
};

namespace
{
void safe_power_mode(mga::HwcConfiguration& config, MirPowerMode mode) noexcept
try
{
    config.power_mode(mga::DisplayName::primary, mode);
} catch (...) {}
}

mga::Display::Display(
    std::shared_ptr<mga::DisplayBufferBuilder> const& display_buffer_builder,
    std::shared_ptr<mg::GLProgramFactory> const& gl_program_factory,
    std::shared_ptr<GLConfig> const& gl_config,
    std::shared_ptr<DisplayReport> const& display_report) :
    display_buffer_builder{display_buffer_builder},
    gl_context{display_buffer_builder->display_format(), *gl_config, *display_report},
    display_buffer{display_buffer_builder->create_display_buffer(*gl_program_factory, gl_context)},
    hwc_config{display_buffer_builder->create_hwc_configuration()},
    hotplug_subscription{hwc_config->subscribe_to_config_changes(std::bind(&mga::Display::on_hotplug, this))},
    config(
        hwc_config->active_attribs_for(mga::DisplayName::primary),
        hwc_config->active_attribs_for(mga::DisplayName::external),
        display_buffer_builder->display_format()),
    display_change_pipe(new DisplayChangePipe)
{
    //Some drivers (depending on kernel state) incorrectly report an error code indicating that the display is already on. Ignore the first failure.
    safe_power_mode(*hwc_config, mir_power_mode_on);

    display_report->report_successful_setup_of_native_resources();

    gl_context.make_current();

    display_report->report_successful_egl_make_current_on_construction();
    display_report->report_successful_display_construction();
}

mga::Display::~Display() noexcept
{
    if (config.primary_config().power_mode != mir_power_mode_off)
        safe_power_mode(*hwc_config, mir_power_mode_off);
}

void mga::Display::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    if (config.primary_config().power_mode == mir_power_mode_on)
        f(*display_buffer);
}

std::unique_ptr<mg::DisplayConfiguration> mga::Display::configuration() const
{
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};
    if (configuration_dirty)
    {
        config = mga::DisplayConfiguration(
            hwc_config->active_attribs_for(mga::DisplayName::primary),
            hwc_config->active_attribs_for(mga::DisplayName::external),
            display_buffer_builder->display_format());
        configuration_dirty = false;
    }

    return std::unique_ptr<mg::DisplayConfiguration>(new mga::DisplayConfiguration(config));
}

void mga::Display::configure(mg::DisplayConfiguration const& new_configuration)
{
    if (!new_configuration.valid())
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid or inconsistent display configuration"));

    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    new_configuration.for_each_output([this](mg::DisplayConfigurationOutput const& output) {
        //TODO: support configuring the external displaybuffer
        if (config.primary_config().id == output.id)
        {
            if (output.current_format != config[output.id].current_format)
                BOOST_THROW_EXCEPTION(std::logic_error("could not change display buffer format"));

            //TODO: We don't support rotation yet, so
            //we preserve this orientation change so the compositor can rotate everything in GL 
            config[output.id].orientation = output.orientation;

            if (output.power_mode != config[output.id].power_mode)
            {
                hwc_config->power_mode(mga::DisplayName::primary, output.power_mode);
                config[output.id].power_mode = output.power_mode;
            }

            display_buffer->configure(output.power_mode, output.orientation);
        }
    });
}

//NOTE: We cannot call back to hwc from within the hotplug callback. Only arrange for an update.
void mga::Display::on_hotplug()
{
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};
    configuration_dirty = true;
    display_change_pipe->notify_change();
}

void mga::Display::register_configuration_change_handler(
    EventHandlerRegister& event_handler,
    DisplayConfigurationChangeHandler const& change_handler)
{
    event_handler.register_fd_handler({display_change_pipe->read_pipe}, this, [change_handler, this](int){
        change_handler();
        display_change_pipe->ack_change();
    });
}

void mga::Display::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mga::Display::pause()
{
}

void mga::Display::resume()
{
}

auto mga::Display::create_hardware_cursor(std::shared_ptr<mg::CursorImage> const& /* initial_image */) -> std::shared_ptr<Cursor>
{
    return nullptr;
}

std::unique_ptr<mg::GLContext> mga::Display::create_gl_context()
{
    return std::unique_ptr<mg::GLContext>{new mga::PbufferGLContext(gl_context)};
}
