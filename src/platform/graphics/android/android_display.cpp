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
#include "android_display_configuration.h"
#include "mir/graphics/display_report.h"
#include "mir/graphics/display_buffer.h"
#include "mir/graphics/gl_context.h"
#include "mir/graphics/egl_resources.h"
#include "android_display.h"
#include "display_builder.h"
#include "mir/geometry/rectangle.h"

namespace mga=mir::graphics::android;
namespace mg=mir::graphics;
namespace geom=mir::geometry;

mga::AndroidDisplay::AndroidDisplay(std::shared_ptr<mga::DisplayBuilder> const& display_builder,
                                    std::shared_ptr<DisplayReport> const& display_report)
    : display_builder{display_builder},
      gl_context{display_builder->display_format(), *display_report},
      display_buffer{display_builder->create_display_buffer(gl_context)}
{
    display_report->report_successful_setup_of_native_resources();

    gl_context.make_current();

    display_report->report_successful_egl_make_current_on_construction();
    display_report->report_successful_display_construction();
}

void mga::AndroidDisplay::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    f(*display_buffer);
}

std::unique_ptr<mg::DisplayConfiguration> mga::AndroidDisplay::configuration() const
{
    return std::unique_ptr<mg::DisplayConfiguration>(
        new mga::AndroidDisplayConfiguration(display_buffer->configuration()));
}

void mga::AndroidDisplay::configure(mg::DisplayConfiguration const& configuration)
{
    configuration.for_each_output([&](mg::DisplayConfigurationOutput const& output)
    {
        display_buffer->configure(output);
    });
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
    return std::unique_ptr<mg::GLContext>{
        new mga::GLContext(gl_context, mga::create_dummy_pbuffer_surface)};
}
