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

#include <unistd.h>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

struct mga::DisplayChangePipe
{
    DisplayChangePipe()
    {
        int pipes_raw[2] {-1, -1};
        ::pipe(pipes_raw);
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
static mg::DisplayConfigurationOutputId const primary_id{0};
static mg::DisplayConfigurationOutputId const external_id{1};

void safe_power_mode(mga::HwcConfiguration& config, MirPowerMode mode) noexcept
try
{
    config.power_mode(mga::DisplayName::primary, mode);
} catch (...) {}

//TODO: probably could be in the helper class
std::array<mg::DisplayConfigurationOutput, 2> query_configs(
    mga::HwcConfiguration& hwc_config,
    MirPixelFormat format)
{
    auto primary_attribs = hwc_config.active_attribs_for(mga::DisplayName::primary);
    auto external_attribs = hwc_config.active_attribs_for(mga::DisplayName::external);
    std::vector<mg::DisplayConfigurationMode> external_modes;
    if (external_attribs.connected)
        external_modes.emplace_back(
            mg::DisplayConfigurationMode{external_attribs.pixel_size, external_attribs.vrefresh_hz});
    return {
        mg::DisplayConfigurationOutput{
            primary_id,
            mg::DisplayConfigurationCardId{0},
            mg::DisplayConfigurationOutputType::lvds,
            {format},
            {mg::DisplayConfigurationMode{primary_attribs.pixel_size, primary_attribs.vrefresh_hz}},
            0,
            primary_attribs.mm_size,
            primary_attribs.connected,
            true,
            geom::Point{0,0},
            0,
            format,
            mir_power_mode_on,
            mir_orientation_normal
        },
        mg::DisplayConfigurationOutput{
            external_id,
            mg::DisplayConfigurationCardId{0},
            mg::DisplayConfigurationOutputType::displayport,
            {format},
            external_modes,
            0,
            external_attribs.mm_size,
            external_attribs.connected,
            false,
            geom::Point{0,0},
            0,
            format,
            mir_power_mode_on,
            mir_orientation_normal
        }
    };
}
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
    configurations(query_configs(*hwc_config, display_buffer_builder->display_format())),
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
    if (configurations[primary_id.as_value()].power_mode != mir_power_mode_off)
        safe_power_mode(*hwc_config, mir_power_mode_off);
}

void mga::Display::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    if (configurations[primary_id.as_value()].power_mode == mir_power_mode_on)
        f(*display_buffer);
}

std::unique_ptr<mg::DisplayConfiguration> mga::Display::configuration() const
{
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};
    if (configuration_dirty)
    {
        configurations = query_configs(*hwc_config, display_buffer_builder->display_format());
        configuration_dirty = false;
    }

    return std::unique_ptr<mg::DisplayConfiguration>(new mga::DisplayConfiguration(configurations));
}

void mga::Display::configure(mg::DisplayConfiguration const& new_configuration)
{
    if (!new_configuration.valid())
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid or inconsistent display configuration"));

    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    new_configuration.for_each_output([this](mg::DisplayConfigurationOutput const& output) {
        if (output.current_format != configurations[output.id.as_value()].current_format)
            BOOST_THROW_EXCEPTION(std::logic_error("could not change display buffer format"));

        //TODO: We don't support rotation yet, so
        //we preserve this orientation change so the compositor can rotate everything in GL 
        configurations[output.id.as_value()].orientation = output.orientation;

        //TODO: add the external displaybuffer
        if (output.id == primary_id)
        {
            if (output.power_mode != configurations[output.id.as_value()].power_mode)
            {
                hwc_config->power_mode(mga::DisplayName::primary, output.power_mode);
                configurations[output.id.as_value()].power_mode = output.power_mode;
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
    event_handler.register_fd_handler({display_change_pipe->read_pipe}, this, [&](int){
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
