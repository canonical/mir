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
class GLConfig;

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
            std::shared_ptr<GLConfig> const& gl_config,
            std::shared_ptr<DisplayReport> const& listener);
    ~Display();

    geometry::Rectangle view_area() const;
    void for_each_display_sync_group(
        std::function<void(graphics::DisplaySyncGroup&)> const& f) override;

    std::unique_ptr<DisplayConfiguration> configuration() const override;
    void configure(DisplayConfiguration const& conf) override;

    void register_configuration_change_handler(
        EventHandlerRegister& handlers,
        DisplayConfigurationChangeHandler const& conf_change_handler) override;

    void register_pause_resume_handlers(
        EventHandlerRegister& handlers,
        DisplayPauseHandler const& pause_handler,
        DisplayResumeHandler const& resume_handler) override;

    void pause() override;
    void resume() override;

    std::shared_ptr<graphics::Cursor> create_hardware_cursor(std::shared_ptr<CursorImage> const& initial_image) override;
    std::unique_ptr<GLContext> create_gl_context() override;

private:
    void clear_connected_unused_outputs();

    mutable std::mutex configuration_mutex;
    std::shared_ptr<Platform> const platform;
    std::shared_ptr<DisplayReport> const listener;
    mir::udev::Monitor monitor;
    helpers::EGLHelper shared_egl;
    std::vector<std::unique_ptr<DisplayBuffer>> display_buffers;
    RealKMSOutputContainer output_container;
    mutable RealKMSDisplayConfiguration current_display_configuration;
    
    std::weak_ptr<Cursor> cursor;
    std::shared_ptr<GLConfig> const gl_config;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_DISPLAY_H_ */
