/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GRAPHICS_GBM_ATOMIC_KMS_DISPLAY_H_
#define MIR_GRAPHICS_GBM_ATOMIC_KMS_DISPLAY_H_

#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/udev/wrapper.h"
#include "platform_common.h"
#include "real_kms_output_container.h"
#include "real_kms_display_configuration.h"

#include <atomic>
#include <mutex>
#include <vector>

namespace mir
{
namespace graphics
{

class DisplayInterfaceProvider;
class DisplayReport;
class DisplayConfigurationPolicy;
class EventHandlerRegister;
class GLConfig;

namespace atomic
{

namespace helpers
{
class DRMHelper;
class GBMHelper;
}

class DisplaySink;
class KMSOutput;
class Cursor;
class RuntimeQuirks;

class Display : public graphics::Display
{
public:
    Display(
        mir::Fd drm_fd,
        std::shared_ptr<struct gbm_device> gbm,
        BypassOption bypass_option,
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<DisplayReport> const& listener,
        std::shared_ptr<RuntimeQuirks> quirks);
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

    void pause() override;
    void resume() override;

    std::shared_ptr<graphics::Cursor> create_hardware_cursor() override;

private:
    void clear_connected_unused_outputs();

    mutable std::mutex configuration_mutex;
    mir::Fd const drm_fd;
    std::shared_ptr<struct gbm_device> const gbm;
    std::shared_ptr<DisplayReport> const listener;
    mir::udev::Monitor monitor;
    std::shared_ptr<KMSOutputContainer> const output_container;
    std::vector<std::unique_ptr<DisplaySink>> display_sinks;
    mutable RealKMSDisplayConfiguration current_display_configuration;
    mutable std::atomic<bool> dirty_configuration;

    void configure_locked(
        RealKMSDisplayConfiguration const& conf,
        std::lock_guard<decltype(configuration_mutex)> const&);

    BypassOption bypass_option;
    std::weak_ptr<Cursor> cursor;
    std::shared_ptr<RuntimeQuirks> const runtime_quirks;
};

class GBMDisplayProvider : public graphics::GBMDisplayProvider
{
public:
    explicit GBMDisplayProvider(mir::Fd drm_fd);

    auto is_same_device(mir::udev::Device const& render_device) const -> bool override;

    auto on_this_sink(graphics::DisplaySink& sink) const -> bool override;

    auto gbm_device() const -> std::shared_ptr<struct gbm_device> override;

private:
    mir::Fd const fd;
    std::shared_ptr<struct gbm_device> const gbm;
};

}
}
}

#endif /* MIR_GRAPHICS_GBM_ATOMIC_KMS_DISPLAY_H_ */
