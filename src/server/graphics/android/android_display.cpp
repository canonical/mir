/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/platform.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/egl_resources.h"
#include "android_display.h"
#include "android_display_buffer_factory.h"
#include "display_device.h"
#include "android_format_conversion-inl.h"
#include "mir/geometry/rectangle.h"

#include <boost/throw_exception.hpp>

#include <system/window.h>
#include <algorithm>
#include <stdexcept>

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

mga::AndroidDisplay::AndroidDisplay(std::shared_ptr<mga::AndroidDisplayBufferFactory> const& db_factory,
                                    std::shared_ptr<DisplayReport> const& display_report)
    : db_factory{db_factory},
      display_device(db_factory->create_display_device()),
      gl_context{display_device->display_format()},
      display_buffer{db_factory->create_display_buffer(
          display_device, gl_context),
      current_configuration{display_buffer->view_area().size}
{
    display_report->report_successful_setup_of_native_resources();

    gl_context.make_current();

    display_report->report_successful_egl_make_current_on_construction();
    display_report->report_successful_display_construction();
    display_report->report_egl_configuration(egl_display, egl_config);
}

void mga::AndroidDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    f(*display_buffer);
}

std::shared_ptr<mg::DisplayConfiguration> mga::AndroidDisplay::configuration()
{
    return std::make_shared<mga::AndroidDisplayConfiguration>(current_configuration);
}

void mga::AndroidDisplay::configure(mg::DisplayConfiguration const& configuration)
{
    configuration.for_each_output([&](mg::DisplayConfigurationOutput const& output)
    {
        // TODO: Properly support multiple outputs
        display_device->mode(output.power_mode);
    });
    current_configuration = dynamic_cast<mga::AndroidDisplayConfiguration const&>(configuration);
}

void mga::AndroidDisplay::register_configuration_change_handler(
    EventHandlerRegister&,
    DisplayConfigurationChangeHandler const&)
{
}

void mga::AndroidDisplay::register_pause_resume_handlers(
    EventHandlerRegister& /*handlers*/,
    DisplayPauseHandler const& /*pause_handler*/,
    DisplayResumeHandler const& /*resume_handler*/)
{
}

void mga::AndroidDisplay::pause()
{
}

void mga::AndroidDisplay::resume()
{
}

auto mga::AndroidDisplay::the_cursor() -> std::weak_ptr<Cursor>
{
    return std::weak_ptr<Cursor>();
}

std::unique_ptr<mg::GLContext> mga::AndroidDisplay::create_gl_context()
{
    return std::unique_ptr<AndroidGLContext>{
        new EGLHelper(gl_context, mga::create_dummy_pbuffer_surface)}
}
