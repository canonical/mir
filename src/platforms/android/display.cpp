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
#include "display_component_factory.h"
#include "interpreter_cache.h"
#include "server_render_window.h"
#include "display_buffer.h"
#include "mir/graphics/android/mir_native_window.h"
#include "mir/geometry/rectangle.h"

#include <boost/throw_exception.hpp>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

namespace
{
void safe_power_mode(mga::HwcConfiguration& config, MirPowerMode mode) noexcept
try
{
    config.power_mode(mga::DisplayName::primary, mode);
} catch (...) {}

std::unique_ptr<mga::ConfigurableDisplayBuffer> create_display_buffer(
    mga::DisplayComponentFactory& display_buffer_builder,
    mga::DisplayAttribs const& attribs,
    std::shared_ptr<mg::GLProgramFactory> const& gl_program_factory,
    mga::PbufferGLContext const& gl_context,
    mga::OverlayOptimization overlay_option)
{
    std::shared_ptr<mga::FramebufferBundle> fbs{display_buffer_builder.create_framebuffers(attribs)};
    auto cache = std::make_shared<mga::InterpreterCache>();
    auto interpreter = std::make_shared<mga::ServerRenderWindow>(fbs, cache);
    auto native_window = std::make_shared<mga::MirNativeWindow>(interpreter);
    return std::unique_ptr<mga::ConfigurableDisplayBuffer>(new mga::DisplayBuffer(
        fbs,
        display_buffer_builder.create_display_device(),
        native_window,
        gl_context,
        *gl_program_factory,
        mir_orientation_normal,
        overlay_option));
}

mg::DisplayConfigurationOutput fill_config_from(mga::DisplayAttribs const& attribs)
{
    return mg::DisplayConfigurationOutput{
        mg::DisplayConfigurationOutputId{1},
        mg::DisplayConfigurationCardId{0},
        mg::DisplayConfigurationOutputType::lvds,
        {attribs.display_format},
        {mg::DisplayConfigurationMode{attribs.pixel_size, attribs.vrefresh_hz}},
        0,
        attribs.mm_size,
        attribs.connected,
        true,
        geom::Point{0,0},
        0,
        attribs.display_format,
        mir_power_mode_on,
        mir_orientation_normal};
}
}

mga::Display::Display(
    std::shared_ptr<mga::DisplayComponentFactory> const& display_buffer_builder,
    std::shared_ptr<mg::GLProgramFactory> const& gl_program_factory,
    std::shared_ptr<GLConfig> const& gl_config,
    std::shared_ptr<DisplayReport> const& display_report,
    mga::OverlayOptimization overlay_option) :
    display_buffer_builder{display_buffer_builder},
    hwc_config{display_buffer_builder->create_hwc_configuration()},
    attribs(hwc_config->active_attribs_for(mga::DisplayName::primary)),
    primary_configuration(fill_config_from(attribs)),
    gl_context{attribs.display_format, *gl_config, *display_report},
    display_buffer{create_display_buffer(
        *display_buffer_builder,
        attribs,
        gl_program_factory,
        gl_context,
        overlay_option)}
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
    if (primary_configuration.power_mode != mir_power_mode_off)
        safe_power_mode(*hwc_config, mir_power_mode_off);
}

void mga::Display::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    if (primary_configuration.power_mode == mir_power_mode_on)
        f(*display_buffer);
}

std::unique_ptr<mg::DisplayConfiguration> mga::Display::configuration() const
{
    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    return std::unique_ptr<mg::DisplayConfiguration>(
        new mga::DisplayConfiguration(mg::DisplayConfigurationOutput(primary_configuration)));
}

void mga::Display::configure(mg::DisplayConfiguration const& new_configuration)
{
    if (!new_configuration.valid())
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid or inconsistent display configuration"));

    std::lock_guard<decltype(configuration_mutex)> lock{configuration_mutex};

    new_configuration.for_each_output([this](mg::DisplayConfigurationOutput const& output) {
        if (output.current_format != primary_configuration.current_format)
            BOOST_THROW_EXCEPTION(std::logic_error("could not change display buffer format"));

        if (output.power_mode != primary_configuration.power_mode)
        {
            hwc_config->power_mode(mga::DisplayName::primary, output.power_mode);
            primary_configuration.power_mode = output.power_mode;
        }

        //TODO: We don't support rotation yet, so
        //we preserve this orientation change so the compositor can rotate everything in GL 
        primary_configuration.orientation = output.orientation;

        display_buffer->configure(primary_configuration.power_mode, primary_configuration.orientation);
    });
}

void mga::Display::register_configuration_change_handler(
    EventHandlerRegister&,
    DisplayConfigurationChangeHandler const&)
{
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
