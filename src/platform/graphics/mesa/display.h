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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_MESA_DISPLAY_H_
#define MIR_GRAPHICS_MESA_DISPLAY_H_

#include "mir/graphics/display.h"
#include "real_kms_output_container.h"
#include "real_kms_display_configuration.h"
#include "display_helpers.h"

#include <mutex>

#include <vector>

namespace mir
{
namespace geometry
{
struct Rectangle;
}
namespace graphics
{

class DisplayReport;
class DisplayBuffer;
class DisplayConfigurationPolicy;
class EventHandlerRegister;

namespace mesa
{

class Platform;
class DisplayBuffer;
class KMSOutput;
class Cursor;

class Display : public graphics::Display
{
public:
    Display(std::shared_ptr<Platform> const& platform,
               std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
               std::shared_ptr<DisplayReport> const& listener);
    ~Display();

    geometry::Rectangle view_area() const;
    void for_each_display_buffer(
        std::function<void(graphics::DisplayBuffer&)> const& f);

    std::shared_ptr<DisplayConfiguration> configuration();
    void configure(DisplayConfiguration const& conf);

    void register_configuration_change_handler(
        EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler);

    void register_pause_resume_handlers(
        EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler);

    void pause();
    void resume();

    std::weak_ptr<graphics::Cursor> the_cursor();
    std::unique_ptr<GLContext> create_gl_context();

private:
    void clear_connected_unused_outputs();

    std::mutex configuration_mutex;
    std::shared_ptr<Platform> const platform;
    std::shared_ptr<DisplayReport> const listener;
    mir::udev::Monitor monitor;
    helpers::EGLHelper shared_egl;
    std::vector<std::unique_ptr<DisplayBuffer>> display_buffers;
    RealKMSOutputContainer output_container;
    RealKMSDisplayConfiguration current_display_configuration;
    std::shared_ptr<Cursor> cursor;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_DISPLAY_H_ */
