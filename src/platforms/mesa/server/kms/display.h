/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include "mir/renderer/gl/context_source.h"
#include "real_kms_output_container.h"
#include "real_kms_display_configuration.h"
#include "display_helpers.h"
#include "egl_helper.h"
#include "platform_common.h"

#include <atomic>
#include <mutex>
#include <vector>

namespace mir
{
class ConsoleServices;
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

namespace helpers
{
class DRMHelper;
class GBMHelper;
}

class DisplayBuffer;
class KMSOutput;
class Cursor;

class Display : public graphics::Display,
                public graphics::NativeDisplay,
                public renderer::gl::ContextSource
{
public:
    Display(std::vector<std::shared_ptr<helpers::DRMHelper>> const& drm,
            std::shared_ptr<helpers::GBMHelper> const& gbm,
            std::shared_ptr<ConsoleServices> const& vt,
            BypassOption bypass_option,
            std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
            std::shared_ptr<GLConfig> const& gl_config,
            std::shared_ptr<DisplayReport> const& listener);
    ~Display();

    geometry::Rectangle view_area() const;
    void for_each_display_sync_group(
        std::function<void(graphics::DisplaySyncGroup&)> const& f) override;

    std::unique_ptr<DisplayConfiguration> configuration() const override;
    bool apply_if_configuration_preserves_display_buffers(DisplayConfiguration const& conf) override;
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

    std::shared_ptr<graphics::Cursor> create_hardware_cursor() override;
    std::unique_ptr<VirtualOutput> create_virtual_output(int width, int height) override;
    NativeDisplay* native_display() override;

    std::unique_ptr<renderer::gl::Context> create_gl_context() const override;

    Frame last_frame_on(unsigned output_id) const override;

private:
    void clear_connected_unused_outputs();

    mutable std::mutex configuration_mutex;
    std::vector<std::shared_ptr<helpers::DRMHelper>> const drm;
    std::shared_ptr<helpers::GBMHelper> const gbm;
    std::shared_ptr<ConsoleServices> const vt;
    std::shared_ptr<DisplayReport> const listener;
    mir::udev::Monitor monitor;
    helpers::EGLHelper shared_egl;
    std::vector<std::unique_ptr<DisplayBuffer>> display_buffers;
    std::shared_ptr<KMSOutputContainer> const output_container;
    mutable RealKMSDisplayConfiguration current_display_configuration;
    mutable std::atomic<bool> dirty_configuration;

    void configure_locked(
        RealKMSDisplayConfiguration const& conf,
        std::lock_guard<decltype(configuration_mutex)> const&);

    BypassOption bypass_option;
    std::weak_ptr<Cursor> cursor;
    std::shared_ptr<GLConfig> const gl_config;
};

}
}
}

#endif /* MIR_GRAPHICS_MESA_DISPLAY_H_ */
